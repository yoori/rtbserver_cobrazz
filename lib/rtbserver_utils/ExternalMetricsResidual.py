"""Importer-owned estimated remainder and a replayable ClickHouse CSV outbox."""

import datetime
import json
import uuid

from ServiceUtilsPy.Context import Context
from rtbserver_utils.ExternalMetricsTypes import YANDEX_METRIKA


UTC = datetime.timezone.utc
METRICS = (
  'visits', 'visits_with_bounce', 'visits_robot', 'visits_robot_available',
  'session_time_sum', 'page_views', 'new_user_visits')
RESIDUAL = 'YandexPostClickResidual'
OUTBOX = 'YandexPostClickResidualOutbox'
CLOSED = 'YandexPostClickClosedPeriods'
BOOTSTRAP = 'YandexPostClickResidualBootstrap'
COVERAGE = 'YandexPostClickCoverageV2'
CSV_HEADER = (
  'batch_id', 'target_scope', 'external_metrics_connection_type_id', 'sdate', 'adv_sdate',
  'campaign_id', 'ccg_id', 'cc_id', *METRICS)
DDL = (
  f"""CREATE TABLE IF NOT EXISTS {RESIDUAL} (
    metrika_id UInt64, hour DateTime('UTC'), cc_id UInt64,
    source_date Date, target_scope String, campaign_id UInt64, ccg_id UInt64,
    payload String, version UInt64
  ) ENGINE = ReplacingMergeTree(version)
  ORDER BY (metrika_id, hour, cc_id, source_date,
    target_scope, campaign_id, ccg_id)""",
  f"""CREATE TABLE IF NOT EXISTS {OUTBOX} (
    batch_id String, payload String, done UInt8, version UInt64
  ) ENGINE = ReplacingMergeTree(version) ORDER BY batch_id""",
  f"""CREATE TABLE IF NOT EXISTS {CLOSED} (
    metrika_id UInt64, source_date Date, version UInt64
  ) ENGINE = ReplacingMergeTree(version)
  ORDER BY (metrika_id, source_date)""",
  f"""CREATE TABLE IF NOT EXISTS {BOOTSTRAP} (
    metrika_id UInt64, version UInt64
  ) ENGINE = ReplacingMergeTree(version) ORDER BY metrika_id""",
  f"""CREATE TABLE IF NOT EXISTS {COVERAGE} (
    metrika_id UInt64, source_date Date, visit_id UInt64,
    hour DateTime('UTC'), cc_id UInt64, metrics Array(Float64), version UInt64
  ) ENGINE = ReplacingMergeTree(version)
  PARTITION BY toYYYYMM(source_date)
  ORDER BY (metrika_id, source_date, visit_id)""",
)


def utc(value):
  if isinstance(value, str):
    value = datetime.datetime.fromisoformat(value)
  return value.replace(tzinfo=UTC) if value.tzinfo is None else value.astimezone(UTC)


def remainder(reporting, covered):
  result = [max(a - b, 0) for a, b in zip(reporting, covered)]
  if result[0] == 0:
    return [0] * len(METRICS)
  for index in (1, 2, 3, 6):
    result[index] = min(result[index], result[0])
  result[2] = min(result[2], result[3])
  return result


def revision():
  return int(datetime.datetime.now(UTC).timestamp() * 1000000)


def row_key(row):
  return row['hour'], row['cc_id'], row['source_date']


def legacy_seed_key(connection_id, row):
  return (connection_id, row.get('target_scope', ''), utc(row['hour']).isoformat(),
    int(row['cc_id']), str(row['source_date']), int(row['campaign_id']), int(row['ccg_id']))


class ResidualPublisher:
  def __init__(self, app):
    self.app = app

  def rows(self, metrika_id=None):
    condition = '' if metrika_id is None else ' WHERE metrika_id = %(id)s'
    return [json.loads(row[0]) for row in self.app.ch.query(
      f'SELECT payload FROM {RESIDUAL} FINAL' + condition,
      parameters={} if metrika_id is None else {'id': metrika_id}).result_rows]

  def bootstrap(self, metrika_id, timezone):
    self.drain()
    if self.app.ch.query(
        f'SELECT 1 FROM {BOOTSTRAP} FINAL WHERE metrika_id = %(id)s',
        parameters={'id': metrika_id}).result_rows:
      return
    if callable(timezone):
      timezone = timezone()
    with self.app.pg.cursor() as cursor:
      cursor.execute(
        'SELECT s.external_metrics_connection_id, target_scope, hour, cc_id, source_date, '
        'adv_sdate, campaign_id, ccg_id, '
        + ', '.join('sum(s.' + name + ')' for name in METRICS)
        + ' FROM adserver.externalpostclickresidualseed s '
        'JOIN externalmetricsconnectionyandexmetrika y USING (external_metrics_connection_id) '
        'WHERE y.metrika_id = %s AND NOT (s.external_metrics_connection_id = ANY(%s)) '
        'GROUP BY s.external_metrics_connection_id, target_scope, hour, cc_id, source_date, '
        'adv_sdate, campaign_id, ccg_id',
        (metrika_id, sorted(getattr(self.app, 'legacy_bootstrapped_connections', set()))))
      seeds = cursor.fetchall()
    self.app.pg.commit()
    rows = self.rows(metrika_id)
    for (connection, scope, hour, ccid, source_date, adv_date,
        campaign_id, ccg_id, *metrics) in seeds:
      period = source_date
      row = dict(
        metrika_id=metrika_id, target_scope=scope,
        hour=utc(hour).isoformat(), cc_id=int(ccid),
        source_date=source_date.isoformat(), period=period.isoformat(),
        adv_sdate=adv_date.isoformat(), campaign_id=int(campaign_id), ccg_id=int(ccg_id),
        metrics=[float(value) for value in metrics], deadline=self.deadline(period, timezone))
      if legacy_seed_key(connection, row) not in getattr(self.app, 'legacy_residual_keys', set()):
        rows.append(row)
    combined = {}
    for row in rows:
      key = (*row_key(row), row.get('target_scope', ''), row['campaign_id'], row['ccg_id'])
      if key in combined:
        previous = combined[key]
        previous['metrics'] = [a + b for a, b in zip(previous['metrics'], row['metrics'])]
      else:
        combined[key] = row
    payload = json.dumps(dict(rows=list(combined.values()), deltas=[], closed=[],
      bootstrap=[metrika_id]), sort_keys=True)
    self.app.ch.insert(OUTBOX, [(uuid.uuid4().hex, payload, 0, revision())],
      column_names=('batch_id', 'payload', 'done', 'version'))
    self.drain()

  def deadline(self, date, timezone):
    end = datetime.datetime.combine(date + datetime.timedelta(days=1), datetime.time(), timezone)
    return (end.astimezone(UTC) + datetime.timedelta(hours=self.app.logs_expiry_hours)).isoformat()

  def _save_rows(self, rows, version):
    if rows:
      self.app.ch.insert(RESIDUAL, [
        (row['metrika_id'], utc(row['hour']), row['cc_id'],
         datetime.date.fromisoformat(row['source_date']), row.get('target_scope', ''),
         row['campaign_id'], row['ccg_id'], json.dumps(row), version)
        for row in rows], column_names=(
          'metrika_id', 'hour', 'cc_id', 'source_date', 'target_scope',
          'campaign_id', 'ccg_id', 'payload', 'version'))

  def drain(self):
    pending = self.app.ch.query(
      f'SELECT batch_id, payload, version FROM {OUTBOX} FINAL WHERE done = 0 ORDER BY version'
    ).result_rows
    for batch_id, payload, version in pending:
      self.app.verify_running()
      batch = json.loads(payload)
      if batch['deltas']:
        with Context(self.app, out_dir=self.app.estimation_dir) as context:
          filename = 'ExternalPostClickEstimationStats_' + context.fname_seed.replace('.', '-')
          writer = context.files.get_line_writer(name=filename + '.csv')
          writer.write_line(self.app._csv_line(CSV_HEADER))
          for row in batch['deltas']:
            metrics = [value if i == 4 else int(value) for i, value in enumerate(row['metrics'])]
            writer.write_line(self.app._csv_line((
              batch_id, row.get('target_scope', ''), YANDEX_METRIKA,
              row['hour'], row['adv_sdate'],
              row['campaign_id'], row['ccg_id'], row['cc_id'], *metrics)))
          writer.write('\n')
      self._save_rows(batch['rows'], version)
      if batch['closed']:
        self.app.ch.insert(CLOSED, [
          (conn, datetime.date.fromisoformat(date), version) for conn, date in batch['closed']],
          column_names=('metrika_id', 'source_date', 'version'))
      if batch.get('bootstrap'):
        self.app.ch.insert(BOOTSTRAP, [(counter, version) for counter in batch['bootstrap']],
          column_names=('metrika_id', 'version'))
      self.app.ch.insert(OUTBOX, [(batch_id, payload, 1, version + 1)],
        column_names=('batch_id', 'payload', 'done', 'version'))

  def publish(self, old, new, closed=()):
    self.drain()
    def key(row):
      return (row['metrika_id'], *row_key(row), row.get('target_scope', ''),
        row['campaign_id'], row['ccg_id'])
    previous = {key(row): row for row in old}
    changes = []
    deltas = []
    for row in new:
      before = previous.get(key(row))
      difference = [a - b for a, b in zip(
        row['metrics'], before['metrics'] if before else [0] * len(METRICS))]
      if before is None or row != before:
        changes.append(row)
      if any(difference):
        deltas.append(dict(row, metrics=difference))
    if changes or closed:
      batch_id = uuid.uuid4().hex
      payload = json.dumps(dict(rows=changes, deltas=deltas, closed=list(closed)), sort_keys=True)
      self.app.ch.insert(OUTBOX, [(batch_id, payload, 0, revision())],
        column_names=('batch_id', 'payload', 'done', 'version'))
      self.drain()

  def finalize(self, now=None):
    self.drain()
    now = now or datetime.datetime.now(UTC)
    old = self.rows()
    expired = [row for row in old if utc(row['deadline']) <= now]
    closed = sorted({(row['metrika_id'], row['period']) for row in expired})
    known = {(int(conn), date.isoformat()) for conn, date in self.app.ch.query(
      f'SELECT metrika_id, source_date FROM {CLOSED} FINAL').result_rows}
    self.publish(old, [dict(row, metrics=[0] * len(METRICS)) for row in expired],
      [key for key in closed if key not in known])

  def remember(self, metrika_id, records, ccids, timezone=UTC):
    if not records:
      return
    dates = tuple({record[1] for record in records})
    known = {(date, int(visit)) for date, visit in self.app.ch.query(
      f'SELECT source_date, visit_id FROM {COVERAGE} FINAL '
      'WHERE metrika_id = %(id)s AND source_date IN %(dates)s',
      parameters={'id': metrika_id, 'dates': dates}).result_rows}
    added = []
    for visit, date, event_time, _, _, payload, comparable in records:
      if (date, visit) in known:
        continue
      known.add((date, visit))
      value = json.loads(payload)
      metrics = [1, int(value['landing_bounced']), int(value.get('landing_is_robot', False)),
        int(value.get('landing_robot_available', False)), value['landing_session_time'],
        value['landing_page_views'], int(value['landing_is_new_user'])]
      local_hour = event_time.astimezone(timezone).replace(minute=0, second=0, microsecond=0)
      hour = local_hour.astimezone(UTC).replace(minute=0)
      added.append((metrika_id, date, visit, hour,
        ccids.get(visit, 0) if comparable else 0, metrics, revision()))
    if added:
      self.app.ch.insert(COVERAGE, added, column_names=(
        'metrika_id', 'source_date', 'visit_id', 'hour', 'cc_id',
        'metrics', 'version'))

  def reconcile(self, metrika_id, api, reporting):
    self.drain()
    timezone = api.timezone()
    self.bootstrap(metrika_id, timezone)
    old = self.rows(metrika_id)
    covered = {}
    coverage_ids = set()
    for date, visit, hour, ccid, metrics in self.app.ch.query(
        f'SELECT source_date, visit_id, hour, cc_id, metrics FROM {COVERAGE} FINAL '
        'WHERE metrika_id = %(id)s',
        parameters={'id': metrika_id}).result_rows:
      coverage_ids.add((date, int(visit)))
      key = (utc(hour), int(ccid))
      covered[key] = [a + b for a, b in zip(covered.get(key, [0] * len(METRICS)), metrics)]
    # Legacy sync rows have only payload hashes. Do not recreate a migrated estimate until
    # a refreshed export has reconstructed all previously published visits for that period.
    incomplete = {date for date, visit in self.app.ch.query(
      'SELECT source_date, visit_id FROM YandexPostClickLogsSyncV3 FINAL '
      'WHERE metrika_id = %(id)s',
      parameters={'id': metrika_id}).result_rows if (date, int(visit)) not in coverage_ids}
    closed = {date for (date,) in self.app.ch.query(
      f'SELECT source_date FROM {CLOSED} FINAL WHERE metrika_id = %(id)s',
      parameters={'id': metrika_id}).result_rows}
    desired = {}
    now = datetime.datetime.now(UTC)
    for key, values in reporting['rows'].items():
      hour, ccid = key
      date = reporting.get('source_dates', {}).get(key, hour.astimezone(timezone).date())
      if date in incomplete:
        continue
      metrics = remainder(values[:len(METRICS)], covered.get(key, [0] * len(METRICS)))
      if date in closed or utc(self.deadline(date, timezone)) <= now:
        metrics = [0] * len(METRICS)
      desired[(hour.isoformat(), ccid, date.isoformat())] = metrics
    previous = {row_key(row): row for row in old if not row.get('target_scope')}
    ccids = sorted({key[1] for key in desired if key not in previous and any(desired[key])})
    mapping = {}
    if ccids:
      with self.app.pg.cursor() as cursor:
        cursor.execute(
          'SELECT cc.cc_id, g.campaign_id, cc.ccg_id, tz.tzname FROM campaigncreative cc '
          'JOIN campaigncreativegroup g ON g.ccg_id = cc.ccg_id '
          'JOIN campaign c ON c.campaign_id = g.campaign_id '
          'JOIN account a ON a.account_id = c.account_id '
          'JOIN timezone tz ON tz.timezone_id = a.timezone_id WHERE cc.cc_id = ANY(%s)', (ccids,))
        mapping = {int(row[0]): row[1:] for row in cursor.fetchall()}
      self.app.pg.commit()
    from zoneinfo import ZoneInfo
    new = [dict(row, metrics=[0] * len(METRICS)) for row in old
      if row.get('target_scope') and datetime.date.fromisoformat(row['period']) not in incomplete
      and reporting['date1'] <= datetime.date.fromisoformat(row['period']) <= reporting['date2']]
    for key in set(previous) | set(desired):
      hour, ccid, date = key
      period = datetime.date.fromisoformat(date)
      if period in incomplete:
        continue
      if not reporting['date1'] <= period <= reporting['date2']:
        continue
      if key in previous:
        row = dict(previous[key], metrics=desired.get(key, [0] * len(METRICS)))
      elif ccid in mapping and any(desired[key]):
        campaign, ccg, adv_timezone = mapping[ccid]
        row = dict(metrika_id=metrika_id, hour=hour, cc_id=ccid, source_date=date,
          period=period.isoformat(), adv_sdate=utc(hour).astimezone(ZoneInfo(adv_timezone)).date()
          .isoformat(), campaign_id=int(campaign), ccg_id=int(ccg), metrics=desired[key],
          deadline=self.deadline(period, timezone))
      else:
        continue
      new.append(row)
    self.publish(old, new)
    self.finalize(now)
