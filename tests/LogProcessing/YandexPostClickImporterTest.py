#!/usr/bin/env python3.12

import datetime
import importlib.util
import json
import pathlib
import sys
import tempfile
import types
import unittest


REPOSITORY_ROOT = pathlib.Path(__file__).resolve().parents[2]


def load_importer():
  sys.path.insert(0, str(REPOSITORY_ROOT / 'lib'))
  sys.modules.setdefault('requests', types.ModuleType('requests'))
  module_path = REPOSITORY_ROOT / 'bin' / 'YandexPostClickImporter.py'
  spec = importlib.util.spec_from_file_location('YandexPostClickImporter', module_path)
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


IMPORTER = load_importer()
REQUEST_ID = 'AAAAAAAAAAAAAAAAAAAAAA..'


def log_part(request_id=REQUEST_ID, content='ccid:2527264'):
  header = (
    'ym:s:visitID\tym:s:date\tym:s:dateTimeUTC\tym:s:bounce\t'
    'ym:s:visitDuration\tym:s:pageViews\tym:s:isNewUser\t'
    'ym:s:lastsignUTMTerm\tym:s:lastsignUTMContent')
  row = (
    f'123\t2026-09-10\t2026-09-10T12:34:56\t1\t42\t3\t0\t'
    f'{request_id}\t{content}')
  return header + '\n' + row + '\n'


class YandexPostClickImporterTest(unittest.TestCase):
  def setUp(self):
    self.application = object.__new__(IMPORTER.Application)
    self.application.attribution = 'lastsign'
    self.event_date = datetime.date(2026, 9, 10)

  def test_reporting_api_contract(self):
    class Api:
      def reporting(self, params):
        return params

    params = self.application._reporting_page(
      Api(),
      self.event_date,
      self.event_date,
      1)

    self.assertEqual(
      params['metrics'],
      'ym:s:visits,ym:s:bounceRate,ym:s:avgVisitDurationSeconds,'
      'ym:s:pageviews,ym:s:newUsers')
    self.assertEqual(
      params['dimensions'],
      'ym:s:dateTime,ym:s:<attribution>UTMContent')
    self.assertEqual(params['accuracy'], 'full')
    self.assertEqual(params['attribution'], 'lastsign')
    self.assertNotIn('UTMSource', params['filters'])
    self.assertIn("ym:s:<attribution>UTMContent=@'ccid:'", params['filters'])
    self.assertIn("ym:s:<attribution>UTMTerm=~", params['filters'])

  def test_logs_api_contract(self):
    calls = []

    class Response:
      def raise_for_status(self):
        pass

      def json(self):
        return {'log_request': {'request_id': 11, 'status': 'created'}}

    def post(url, params, headers, timeout):
      calls.append((url, params, headers, timeout))
      return Response()

    old_post = getattr(IMPORTER.requests, 'post', None)
    IMPORTER.requests.post = post
    try:
      api = IMPORTER.YandexApi('secret', 123, 17)
      result = api.create_log_request(self.event_date, 'lastsign')
    finally:
      if old_post is None:
        del IMPORTER.requests.post
      else:
        IMPORTER.requests.post = old_post

    self.assertEqual(result['request_id'], 11)
    self.assertEqual(len(calls), 1)
    url, params, headers, timeout = calls[0]
    self.assertEqual(
      url,
      'https://api-metrika.yandex.net/management/v1/counter/123/logrequests')
    self.assertEqual(params['source'], 'visits')
    self.assertEqual(params['attribution'], 'LASTSIGN')
    self.assertNotIn('UTMSource', params['fields'])
    self.assertIn('ym:s:lastsignUTMTerm', params['fields'])
    self.assertIn('ym:s:lastsignUTMContent', params['fields'])
    self.assertEqual(headers, {'Authorization': 'OAuth secret'})
    self.assertEqual(timeout, 17)

  def test_parse_ccid(self):
    self.assertEqual(IMPORTER.parse_ccid('ccid:2527264'), 2527264)
    self.assertEqual(IMPORTER.parse_ccid('key:value;ccid:17&other:value'), 17)
    self.assertIsNone(IMPORTER.parse_ccid('prefix-ccid:17'))
    self.assertIsNone(IMPORTER.parse_ccid(''))

  def test_request_chunk(self):
    self.assertEqual(IMPORTER.request_chunk(REQUEST_ID, 24), 0)

  def test_parse_reporting_metrics(self):
    self.assertEqual(
      IMPORTER.parse_reporting_metrics([10, 25.0, 4.5, 17, 3]),
      (10, 3, 45.0, 17, 3))

  def test_load_reporting_paginates_and_aggregates_by_ccid(self):
    class Api:
      def __init__(self):
        self.offsets = []

      def reporting(self, params):
        self.offsets.append(params['offset'])
        if params['offset'] == 1:
          return {
            'sampled': False,
            'sample_share': 1.0,
            'total_rows': 3,
            'data': [
              {
                'dimensions': [
                  {'name': '2026-09-10T12:00:00'},
                  {'name': 'ccid:1'},
                ],
                'metrics': [2, 50.0, 10.0, 3, 1],
              },
              {
                'dimensions': [
                  {'name': '2026-09-10T12:00:00'},
                  {'name': 'ccid:1;customer:value'},
                ],
                'metrics': [1, 0.0, 4.0, 1, 0],
              },
            ],
          }

        return {
          'sampled': True,
          'sample_share': 0.5,
          'total_rows': 3,
          'data': [{
            'dimensions': [
              {'name': '2026-09-10T13:00:00'},
              {'name': 'ccid:2'},
            ],
            'metrics': [4, 25.0, 2.0, 5, 2],
          }],
        }

    class Ch:
      def __init__(self):
        self.inserted = []

      def query(self, query, parameters):
        return types.SimpleNamespace(result_rows=[])

      def insert(self, table, rows, column_names):
        self.inserted.append((table, rows, column_names))

    api = Api()
    self.application.days = 2
    self.application.ch = Ch()
    upserted = []
    self.application._upsert_estimation = lambda rows: upserted.extend(rows)
    old_page_size = IMPORTER.REPORTING_PAGE_SIZE
    IMPORTER.REPORTING_PAGE_SIZE = 2
    try:
      result = self.application._load_reporting(17, api)
    finally:
      IMPORTER.REPORTING_PAGE_SIZE = old_page_size

    hour1 = datetime.datetime(2026, 9, 10, 12, tzinfo=datetime.timezone.utc)
    hour2 = datetime.datetime(2026, 9, 10, 13, tzinfo=datetime.timezone.utc)
    self.assertEqual(api.offsets, [1, 3])
    self.assertEqual(result['rows'][(hour1, 1)][:5], [3, 1, 24.0, 4, 1])
    self.assertEqual(result['rows'][(hour2, 2)][:5], [4, 1, 8.0, 5, 2])
    self.assertFalse(result['unsampled'])
    self.assertEqual(len(upserted), 2)
    self.assertEqual(len(self.application.ch.inserted), 1)

  def test_update_import_status_uses_postgres_function(self):
    calls = []

    class Cursor:
      def __enter__(self):
        return self

      def __exit__(self, exc_type, exc_value, traceback):
        return False

      def execute(self, query, parameters):
        calls.append((query, parameters))

    class Pg:
      def __init__(self):
        self.commits = 0

      def cursor(self):
        return Cursor()

      def commit(self):
        self.commits += 1

    class Ch:
      def query(self, query, parameters):
        self.query_text = query
        self.parameters = parameters
        return types.SimpleNamespace(result_rows=[(7,)])

    self.application.pg = Pg()
    self.application.ch = Ch()
    reporting = {
      'rows': {
        (datetime.datetime(2026, 9, 10, 10), 1): (3, 0, 0, 0, 0),
        (datetime.datetime(2026, 9, 10, 11), 2): (5, 0, 0, 0, 0),
        (datetime.datetime(2026, 9, 9, 11), 2): (100, 0, 0, 0, 0),
      },
      'unsampled': True,
    }

    self.application._update_import_status(17, self.event_date, reporting)

    self.assertEqual(calls, [(
      'SELECT adserver.update_yandex_metrika_import_status(%s, %s, %s, %s, %s)',
      (17, self.event_date, 8, 7, True),
    )])
    self.assertEqual(self.application.pg.commits, 1)

  def test_upsert_estimation_uses_postgres_function(self):
    calls = []

    class Cursor:
      def __enter__(self):
        return self

      def __exit__(self, exc_type, exc_value, traceback):
        return False

      def executemany(self, query, parameters):
        calls.append((query, parameters))

    class Pg:
      def __init__(self):
        self.commits = 0

      def cursor(self):
        return Cursor()

      def commit(self):
        self.commits += 1

    self.application.pg = Pg()
    hour = datetime.datetime(2026, 9, 10, 12, tzinfo=datetime.timezone.utc)
    version = datetime.datetime(2026, 9, 11, 1, tzinfo=datetime.timezone.utc)

    self.application._upsert_estimation([
      (17, hour, 2527264, 10, 3, 45.5, 17, 4, False, 1.0, version),
    ])

    self.assertEqual(calls, [(
      'SELECT adserver.upsert_yandex_metrika_post_click_estimation('
      '%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)',
      [(17, hour, 2527264, 10, 3, 45.5, 17, 4, False, 1.0)],
    )])
    self.assertEqual(self.application.pg.commits, 1)

  def test_mark_fetch_time_updates_requested_column(self):
    calls = []

    class Cursor:
      def __enter__(self):
        return self

      def __exit__(self, exc_type, exc_value, traceback):
        return False

      def execute(self, query, parameters):
        calls.append((query, parameters))

    class Pg:
      def __init__(self):
        self.commits = 0

      def cursor(self):
        return Cursor()

      def commit(self):
        self.commits += 1

    self.application.pg = Pg()
    self.application._mark_fetch_time(17, 'last_fetch_try_time')
    self.application._mark_fetch_time(17, 'last_success_fetch_time')

    self.assertEqual(calls, [
      (
        'UPDATE YandexMetrikaRef SET last_fetch_try_time = now() WHERE ymref_id = %s',
        (17,),
      ),
      (
        'UPDATE YandexMetrikaRef SET last_success_fetch_time = now() WHERE ymref_id = %s',
        (17,),
      ),
    ])
    self.assertEqual(self.application.pg.commits, 2)

  def test_mark_fetch_time_rejects_unknown_column(self):
    with self.assertRaises(ValueError):
      self.application._mark_fetch_time(17, 'token')

  def test_on_timer_filters_source_and_marks_try_then_success(self):
    calls = []

    class Cursor:
      def __enter__(self):
        return self

      def __exit__(self, exc_type, exc_value, traceback):
        return False

      def execute(self, query, parameters):
        calls.append(('select', query, parameters))

      def fetchall(self):
        return [(17, 'secret', 123)]

    class Pg:
      def __init__(self):
        self.commits = 0

      def cursor(self):
        return Cursor()

      def commit(self):
        self.commits += 1

    self.application.pg = Pg()
    self.application.request_timeout = 11
    self.application._connect = lambda: None
    self.application.verify_running = lambda: None
    self.application._mark_fetch_time = (
      lambda ymref_id, column: calls.append(('mark', ymref_id, column)))
    self.application._load_reporting = (
      lambda ymref_id, api: calls.append(('reporting', ymref_id, api.counter_id)) or {})
    self.application._process_logs = (
      lambda ymref_id, api, reporting: calls.append(('logs', ymref_id, api.counter_id)))

    self.application.on_timer()

    self.assertEqual(
      calls[0],
      (
        'select',
        "SELECT ymref_id, token, metrika_id FROM YandexMetrikaRef "
        "WHERE status = 'A' AND source = %s",
        (IMPORTER.YANDEX_METRIKA_SOURCE,),
      ))
    self.assertEqual(calls[1:], [
      ('mark', 17, 'last_fetch_try_time'),
      ('reporting', 17, 123),
      ('logs', 17, 123),
      ('mark', 17, 'last_success_fetch_time'),
    ])
    self.assertEqual(self.application.pg.commits, 1)

  def test_on_timer_does_not_mark_success_after_failed_import(self):
    calls = []

    class Cursor:
      def __enter__(self):
        return self

      def __exit__(self, exc_type, exc_value, traceback):
        return False

      def execute(self, query, parameters):
        pass

      def fetchall(self):
        return [(17, 'secret', 123)]

    class Pg:
      def cursor(self):
        return Cursor()

      def commit(self):
        pass

    def fail_reporting(ymref_id, api):
      calls.append(('reporting', ymref_id))
      raise RuntimeError('test failure')

    self.application.pg = Pg()
    self.application.request_timeout = 11
    self.application._connect = lambda: calls.append(('connect',))
    self.application._reset_connections = lambda: calls.append(('reset',))
    self.application.verify_running = lambda: None
    self.application.print_ = lambda level, message: calls.append(('error', level))
    self.application._mark_fetch_time = (
      lambda ymref_id, column: calls.append(('mark', ymref_id, column)))
    self.application._load_reporting = fail_reporting

    self.application.on_timer()

    self.assertEqual(calls, [
      ('connect',),
      ('mark', 17, 'last_fetch_try_time'),
      ('reporting', 17),
      ('error', 0),
      ('reset',),
      ('connect',),
    ])

  def test_parse_log_part(self):
    records = self.application._parse_log_part(17, log_part(), self.event_date)

    self.assertEqual(len(records), 1)
    visit_id, event_date, event_time, request_id, payload, comparable = records[0]
    self.assertEqual(visit_id, 123)
    self.assertEqual(event_date, self.event_date)
    self.assertEqual(event_time, datetime.datetime(
      2026, 9, 10, 9, 34, 56, tzinfo=datetime.timezone.utc))
    self.assertEqual(request_id, REQUEST_ID)
    self.assertTrue(comparable)
    self.assertEqual(json.loads(payload), {
      'landing_bounced': True,
      'landing_session_time': 42,
      'landing_page_views': 3,
      'landing_is_new_user': False,
      'yandex_ref_id': 17,
      'yandex_event_date': '2026-09-10',
      'yandex_reporting_comparable': True,
    })

  def test_logs_request_id_does_not_depend_on_ccid(self):
    records = self.application._parse_log_part(
      17,
      log_part(content='customer-value'),
      self.event_date)

    self.assertEqual(len(records), 1)
    self.assertFalse(records[0][5])
    self.assertFalse(json.loads(records[0][4])['yandex_reporting_comparable'])

  def test_filters_invalid_request_id(self):
    self.assertEqual(
      self.application._parse_log_part(
        17,
        log_part(request_id='invalid'),
        self.event_date),
      [])

  def test_process_logs_publishes_once_across_repeated_exports(self):
    class Ch:
      def __init__(self):
        self.request_states = {}
        self.logs = []

      def query(self, query, parameters):
        if f'FROM {IMPORTER.LOG_REQUESTS_TABLE}' in query:
          key = (parameters['ymref_id'], parameters['event_date'])
          state = self.request_states.get(key)
          return types.SimpleNamespace(result_rows=[state] if state else [])

        if 'SELECT visit_id' in query:
          rows = [
            (row[1],) for row in self.logs
            if row[0] == parameters['ymref_id'] and
              row[2] == parameters['event_date']
          ]
          return types.SimpleNamespace(result_rows=rows)

        raise AssertionError(query)

      def insert(self, table, rows, column_names):
        if table == IMPORTER.LOG_REQUESTS_TABLE:
          for row in rows:
            self.request_states[(row[0], row[1])] = (row[2], row[3])
        elif table == IMPORTER.LOGS_SYNC_TABLE:
          self.logs.extend(rows)
        else:
          raise AssertionError(table)

    class Api:
      def __init__(self):
        self.created = []
        self.cleaned = []
        self.next_request_id = 100

      def create_log_request(self, event_date, attribution):
        request_id = self.next_request_id
        self.next_request_id += 1
        self.created.append((event_date, attribution, request_id))
        return {'request_id': request_id, 'status': 'created'}

      def get_log_request(self, request_id):
        return {
          'request_id': request_id,
          'status': 'processed',
          'parts': [{'part_number': 0}],
        }

      def download_log_part(self, request_id, part_number):
        return log_part()

      def clean_log_request(self, request_id):
        self.cleaned.append(request_id)

    self.application.days = 2
    self.application.running = True
    self.application.print_line = 0
    self.application.ch = Ch()
    self.application.chunks_count = 24
    self.application.print_ = lambda *args, **kwargs: None
    updated = []
    self.application._update_import_status = (
      lambda ymref_id, event_date, reporting:
        updated.append((ymref_id, event_date)))
    api = Api()

    with tempfile.TemporaryDirectory() as directory:
      self.application.in_dir = None
      self.application.out_dir = None
      self.application.markers_dir = None
      self.application.tmp_dir = str(pathlib.Path(directory) / 'tmp')
      self.application.post_click_dir = str(pathlib.Path(directory) / 'out')
      pathlib.Path(self.application.tmp_dir).mkdir()
      pathlib.Path(self.application.post_click_dir).mkdir()

      self.application._process_logs(17, api, {'rows': {}, 'unsampled': True})
      self.application._process_logs(17, api, {'rows': {}, 'unsampled': True})

      output_files = tuple(pathlib.Path(self.application.post_click_dir).iterdir())
      self.assertEqual(len(output_files), 1)
      lines = output_files[0].read_text().splitlines()
      self.assertEqual(lines[0], IMPORTER.POST_CLICK_ACTION_VERSION)
      self.assertEqual(lines[1].split('\t')[1:3], [REQUEST_ID, 'landing'])

    self.assertEqual(len(api.created), 2)
    self.assertEqual(api.cleaned, [100, 101])
    self.assertEqual(len(self.application.ch.logs), 1)
    self.assertEqual(len(updated), 2)

  def test_reset_connections_closes_postgres_after_rollback_error(self):
    class Pg:
      def __init__(self):
        self.closed = False

      def rollback(self):
        raise RuntimeError('rollback failed')

      def close(self):
        self.closed = True

    class Ch:
      def __init__(self):
        self.closed = False

      def close(self):
        self.closed = True

    pg = Pg()
    ch = Ch()
    self.application.pg = pg
    self.application.ch = ch

    self.application._reset_connections()

    self.assertTrue(pg.closed)
    self.assertTrue(ch.closed)
    self.assertIsNone(self.application.pg)
    self.assertIsNone(self.application.ch)


if __name__ == '__main__':
  unittest.main()
