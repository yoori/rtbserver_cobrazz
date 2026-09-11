#!/usr/bin/python3.12

import base64
import csv
import datetime
import hashlib
import json
import os
import re
import urllib.parse

import clickhouse_connect
import psycopg2
import requests

from ServiceUtilsPy.Context import Context
from ServiceUtilsPy.Service import Service


REPORTING_SYNC_TABLE = 'YandexPostClickReportingSync'
LOGS_SYNC_TABLE = 'YandexPostClickLogsSync'
LOG_REQUESTS_TABLE = 'YandexPostClickLogRequests'
ESTIMATION_TABLE = 'adserver.ccgpostclickestimationstatshourly'
DAY_STATUS_TABLE = 'adserver.yandexpostclickdaystatus'
EXACT_PROGRESS_TABLE = 'adserver.yandexpostclickexactprogress'

INTERNAL_CLICKHOUSE_DDL = (
  f"""
CREATE TABLE IF NOT EXISTS {REPORTING_SYNC_TABLE}
(
  ymref_id UInt64,
  hour DateTime('UTC'),
  ccid UInt64,
  visits UInt64,
  visits_with_bounce UInt64,
  session_time_sum Float64,
  page_views UInt64,
  new_user_visits UInt64,
  sampled UInt8,
  sample_share Float64,
  version DateTime64(3, 'UTC')
)
ENGINE = ReplacingMergeTree(version)
PARTITION BY toYYYYMM(hour)
ORDER BY (ymref_id, hour, ccid)
""",
  f"""
CREATE TABLE IF NOT EXISTS {LOGS_SYNC_TABLE}
(
  ymref_id UInt64,
  visit_id UInt64,
  event_date Date,
  request_id String,
  payload_hash UInt64,
  reporting_comparable UInt8,
  published_at DateTime64(3, 'UTC')
)
ENGINE = ReplacingMergeTree(published_at)
PARTITION BY toYYYYMM(event_date)
ORDER BY (ymref_id, visit_id)
""",
  f"""
ALTER TABLE {LOGS_SYNC_TABLE}
  ADD COLUMN IF NOT EXISTS reporting_comparable UInt8 AFTER payload_hash
""",
  f"""
CREATE TABLE IF NOT EXISTS {LOG_REQUESTS_TABLE}
(
  ymref_id UInt64,
  event_date Date,
  request_id UInt64,
  status LowCardinality(String),
  version DateTime64(3, 'UTC')
)
ENGINE = ReplacingMergeTree(version)
PARTITION BY toYYYYMM(event_date)
ORDER BY (ymref_id, event_date)
""",
)

POST_CLICK_ACTION_VERSION = 'PostClickAction\t1.0'
REQUEST_ID_RE = re.compile(r'^[A-Za-z0-9_-]{22}\.\.$')
CCID_RE = re.compile(r'(?:^|[;&])ccid:(\d+)(?:$|[;&])')
LOG_FIELDS = (
  'ym:s:visitID',
  'ym:s:date',
  'ym:s:dateTimeUTC',
  'ym:s:bounce',
  'ym:s:visitDuration',
  'ym:s:pageViews',
  'ym:s:isNewUser',
  'ym:s:<attribution>UTMSource',
  'ym:s:<attribution>UTMTerm',
  'ym:s:<attribution>UTMContent',
)

TERMINAL_LOG_REQUEST_STATUSES = {
  'canceled',
  'cleaned',
  'cleaned_by_user',
  'cleaned_automatically_as_too_old',
  'processing_failed',
}


def parse_ccid(value):
  match = CCID_RE.search(value or '')
  return int(match.group(1)) if match else None


def decode_dimension(item, index):
  value = item['dimensions'][index].get('name')
  return value if value is not None else ''


def parse_bool(value):
  return str(value).lower() in ('1', 'true', 'yes')


def parse_reporting_metrics(metrics):
  visits = int(float(metrics[0]) + 0.5)
  return (
    visits,
    int(visits * float(metrics[1]) / 100.0 + 0.5),
    visits * float(metrics[2]),
    int(float(metrics[3]) + 0.5),
    int(float(metrics[4]) + 0.5),
  )


def request_chunk(request_id, chunks_count):
  raw = base64.urlsafe_b64decode(request_id[:-2] + '==')
  crc = 0
  for byte in raw:
    crc ^= byte << 24
    for _ in range(8):
      crc = ((crc << 1) ^ 0x04C11DB7) & 0xffffffff if crc & 0x80000000 else \
        (crc << 1) & 0xffffffff
  return crc % chunks_count


class YandexApi:
  def __init__(self, token, counter_id, timeout):
    self.counter_id = counter_id
    self.timeout = timeout
    self.headers = {'Authorization': 'OAuth ' + token}

  def reporting(self, params):
    response = requests.get(
      'https://api-metrika.yandex.net/stat/v1/data',
      params={'ids': self.counter_id, **params},
      headers=self.headers,
      timeout=self.timeout)
    response.raise_for_status()
    return response.json()

  def create_log_request(self, event_date, attribution):
    fields = ','.join(
      field.replace('<attribution>', attribution) for field in LOG_FIELDS)
    response = requests.post(
      f'https://api-metrika.yandex.net/management/v1/counter/{self.counter_id}/logrequests',
      params={
        'date1': event_date.isoformat(),
        'date2': event_date.isoformat(),
        'fields': fields,
        'source': 'visits',
        'attribution': attribution,
      },
      headers=self.headers,
      timeout=self.timeout)
    response.raise_for_status()
    return response.json()['log_request']

  def get_log_request(self, request_id):
    response = requests.get(
      f'https://api-metrika.yandex.net/management/v1/counter/{self.counter_id}/logrequest/{request_id}',
      headers=self.headers,
      timeout=self.timeout)
    response.raise_for_status()
    return response.json()['log_request']

  def download_log_part(self, request_id, part_number):
    response = requests.get(
      f'https://api-metrika.yandex.net/management/v1/counter/{self.counter_id}/logrequest/'
      f'{request_id}/part/{part_number}/download',
      headers=self.headers,
      timeout=self.timeout)
    response.raise_for_status()
    return response.text

  def clean_log_request(self, request_id):
    response = requests.post(
      f'https://api-metrika.yandex.net/management/v1/counter/{self.counter_id}/logrequest/'
      f'{request_id}/clean',
      headers=self.headers,
      timeout=self.timeout)
    response.raise_for_status()


class Application(Service):
  def __init__(self):
    super().__init__()
    self.args_parser.add_argument('--days', type=int)
    self.args_parser.add_argument('--sources', nargs='+')
    self.args_parser.add_argument('--attribution')
    self.args_parser.add_argument('--pg-dsn')
    self.args_parser.add_argument('--ch-host')
    self.args_parser.add_argument('--ch-port', type=int)
    self.args_parser.add_argument('--ch-database')
    self.args_parser.add_argument('--ch-user')
    self.args_parser.add_argument('--ch-pass')
    self.args_parser.add_argument('--ch-secure', action='store_true', default=None)
    self.args_parser.add_argument('--chunks-count', type=int)
    self.args_parser.add_argument('--request-timeout', type=float)
    self.pg = None
    self.ch = None

  def on_start(self):
    super().on_start()
    self.days = self.params.get('days', 4)
    sources = self.params.get('sources', ['genius', 'pml', 'pharmatic'])
    if isinstance(sources, str):
      sources = [source.strip() for source in sources.split(',')]
    self.sources = {source for source in sources if source}
    self.attribution = self.params.get('attribution', 'lastsign')
    self.chunks_count = self.params.get('chunks_count', 24)
    self.request_timeout = self.params.get('request_timeout', 60.0)
    if self.days < 2:
      raise ValueError('days must be at least 2')

    if not self.sources:
      raise ValueError('sources must not be empty')

    if self.chunks_count <= 0:
      raise ValueError('chunks_count must be positive')

    if self.out_dir is None:
      raise ValueError('out_dir is required')

    if self.tmp_dir is None:
      self.tmp_dir = os.path.join(self.out_dir, '.tmp')
      os.makedirs(self.tmp_dir, exist_ok=True)

    self.post_click_dir = os.path.join(self.out_dir, 'PostClickAction')
    os.makedirs(self.post_click_dir, exist_ok=True)
    self._connect()
    for statement in INTERNAL_CLICKHOUSE_DDL:
      self.ch.command(statement)

  def on_stop(self):
    if self.pg is not None:
      self.pg.close()
    if self.ch is not None:
      self.ch.close()
    super().on_stop()

  def on_timer(self):
    try:
      self._connect()
      with self.pg.cursor() as cursor:
        cursor.execute(
          "SELECT ymref_id, token, metrika_id FROM YandexMetrikaRef WHERE status = 'A'")
        references = tuple(cursor.fetchall())
    except Exception as ex:
      self.print_(0, f'Unable to load Yandex Metrika references: {ex}')
      self._reset_connections()
      return

    for ymref_id, token, counter_id in references:
      self.verify_running()
      try:
        api = YandexApi(token, counter_id, self.request_timeout)
        reporting = self._load_reporting(ymref_id, api)
        self._process_logs(ymref_id, api, reporting)
      except Exception as ex:
        self.print_(0, f'Unable to import Yandex Metrika reference {ymref_id}: {ex}')
        self._reset_connections()
        try:
          self._connect()
        except Exception as reconnect_ex:
          self.print_(0, f'Unable to reconnect import storage: {reconnect_ex}')
          return

  def _reset_connections(self):
    if self.pg is not None:
      try:
        self.pg.rollback()
        self.pg.close()
      except Exception:
        pass
      self.pg = None

    if self.ch is not None:
      try:
        self.ch.close()
      except Exception:
        pass
      self.ch = None

  def _connect(self):
    if self.pg is None or self.pg.closed:
      self.pg = psycopg2.connect(self.params['pg_dsn'])

    if self.ch is None:
      self.ch = clickhouse_connect.get_client(
        host=self.params['ch_host'],
        port=self.params.get('ch_port', 8123),
        database=self.params.get('ch_database', 'default'),
        username=self.params.get('ch_user', 'default'),
        password=self.params.get('ch_pass', ''),
        secure=self.params.get('ch_secure', False))

  def _reporting_page(self, api, source, date1, date2, offset):
    source_dimension = 'ym:s:<attribution>UTMSource'
    content_dimension = 'ym:s:<attribution>UTMContent'
    term_dimension = 'ym:s:<attribution>UTMTerm'
    return api.reporting({
      'metrics': (
        'ym:s:visits,ym:s:bounceRate,ym:s:avgVisitDurationSeconds,'
        'ym:s:pageviews,ym:s:newUsers'),
      'dimensions': 'ym:s:dateTime,ym:s:<attribution>UTMContent',
      'date1': date1.isoformat(),
      'date2': date2.isoformat(),
      'group': 'hour',
      'accuracy': 'full',
      'timezone': '+00:00',
      'filters': (
        f"{source_dimension}=='{source}' AND "
        f"{content_dimension}=@'ccid:' AND "
        f"{term_dimension}=~'^[A-Za-z0-9_-]{{22}}\\.\\.$'"),
      'attribution': self.attribution,
      'offset': offset,
      'limit': 100000,
    })

  def _load_reporting(self, ymref_id, api):
    today = datetime.datetime.now(datetime.timezone.utc).date()
    date1 = today - datetime.timedelta(days=self.days - 1)
    rows = {}
    unsampled = True

    for source in self.sources:
      offset = 1
      while True:
        result = self._reporting_page(api, source, date1, today, offset)
        sampled = bool(result.get('sampled', False))
        unsampled = unsampled and not sampled
        sample_share = float(result.get('sample_share', 1.0))
        data = result.get('data', [])
        for item in data:
          ccid = parse_ccid(decode_dimension(item, 1))
          if ccid is None:
            continue

          hour = datetime.datetime.fromisoformat(decode_dimension(item, 0))
          if hour.tzinfo is None:
            hour = hour.replace(tzinfo=datetime.timezone.utc)
          else:
            hour = hour.astimezone(datetime.timezone.utc)
          hour = hour.replace(minute=0, second=0, microsecond=0)
          metrics = parse_reporting_metrics(item['metrics'])
          key = (hour, ccid)
          current = rows.setdefault(key, [0, 0, 0.0, 0, 0, False, 1.0])
          current[0] += metrics[0]
          current[1] += metrics[1]
          current[2] += float(metrics[2])
          current[3] += metrics[3]
          current[4] += metrics[4]
          current[5] = current[5] or sampled
          current[6] = min(current[6], sample_share)

        if len(data) < 100000:
          break
        offset += len(data)

    old_rows = {
      (row[0], int(row[1])): tuple(row[2:])
      for row in self.ch.query(
        f"SELECT hour, ccid, visits, visits_with_bounce, session_time_sum, "
        f"page_views, new_user_visits, sampled, sample_share FROM {REPORTING_SYNC_TABLE} FINAL "
        "WHERE ymref_id = %(ymref_id)s AND hour >= %(date1)s",
        parameters={'ymref_id': ymref_id, 'date1': date1}).result_rows
    }
    changed = []
    version = datetime.datetime.now(datetime.timezone.utc)
    for (hour, ccid), values in rows.items():
      comparable = tuple(values)
      if old_rows.get((hour, ccid)) != comparable:
        changed.append((ymref_id, hour, ccid, *values, version))

    if changed:
      self._upsert_estimation(changed)
      self.ch.insert(
        REPORTING_SYNC_TABLE,
        changed,
        column_names=(
          'ymref_id',
          'hour',
          'ccid',
          'visits',
          'visits_with_bounce',
          'session_time_sum',
          'page_views',
          'new_user_visits',
          'sampled',
          'sample_share',
          'version'))

    return {'rows': rows, 'unsampled': unsampled}

  def _upsert_estimation(self, rows):
    statement = f"""
INSERT INTO {ESTIMATION_TABLE}
  (ymref_id, sdate, cc_id, visits, visits_with_bounce, session_time_sum,
   page_views, new_user_visits, sampled, sample_share, updated_at)
VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, now())
ON CONFLICT (ymref_id, sdate, cc_id) DO UPDATE SET
  visits = EXCLUDED.visits,
  visits_with_bounce = EXCLUDED.visits_with_bounce,
  session_time_sum = EXCLUDED.session_time_sum,
  page_views = EXCLUDED.page_views,
  new_user_visits = EXCLUDED.new_user_visits,
  sampled = EXCLUDED.sampled,
  sample_share = EXCLUDED.sample_share,
  updated_at = now()
"""
    with self.pg.cursor() as cursor:
      cursor.executemany(statement, [
        (ymref_id, row[1], row[2], *row[3:10])
        for row in rows
      ])
    self.pg.commit()

  def _process_logs(self, ymref_id, api, reporting):
    today = datetime.datetime.now(datetime.timezone.utc).date()
    for age in range(1, self.days):
      event_date = today - datetime.timedelta(days=age)
      request_state = self._log_request_state(ymref_id, event_date)
      if request_state is None or request_state[1] in TERMINAL_LOG_REQUEST_STATUSES:
        log_request = api.create_log_request(event_date, self.attribution)
        request_id = int(log_request['request_id'])
        self._save_log_request(ymref_id, event_date, request_id, log_request['status'])
      else:
        request_id = request_state[0]

      log_request = api.get_log_request(request_id)
      status = log_request['status']
      self._save_log_request(ymref_id, event_date, request_id, status)
      if status != 'processed':
        continue

      records = []
      for part in log_request.get('parts', []):
        text = api.download_log_part(request_id, int(part['part_number']))
        records.extend(self._parse_log_part(ymref_id, text, event_date))

      published = self._publish_post_click_actions(ymref_id, event_date, records)
      if published:
        self.ch.insert(
          LOGS_SYNC_TABLE,
          published,
          column_names=(
            'ymref_id',
            'visit_id',
            'event_date',
            'request_id',
            'payload_hash',
            'reporting_comparable',
            'published_at'))

      api.clean_log_request(request_id)
      self._save_log_request(ymref_id, event_date, request_id, 'cleaned')
      self._update_day_status(ymref_id, event_date, reporting)

  def _parse_log_part(self, ymref_id, text, event_date):
    records = []
    for row in csv.DictReader(text.splitlines(), delimiter='\t'):
      source = self._field(row, 'UTMSource')
      if source not in self.sources:
        continue

      request_id = urllib.parse.unquote_plus(self._field(row, 'UTMTerm'))
      if not REQUEST_ID_RE.fullmatch(request_id):
        continue

      visit_id = int(self._field(row, 'visitID'))
      event_time = datetime.datetime.fromisoformat(self._field(row, 'dateTimeUTC'))
      if event_time.tzinfo is None:
        # Logs API documents dateTimeUTC as a fixed UTC+3 value.
        event_time = event_time.replace(
          tzinfo=datetime.timezone(datetime.timedelta(hours=3)))
      event_time = event_time.astimezone(datetime.timezone.utc)
      reporting_comparable = parse_ccid(self._field(row, 'UTMContent')) is not None
      payload = json.dumps({
        'landing_bounced': parse_bool(self._field(row, 'bounce')),
        'landing_session_time': int(self._field(row, 'visitDuration')),
        'landing_page_views': int(self._field(row, 'pageViews')),
        'landing_is_new_user': parse_bool(self._field(row, 'isNewUser')),
        'yandex_ref_id': ymref_id,
        'yandex_event_date': event_date.isoformat(),
        'yandex_reporting_comparable': reporting_comparable,
      }, separators=(',', ':'), sort_keys=True)
      records.append(
        (visit_id, event_date, event_time, request_id, payload, reporting_comparable))
    return records

  @staticmethod
  def _field(row, suffix):
    for name, value in row.items():
      if name.endswith(suffix):
        return value or ''
    raise KeyError('Yandex Logs API field is missing: ' + suffix)

  def _publish_post_click_actions(self, ymref_id, event_date, records):
    processed = {
      int(row[0])
      for row in self.ch.query(
        f"SELECT visit_id FROM {LOGS_SYNC_TABLE} FINAL "
        "WHERE ymref_id = %(ymref_id)s AND event_date = %(event_date)s",
        parameters={'ymref_id': ymref_id, 'event_date': event_date}).result_rows
    }
    records_by_visit = {record[0]: record for record in records}
    new_records = [
      record for visit_id, record in records_by_visit.items()
      if visit_id not in processed
    ]
    if not new_records:
      return []

    with Context(self, out_dir=self.post_click_dir) as context:
      for _, _, event_time, request_id, payload, _ in new_records:
        chunk = request_chunk(request_id, self.chunks_count)
        writer = context.files.get_line_writer(
          key=chunk,
          name=lambda chunk=chunk:
            f'PostClickAction.{context.fname_seed}.{self.chunks_count}.{chunk}')
        if writer.first:
          writer.write_line(POST_CLICK_ACTION_VERSION)
        writer.write_line(
          event_time.strftime('%Y-%m-%d_%H:%M:%S') + '\t' + request_id +
          '\tlanding\t' + payload)

    published_at = datetime.datetime.now(datetime.timezone.utc)
    return [
      (
        ymref_id,
        visit_id,
        date,
        request_id,
        int.from_bytes(hashlib.blake2b(payload.encode(), digest_size=8).digest(), 'big'),
        reporting_comparable,
        published_at)
      for visit_id, date, _, request_id, payload, reporting_comparable in new_records
    ]

  def _log_request_state(self, ymref_id, event_date):
    rows = self.ch.query(
      f"SELECT request_id, status FROM {LOG_REQUESTS_TABLE} FINAL "
      "WHERE ymref_id = %(ymref_id)s AND event_date = %(event_date)s",
      parameters={'ymref_id': ymref_id, 'event_date': event_date}).result_rows
    return (int(rows[0][0]), rows[0][1]) if rows else None

  def _save_log_request(self, ymref_id, event_date, request_id, status):
    self.ch.insert(
      LOG_REQUESTS_TABLE,
      [(ymref_id, event_date, request_id, status,
        datetime.datetime.now(datetime.timezone.utc))],
      column_names=('ymref_id', 'event_date', 'request_id', 'status', 'version'))

  def _update_day_status(self, ymref_id, event_date, reporting):
    reporting_visits = sum(
      values[0] for (hour, _), values in reporting['rows'].items()
      if hour.date() == event_date)
    logs_visits = int(self.ch.query(
      f"SELECT countIf(reporting_comparable) FROM {LOGS_SYNC_TABLE} FINAL "
      "WHERE ymref_id = %(ymref_id)s AND event_date = %(event_date)s",
      parameters={'ymref_id': ymref_id, 'event_date': event_date}).result_rows[0][0])
    equal = reporting['unsampled'] and reporting_visits == logs_visits

    with self.pg.cursor() as cursor:
      cursor.execute(
        f"SELECT equal_runs, state FROM {DAY_STATUS_TABLE} "
        "WHERE ymref_id = %s AND sdate = %s FOR UPDATE",
        (ymref_id, event_date))
      previous = cursor.fetchone()
      equal_runs = (previous[0] if previous and equal else 0) + (1 if equal else 0)
      state = 'READY' if equal_runs >= 2 else 'ESTIMATED'
      if previous and previous[1] == 'EXACT':
        state = 'EXACT'

      if state == 'READY':
        cursor.execute(
          f"SELECT visits FROM {EXACT_PROGRESS_TABLE} "
          "WHERE ymref_id = %s AND sdate = %s",
          (ymref_id, event_date))
        progress = cursor.fetchone()
        if (progress[0] if progress else 0) >= logs_visits:
          state = 'EXACT'

      cursor.execute(f"""
INSERT INTO {DAY_STATUS_TABLE}
  (ymref_id, sdate, state, reporting_visits, logs_visits, equal_runs, updated_at)
VALUES (%s, %s, %s, %s, %s, %s, now())
ON CONFLICT (ymref_id, sdate) DO UPDATE SET
  state = EXCLUDED.state,
  reporting_visits = EXCLUDED.reporting_visits,
  logs_visits = EXCLUDED.logs_visits,
  equal_runs = EXCLUDED.equal_runs,
  updated_at = now()
""", (ymref_id, event_date, state, reporting_visits, logs_visits, equal_runs))
    self.pg.commit()


if __name__ == '__main__':
  Application().run()
