"""Upgrade connection-scoped importer state without replaying published visits."""

import json

from rtbserver_utils.ExternalMetricsResidual import ResidualPublisher, legacy_seed_key


def migrate(app):
  app.ch.command('CREATE TABLE IF NOT EXISTS ExternalMetricsSchemaMigrations '
    '(name String) ENGINE = ReplacingMergeTree ORDER BY name')
  exists = lambda table: bool(app.ch.command(f'EXISTS TABLE {table}'))
  app.legacy_bootstrapped_connections = set()
  if exists('ExternalMetricsResidualBootstrap'):
    app.legacy_bootstrapped_connections = {int(row[0]) for row in app.ch.query(
      'SELECT external_metrics_connection_id FROM ExternalMetricsResidualBootstrap FINAL'
    ).result_rows}
  previous = []
  if exists('ExternalMetricsResidual'):
    previous = [json.loads(payload) for (payload,) in app.ch.query(
      'SELECT payload FROM ExternalMetricsResidual FINAL').result_rows]
  app.legacy_residual_keys = {legacy_seed_key(row['connection_id'], row) for row in previous}
  name = 'YandexPostClickCounterIdentityV3'
  if app.ch.query('SELECT 1 FROM ExternalMetricsSchemaMigrations FINAL WHERE name = %(name)s',
      parameters={'name': name}).result_rows:
    return
  if exists('ExternalMetricsResidualOutbox') and app.ch.query(
      'SELECT count() FROM ExternalMetricsResidualOutbox FINAL WHERE done = 0'
      ).result_rows[0][0]:
    raise RuntimeError('Drain the previous importer residual outbox before upgrading')
  with app.pg.cursor() as cursor:
    cursor.execute('SELECT external_metrics_connection_id, metrika_id '
      'FROM externalmetricsconnectionyandexmetrika')
    counters = {int(connection): int(counter) for connection, counter in cursor.fetchall()}
  app.pg.commit()

  def copy(destination, source, fields, legacy=False):
    if not exists(source):
      return
    identity = 'ymref_id' if legacy else 'external_metrics_connection_id'
    ids = {int(row[0]) for row in app.ch.query(
      f'SELECT DISTINCT {identity} FROM {source} FINAL').result_rows}
    if ids - counters.keys():
      raise RuntimeError(f'Cannot map historical connections in {source} to Yandex counters')
    columns = ', '.join(('metrika_id', *fields))
    values = ', '.join('event_date' if legacy and field == 'source_date' else field
      for field in fields)
    for connection in sorted(ids):
      app.ch.command(f'INSERT INTO {destination} ({columns}) '
        f'SELECT {counters[connection]}, {values} FROM {source} FINAL '
        f'WHERE {identity} = {connection}')

  for base, fields in (
      ('YandexPostClickReportingSync', ('hour', 'ccid', 'visits', 'visits_with_bounce',
        'visits_robot', 'visits_robot_available', 'session_time_sum', 'page_views',
        'new_user_visits', 'sampled', 'sample_share', 'version')),
      ('YandexPostClickLogsSync', ('visit_id', 'source_date', 'request_id', 'payload_hash',
        'reporting_comparable', 'published_at')),
      ('YandexPostClickLogRequests', ('source_date', 'request_id', 'status', 'version'))):
    migrated = app.ch.query(
      'SELECT 1 FROM ExternalMetricsSchemaMigrations FINAL WHERE name = %(name)s',
      parameters={'name': base + 'V2'}).result_rows
    # An empty V2 table can remain after an interrupted upgrade. A completed V2 migration,
    # however, supersedes V1: copying V1 again could resurrect obsolete API request IDs.
    if not migrated:
      copy(base + 'V3', base, fields, legacy=True)
    copy(base + 'V3', base + 'V2', fields)
  copy('YandexPostClickCoverageV2', 'YandexPostClickCoverage',
    ('source_date', 'visit_id', 'hour', 'cc_id', 'metrics', 'version'))
  copy('YandexPostClickConvNamesSyncV3', 'YandexPostClickConvNamesSyncV2',
    ('conversion_name', 'name', 'version'))
  copy('YandexPostClickClosedPeriods', 'ExternalMetricsClosedPeriods', ('source_date', 'version'))

  if previous:
    combined = {}
    for row in previous:
      connection = row.pop('connection_id')
      if connection not in counters:
        raise RuntimeError('Cannot map a residual connection to its Yandex counter')
      row['metrika_id'] = counters[connection]
      key = tuple(row.get(field, '') for field in ('metrika_id', 'hour', 'cc_id', 'source_date',
        'target_scope', 'campaign_id', 'ccg_id'))
      if key in combined:
        previous = combined[key]
        if {k: v for k, v in previous.items() if k != 'metrics'} != {
            k: v for k, v in row.items() if k != 'metrics'}:
          raise RuntimeError('Conflicting residual metadata for a shared Yandex counter')
        previous['metrics'] = [a + b for a, b in zip(previous['metrics'], row['metrics'])]
      else:
        combined[key] = row
    # PG still contains the sum of all old E balances, not a deduplicated estimate.
    ResidualPublisher(app)._save_rows(combined.values(), 1)
  app.ch.insert('ExternalMetricsSchemaMigrations', [(name,)], column_names=('name',))
