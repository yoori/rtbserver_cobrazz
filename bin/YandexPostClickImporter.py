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
from ServiceUtilsPy.Service import Service, StopService


REPORTING_SYNC_TABLE = 'YandexPostClickReportingSync'
LOGS_SYNC_TABLE = 'YandexPostClickLogsSync'
LOG_REQUESTS_TABLE = 'YandexPostClickLogRequests'
UPSERT_ESTIMATION_FUNCTION = 'adserver.upsert_yandex_metrika_post_click_estimation'
UPDATE_IMPORT_STATUS_FUNCTION = 'adserver.update_yandex_metrika_import_status'
ADVANCE_LOGS_WATERMARK_FUNCTION = 'adserver.advance_yandex_metrika_logs_watermark'
YANDEX_METRIKA_SOURCE = 'Yandex Metrika'

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
REPORTING_PAGE_SIZE = 100000
LOGS_READY_DELAY = datetime.timedelta(hours=12)
LOGS_TIMEZONE = datetime.timezone(datetime.timedelta(hours=3))
USER_ID_DISTRIBUTION_MOD = 1000
REQUEST_ID_PATTERN = r'[A-Za-z0-9_-]{22}[.][.]'
USER_ID_PATTERN = rf'({REQUEST_ID_PATTERN})?'
COLON_PATTERN = r'(:|%3[Aa]|%253[Aa])'
TERM_SEPARATOR_PATTERN = r'(;|%3[Bb]|%253[Bb])'
REQUEST_ID_RE = re.compile(rf'^{REQUEST_ID_PATTERN}$')
METRIKA_TERM_RE = re.compile(
  rf'^r:(?P<request_id>{REQUEST_ID_PATTERN});'
  rf'(?:h:(?P<distribution_hash>[0-9]+);)?'
  rf'u1:(?P<user_id>{USER_ID_PATTERN});u2:{USER_ID_PATTERN}$')
REPORTING_TERM_PATTERN = (
  rf'^({REQUEST_ID_PATTERN}|r{COLON_PATTERN}{REQUEST_ID_PATTERN}'
  rf'({TERM_SEPARATOR_PATTERN}h{COLON_PATTERN}[0-9]+)?'
  rf'{TERM_SEPARATOR_PATTERN}u1{COLON_PATTERN}{USER_ID_PATTERN}'
  rf'{TERM_SEPARATOR_PATTERN}u2{COLON_PATTERN}{USER_ID_PATTERN})$')
CCID_RE = re.compile(r'(?:^|[;&])ccid:(\d+)(?:$|[;&])')
ATTRIBUTIONS = frozenset((
  'first',
  'last',
  'lastsign',
  'last_yandex_direct_click',
  'cross_device_first',
  'cross_device_last',
  'cross_device_last_significant',
  'cross_device_last_yandex_direct_click',
  'automatic',
))
LOG_FIELDS = (
  'ym:s:visitID',
  'ym:s:date',
  'ym:s:dateTimeUTC',
  'ym:s:bounce',
  'ym:s:visitDuration',
  'ym:s:pageViews',
  'ym:s:isNewUser',
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


def parse_metrika_term(value):
  value = value or ''
  for _ in range(2):
    decoded_value = urllib.parse.unquote_plus(value)
    if decoded_value == value:
      break
    value = decoded_value

  match = METRIKA_TERM_RE.fullmatch(value)
  if match:
    distribution_hash = match.group('distribution_hash')
    if distribution_hash is None and match.group('user_id'):
      distribution_hash = user_id_distribution_hash(match.group('user_id'))
    elif distribution_hash is not None:
      distribution_hash = int(distribution_hash)
    return match.group('request_id'), distribution_hash
  return (value, None) if REQUEST_ID_RE.fullmatch(value) else None


def parse_metrika_request_id(value):
  term = parse_metrika_term(value)
  return term[0] if term else None


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


def format_timezone_offset(offset_minutes):
  offset_minutes = int(offset_minutes)
  if abs(offset_minutes) >= 24 * 60:
    raise ValueError('counter timezone offset must be less than 24 hours')
  sign = '-' if offset_minutes < 0 else '+'
  hours, minutes = divmod(abs(offset_minutes), 60)
  return f'{sign}{hours:02d}:{minutes:02d}'


def parse_log_event_time(value):
  event_time = datetime.datetime.fromisoformat(value)
  if event_time.tzinfo is None:
    # Logs API documents dateTimeUTC as a fixed UTC+3 value.
    event_time = event_time.replace(tzinfo=LOGS_TIMEZONE)
  return event_time.astimezone(datetime.timezone.utc)


def uuid_distribution_hash(value):
  raw = base64.urlsafe_b64decode(value[:-2] + '==')
  crc = 0
  for byte in raw:
    crc ^= byte << 24
    for _ in range(8):
      crc = ((crc << 1) ^ 0x04C11DB7) & 0xffffffff if crc & 0x80000000 else \
        (crc << 1) & 0xffffffff
  return crc


def user_id_distribution_hash(user_id):
  return uuid_distribution_hash(user_id) % USER_ID_DISTRIBUTION_MOD


def request_chunk(request_id, chunks_count):
  return uuid_distribution_hash(request_id) % chunks_count


class YandexApi:
  def __init__(self, token, counter_id, timeout):
    self.counter_id = counter_id
    self.timeout = timeout
    self.headers = {'Authorization': 'OAuth ' + token}
    self._reporting_timezone = None
    self._timezone = None

  def _load_timezone(self):
    if self._timezone is None:
      response = requests.get(
        f'https://api-metrika.yandex.net/management/v1/counter/{self.counter_id}',
        headers=self.headers,
        timeout=self.timeout)
      response.raise_for_status()
      offset_minutes = int(response.json()['counter']['time_zone_offset'])
      self._reporting_timezone = format_timezone_offset(offset_minutes)
      self._timezone = datetime.timezone(datetime.timedelta(minutes=offset_minutes))

  def reporting_timezone(self):
    self._load_timezone()
    return self._reporting_timezone

  def timezone(self):
    self._load_timezone()
    return self._timezone

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
        'attribution': attribution.upper(),
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
    self.args_parser.add_argument('--attribution', choices=sorted(ATTRIBUTIONS))
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
    self.attribution = self.params.get('attribution', 'lastsign').lower()
    self.chunks_count = self.params.get('chunks_count', 24)
    self.request_timeout = self.params.get('request_timeout', 60.0)
    if self.days < 2:
      raise ValueError('days must be at least 2')

    if self.attribution not in ATTRIBUTIONS:
      raise ValueError(f'unsupported attribution: {self.attribution}')

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
          "SELECT ymref_id, token, metrika_id FROM YandexMetrikaRef "
          "WHERE status = 'A' AND source = %s",
          (YANDEX_METRIKA_SOURCE,))
        references = tuple(cursor.fetchall())
      self.pg.commit()
    except Exception as ex:
      self.print_(0, f'Unable to load Yandex Metrika references: {ex}')
      self._reset_connections()
      return

    for ymref_id, token, counter_id in references:
      self.verify_running()
      try:
        self._mark_fetch_time(ymref_id, False)
        api = YandexApi(token, counter_id, self.request_timeout)
        reporting = self._load_reporting(ymref_id, api)
        self._process_logs(ymref_id, api, reporting)
        self._mark_fetch_time(ymref_id, True)
      except StopService:
        raise
      except Exception as ex:
        self.print_(0, f'Unable to import Yandex Metrika reference {ymref_id}: {ex}')
        self._reset_connections()
        try:
          self._connect()
        except Exception as reconnect_ex:
          self.print_(0, f'Unable to reconnect import storage: {reconnect_ex}')
          return

  def _mark_fetch_time(self, ymref_id, success):
    if not isinstance(success, bool):
      raise ValueError('success must be boolean')

    with self.pg.cursor() as cursor:
      cursor.execute(
        'SELECT adserver.update_yandex_metrika_ref_fetch_time(%s, %s)',
        (ymref_id, success))
    self.pg.commit()

  def _reset_connections(self):
    if self.pg is not None:
      pg = self.pg
      self.pg = None
      try:
        pg.rollback()
      except Exception:
        pass

      try:
        pg.close()
      except Exception:
        pass

    if self.ch is not None:
      ch = self.ch
      self.ch = None
      try:
        ch.close()
      except Exception:
        pass

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

  def _reporting_page(self, api, date1, date2, offset):
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
      'timezone': api.reporting_timezone(),
      'filters': (
        f"{content_dimension}=@'ccid:' AND "
        f"{term_dimension}=~'{REPORTING_TERM_PATTERN}'"),
      'attribution': self.attribution,
      'offset': offset,
      'limit': REPORTING_PAGE_SIZE,
    })

  def _load_reporting(self, ymref_id, api, date1=None, date2=None):
    timezone = api.timezone()
    today = datetime.datetime.now(timezone).date()
    if date1 is None:
      date1 = today - datetime.timedelta(days=self.days - 1)
    if date2 is None:
      date2 = today
    if date1 > date2:
      raise ValueError('reporting date1 must not be after date2')

    rows = {}
    unsampled = True

    offset = 1
    while True:
      result = self._reporting_page(api, date1, date2, offset)
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
          hour = hour.replace(tzinfo=timezone)
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

      total_rows = result.get('total_rows')
      if total_rows is not None and offset - 1 + len(data) >= int(total_rows):
        break

      if not data or len(data) < REPORTING_PAGE_SIZE:
        break
      offset += len(data)

    range_start = datetime.datetime.combine(date1, datetime.time(), timezone)
    range_end = datetime.datetime.combine(
      date2 + datetime.timedelta(days=1),
      datetime.time(),
      timezone)
    old_rows = {}
    for row in self.ch.query(
        f"SELECT hour, ccid, visits, visits_with_bounce, session_time_sum, "
        f"page_views, new_user_visits, sampled, sample_share FROM {REPORTING_SYNC_TABLE} FINAL "
        "WHERE ymref_id = %(ymref_id)s AND hour >= %(range_start)s "
        "AND hour < %(range_end)s",
        parameters={
          'ymref_id': ymref_id,
          'range_start': range_start.astimezone(datetime.timezone.utc),
          'range_end': range_end.astimezone(datetime.timezone.utc),
        }).result_rows:
      hour = row[0]
      if hour.tzinfo is None:
        hour = hour.replace(tzinfo=datetime.timezone.utc)
      else:
        hour = hour.astimezone(datetime.timezone.utc)
      old_rows[(hour, int(row[1]))] = tuple(row[2:])
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

    return {
      'rows': rows,
      'unsampled': unsampled,
      'date1': date1,
      'date2': date2,
      'timezone': timezone,
    }

  def _upsert_estimation(self, rows):
    statement = f"SELECT {UPSERT_ESTIMATION_FUNCTION}(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"
    with self.pg.cursor() as cursor:
      cursor.executemany(statement, [tuple(row[:10]) for row in rows])
    self.pg.commit()

  def _process_logs(self, ymref_id, api, reporting):
    handled_dates = set()
    for event_date, request_id in self._pending_log_requests(ymref_id):
      self.verify_running()
      self._process_log_request(ymref_id, event_date, request_id, api, reporting)
      handled_dates.add(event_date)

    today = datetime.datetime.now(api.timezone()).date()
    for age in range(1, self.days):
      self.verify_running()
      event_date = today - datetime.timedelta(days=age)
      if event_date in handled_dates:
        continue

      request_state = self._log_request_state(ymref_id, event_date)
      if request_state is None or request_state[1] in TERMINAL_LOG_REQUEST_STATUSES:
        log_request = api.create_log_request(event_date, self.attribution)
        request_id = int(log_request['request_id'])
        self._save_log_request(ymref_id, event_date, request_id, log_request['status'])
      else:
        request_id = request_state[0]

      self._process_log_request(ymref_id, event_date, request_id, api, reporting)

  def _process_log_request(self, ymref_id, event_date, request_id, api, reporting):
    log_request = api.get_log_request(request_id)
    status = log_request['status']
    self._save_log_request(ymref_id, event_date, request_id, status)
    if status != 'processed':
      return

    date1 = reporting.get('date1')
    date2 = reporting.get('date2')
    if date1 is not None and date2 is not None and not date1 <= event_date <= date2:
      reporting = self._load_reporting(ymref_id, api, event_date, event_date)

    records = []
    watermark = None
    for part in log_request.get('parts', []):
      self.verify_running()
      text = api.download_log_part(request_id, int(part['part_number']))
      part_records, part_watermark = self._parse_log_part_with_watermark(ymref_id, text, event_date)
      records.extend(part_records)
      if part_watermark is not None and (watermark is None or part_watermark > watermark):
        watermark = part_watermark

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
    if watermark is not None:
      self._advance_logs_watermark(ymref_id, watermark)
    self._update_import_status(ymref_id, event_date, reporting)

  def _pending_log_requests(self, ymref_id):
    rows = self.ch.query(
      f"SELECT event_date, request_id, status FROM {LOG_REQUESTS_TABLE} FINAL "
      "WHERE ymref_id = %(ymref_id)s ORDER BY event_date",
      parameters={'ymref_id': ymref_id}).result_rows
    return [
      (row[0], int(row[1]))
      for row in rows
      if row[2] not in TERMINAL_LOG_REQUEST_STATUSES
    ]

  def _parse_log_part(self, ymref_id, text, event_date):
    return self._parse_log_part_with_watermark(ymref_id, text, event_date)[0]

  def _parse_log_part_with_watermark(self, ymref_id, text, event_date):
    records = []
    watermark = None
    for row in csv.DictReader(text.splitlines(), delimiter='\t'):
      event_time = parse_log_event_time(self._field(row, 'dateTimeUTC'))
      if watermark is None or event_time > watermark:
        watermark = event_time

      metrika_term = parse_metrika_term(self._field(row, 'UTMTerm'))
      if metrika_term is None:
        continue
      request_id, distribution_hash = metrika_term

      visit_id = int(self._field(row, 'visitID'))
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
        (
          visit_id,
          event_date,
          event_time,
          request_id,
          distribution_hash,
          payload,
          reporting_comparable,
        ))
    return records, watermark

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
      writers = {}
      for _, _, event_time, request_id, distribution_hash, payload, _ in new_records:
        chunk = distribution_hash % self.chunks_count if distribution_hash is not None else \
          request_chunk(request_id, self.chunks_count)
        writer = writers.get(chunk)
        if writer is None:
          writer = context.files.get_line_writer(
            key=chunk,
            name=lambda chunk=chunk:
              f'PostClickAction.{context.fname_seed}.{self.chunks_count}.{chunk}')
          writers[chunk] = writer
        if writer.first:
          writer.write_line(POST_CLICK_ACTION_VERSION)
        writer.write_line(
          event_time.strftime('%Y-%m-%d_%H:%M:%S') + '\t' + request_id +
          '\tlanding\t' + payload)
      for writer in writers.values():
        writer.write('\n')

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
      for visit_id, date, _, request_id, _, payload, reporting_comparable in new_records
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

  def _advance_logs_watermark(self, ymref_id, event_time):
    ready_before_date = (
      event_time.astimezone(LOGS_TIMEZONE) - LOGS_READY_DELAY).date()
    with self.pg.cursor() as cursor:
      cursor.execute(
        f"SELECT {ADVANCE_LOGS_WATERMARK_FUNCTION}(%s, %s)",
        (ymref_id, ready_before_date))
    self.pg.commit()

  def _update_import_status(self, ymref_id, event_date, reporting):
    timezone = reporting.get('timezone', datetime.timezone.utc)
    reporting_visits = sum(
      values[0] for (hour, _), values in reporting['rows'].items()
      if hour.astimezone(timezone).date() == event_date)
    logs_visits = int(self.ch.query(
      f"SELECT countIf(reporting_comparable) FROM {LOGS_SYNC_TABLE} FINAL "
      "WHERE ymref_id = %(ymref_id)s AND event_date = %(event_date)s",
      parameters={'ymref_id': ymref_id, 'event_date': event_date}).result_rows[0][0])
    with self.pg.cursor() as cursor:
      cursor.execute(
        f"SELECT {UPDATE_IMPORT_STATUS_FUNCTION}(%s, %s, %s, %s, %s)",
        (
          ymref_id,
          event_date,
          reporting_visits,
          logs_visits,
          reporting['unsampled']))
    self.pg.commit()


if __name__ == '__main__':
  Application().run()
