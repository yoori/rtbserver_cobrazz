#!/usr/bin/env python3.12

import csv
import datetime
import importlib.util
import json
import pathlib
import sys
import tempfile
import types
import urllib.parse
import unittest
from contextlib import nullcontext
from unittest.mock import Mock, patch


REPOSITORY_ROOT = pathlib.Path(__file__).resolve().parents[2]


def load_importer():
  sys.path.insert(0, str(REPOSITORY_ROOT / 'lib'))
  sys.path.insert(0, str(REPOSITORY_ROOT / 'bin'))
  sys.modules.setdefault('requests', types.ModuleType('requests'))
  module_path = REPOSITORY_ROOT / 'bin' / 'YandexPostClickImporter.py'
  spec = importlib.util.spec_from_file_location('YandexPostClickImporter', module_path)
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


IMPORTER = load_importer()
REQUEST_ID = 'AAAAAAAAAAAAAAAAAAAAAA..'
USER_ID = 'BBBBBBBBBBBBBBBBBBBBBB..'
COOKIE_USER_ID = 'CCCCCCCCCCCCCCCCCCCCCC..'
DISTRIBUTION_HASH = 137
LEGACY_METRIKA_TERM = f'r:{REQUEST_ID};u1:{USER_ID};u2:{COOKIE_USER_ID}'
METRIKA_TERM = (
  f'r:{REQUEST_ID};h:{DISTRIBUTION_HASH};u1:{USER_ID};u2:{COOKIE_USER_ID}')


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
    self.application.conversions = Mock(spec=IMPORTER.ConversionImporter)
    self.application.logs_expiry_hours = 24
    self.application.residual = types.SimpleNamespace(
      bootstrap=lambda *args: None, finalize=lambda: None,
      remember=lambda *args: None, reconcile=lambda *args: None)
    self.event_date = datetime.date(2026, 9, 10)

  def test_start_always_initializes_conversions(self):
    with tempfile.TemporaryDirectory() as directory:
      with patch('ServiceUtilsPy.Service.signal.signal'), patch.object(
          sys, 'argv', ['YandexPostClickImporter', '--out-dir', directory]):
        app = IMPORTER.Application()
        app._connect = Mock()
        app._migrate_clickhouse = Mock()
        app.ch = Mock()
        with patch.object(IMPORTER, 'ResidualPublisher'):
          app.on_start()
      self.assertIsInstance(app.conversions, IMPORTER.ConversionImporter)
      self.assertEqual(app.conversions.app, app)
      self.assertTrue(pathlib.Path(app.conversions.names_dir).is_dir())
      self.assertEqual(app.logs_expiry_hours, 72)

  def test_reporting_api_contract(self):
    class Api:
      def timezone(self):
        return datetime.timezone(datetime.timedelta(hours=3))

      def reporting_timezone(self):
        return '+03:00'

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
    self.assertEqual(params['timezone'], '+03:00')
    self.assertEqual(params['attribution'], 'lastsign')
    self.assertNotIn('UTMSource', params['filters'])
    self.assertIn("ym:s:<attribution>UTMContent=@'ccid:'", params['filters'])
    self.assertIn("ym:s:<attribution>UTMTerm=~", params['filters'])
    self.assertIn('r(:|', params['filters'])
    self.assertIn('h(:|', params['filters'])
    self.assertIn('u1(:|', params['filters'])
    self.assertIn('u2(:|', params['filters'])
    self.assertIn('%3[Bb]', params['filters'])
    self.assertIn('%253[Bb]', params['filters'])
    self.assertIn('%253[Aa]', params['filters'])
    self.assertIn('[.][.]', params['filters'])
    self.assertNotIn(r'\.\.', params['filters'])

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
    for field in ('ym:s:goalsID', 'ym:s:goalsSerialNumber', 'ym:s:goalsDateTime'):
      self.assertIn(field, params['fields'].split(','))
    self.assertEqual(headers, {'Authorization': 'OAuth secret'})
    self.assertEqual(timeout, 17)

  def test_logs_evaluation_always_includes_goals(self):
    response = Mock()
    response.json.return_value = {'log_request_evaluation': {'possible': True}}
    api = IMPORTER.YandexApi('secret', 123, 17)
    with patch.object(IMPORTER.requests, 'get', return_value=response, create=True) as get:
      for include_robot in (False, True):
        with self.subTest(include_robot=include_robot):
          self.assertTrue(api.evaluate_log_fields(
            self.event_date, 'lastsign', include_robot=include_robot))
          fields = get.call_args.kwargs['params']['fields'].split(',')
          for field in ('ym:s:goalsID', 'ym:s:goalsSerialNumber', 'ym:s:goalsDateTime'):
            self.assertIn(field, fields)
          self.assertEqual('ym:s:isRobotPro' in fields, include_robot)

  def test_reporting_timezone_uses_counter_offset_and_caches_it(self):
    calls = []

    class Response:
      def raise_for_status(self):
        pass

      def json(self):
        return {'counter': {'time_zone_offset': 180}}

    def get(url, headers, timeout):
      calls.append((url, headers, timeout))
      return Response()

    old_get = getattr(IMPORTER.requests, 'get', None)
    IMPORTER.requests.get = get
    try:
      api = IMPORTER.YandexApi('secret', 123, 17)
      self.assertEqual(api.reporting_timezone(), '+03:00')
      self.assertEqual(api.reporting_timezone(), '+03:00')
      self.assertEqual(api.timezone().utcoffset(None), datetime.timedelta(hours=3))
    finally:
      if old_get is None:
        del IMPORTER.requests.get
      else:
        IMPORTER.requests.get = old_get

    self.assertEqual(calls, [(
      'https://api-metrika.yandex.net/management/v1/counter/123',
      {'Authorization': 'OAuth secret'},
      17,
    )])

  def test_format_timezone_offset(self):
    self.assertEqual(IMPORTER.format_timezone_offset(0), '+00:00')
    self.assertEqual(IMPORTER.format_timezone_offset(330), '+05:30')
    self.assertEqual(IMPORTER.format_timezone_offset(-210), '-03:30')
    with self.assertRaises(ValueError):
      IMPORTER.format_timezone_offset(24 * 60)

  def test_parse_ccid(self):
    self.assertEqual(IMPORTER.parse_ccid('ccid:2527264'), 2527264)
    self.assertEqual(IMPORTER.parse_ccid('key:value;ccid:17&other:value'), 17)
    self.assertIsNone(IMPORTER.parse_ccid('prefix-ccid:17'))
    self.assertIsNone(IMPORTER.parse_ccid(''))

  def test_parse_metrika_request_id(self):
    self.assertEqual(IMPORTER.parse_metrika_request_id(REQUEST_ID), REQUEST_ID)
    self.assertEqual(IMPORTER.parse_metrika_request_id(METRIKA_TERM), REQUEST_ID)
    self.assertEqual(
      IMPORTER.parse_metrika_request_id(METRIKA_TERM.replace(';', '%3B')),
      REQUEST_ID)
    encoded_term = urllib.parse.quote_plus(METRIKA_TERM.replace(';', '%3B'))
    self.assertEqual(IMPORTER.parse_metrika_request_id(encoded_term), REQUEST_ID)
    self.assertEqual(
      IMPORTER.parse_metrika_request_id(f'r:{REQUEST_ID};u1:;u2:'),
      REQUEST_ID)
    self.assertIsNone(
      IMPORTER.parse_metrika_request_id(f'r:{REQUEST_ID};u1:invalid;u2:'))
    self.assertIsNone(IMPORTER.parse_metrika_request_id('invalid'))

  def test_parse_metrika_distribution_hash(self):
    self.assertEqual(
      IMPORTER.parse_metrika_term(METRIKA_TERM),
      (REQUEST_ID, DISTRIBUTION_HASH))
    self.assertEqual(
      IMPORTER.parse_metrika_term(METRIKA_TERM.replace(';', '%3B')),
      (REQUEST_ID, DISTRIBUTION_HASH))
    encoded_term = urllib.parse.quote_plus(METRIKA_TERM.replace(';', '%3B'))
    self.assertEqual(
      IMPORTER.parse_metrika_term(encoded_term),
      (REQUEST_ID, DISTRIBUTION_HASH))

  def test_legacy_metrika_term_uses_resolved_user_id_hash(self):
    self.assertEqual(
      IMPORTER.parse_metrika_term(LEGACY_METRIKA_TERM),
      (REQUEST_ID, IMPORTER.user_id_distribution_hash(USER_ID)))
    self.assertEqual(
      IMPORTER.parse_metrika_term(f'r:{REQUEST_ID};u1:;u2:'),
      (REQUEST_ID, None))
    self.assertEqual(IMPORTER.parse_metrika_term(REQUEST_ID), (REQUEST_ID, None))

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

      def timezone(self):
        return datetime.timezone(datetime.timedelta(hours=3))

      def reporting_timezone(self):
        return '+03:00'

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
        self.parameters = None

      def query(self, query, parameters):
        self.parameters = parameters
        return types.SimpleNamespace(result_rows=[
          (
            datetime.datetime(2026, 9, 10, 9),
            1, 3, 1, 24.0, 4, 1, False, 1.0,
            datetime.datetime(2026, 9, 10, 10),
          ),
          (
            datetime.datetime(2026, 9, 10, 11),
            3, 2, 1, 12.5, 4, 2, True, 0.5,
            datetime.datetime(2026, 9, 10, 12),
          ),
        ])

      def insert(self, table, rows, column_names):
        self.inserted.append((table, rows, column_names))

    api = Api()
    self.application.days = 2
    self.application.ch = Ch()
    published = []
    self.application._publish_estimation_deltas = (
      lambda rows, batch_id: published.extend(rows))
    old_page_size = IMPORTER.REPORTING_PAGE_SIZE
    IMPORTER.REPORTING_PAGE_SIZE = 2
    try:
      result = self.application._load_reporting(17, api, self.event_date, self.event_date)
    finally:
      IMPORTER.REPORTING_PAGE_SIZE = old_page_size

    hour1 = datetime.datetime(2026, 9, 10, 9, tzinfo=datetime.timezone.utc)
    hour2 = datetime.datetime(2026, 9, 10, 10, tzinfo=datetime.timezone.utc)
    self.assertEqual(api.offsets, [1, 3])
    self.assertEqual(result['rows'][(hour1, 1)][:7], [3, 1, 0, 0, 24.0, 4, 1])
    self.assertEqual(result['rows'][(hour2, 2)][:7], [4, 1, 0, 0, 8.0, 5, 2])
    self.assertFalse(result['unsampled'])
    self.assertEqual(result['date1'], datetime.date(2026, 9, 10))
    self.assertEqual(result['date2'], datetime.date(2026, 9, 10))
    self.assertEqual(result['timezone'].utcoffset(None), datetime.timedelta(hours=3))
    self.assertEqual(published, [])
    self.assertEqual(len(self.application.ch.inserted), 1)
    snapshot = self.application.ch.inserted[0][1]
    self.assertEqual(len(snapshot), 3)
    self.assertEqual(snapshot[0][1:10], (hour1, 1, 3, 1, 0, 0, 24.0, 4, 1))
    self.assertEqual(snapshot[1][1:10], (hour2, 2, 4, 1, 0, 0, 8.0, 5, 2))
    self.assertEqual(snapshot[2][3:10], (0, 0, 0, 0, 0.0, 0, 0))

  def test_mark_fetch_time_calls_database_function(self):
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
    self.application._mark_fetch_time(17, False)
    self.application._mark_fetch_time(17, True)

    self.assertEqual(calls, [
      (
        'SELECT adserver.update_external_metrics_connection_fetch_time(%s, %s)',
        (17, False),
      ),
      (
        'SELECT adserver.update_external_metrics_connection_fetch_time(%s, %s)',
        (17, True),
      ),
    ])
    self.assertEqual(self.application.pg.commits, 2)

  def test_mark_fetch_time_rejects_non_boolean_success(self):
    with self.assertRaises(ValueError):
      self.application._mark_fetch_time(17, None)

  def test_on_timer_deduplicates_counter_across_accounts(self):
    calls = []

    class Cursor:
      def __enter__(self):
        return self

      def __exit__(self, exc_type, exc_value, traceback):
        return False

      def execute(self, query, parameters):
        calls.append(('select', query, parameters))

      def fetchall(self):
        return [(17, 'secret', 123, 'A'), (18, 'another-token', 123, 'A')]

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
      lambda connection_id, column: calls.append(('mark', connection_id, column)))
    self.application._load_reporting = (
      lambda connection_id, api: calls.append(('reporting', connection_id, api.counter_id)) or {})
    self.application._process_logs = (
      lambda connection_id, api, reporting: calls.append(('logs', connection_id, api.counter_id)))

    self.application.on_timer()

    self.assertEqual(
      calls[0],
      (
        'select',
        "SELECT c.external_metrics_connection_id, y.token, y.metrika_id, c.status "
        "FROM externalmetricsconnection c JOIN externalmetricsconnectionyandexmetrika y "
        "USING (external_metrics_connection_id) "
        "WHERE c.external_metrics_connection_type_id = %s "
        "ORDER BY c.external_metrics_connection_id",
        (IMPORTER.YANDEX_METRIKA,),
      ))
    self.assertEqual(calls[1:], [
      ('mark', 17, False),
      ('reporting', 123, 123),
      ('logs', 123, 123),
      ('mark', 17, True),
    ])
    self.assertEqual(self.application.pg.commits, 1)
    self.application.conversions.prepare.assert_called_once()
    self.assertEqual(self.application.conversions.prepare.call_args.args[0].counter_id, 123)

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
        return [(17, 'secret', 123, 'A')]

    class Pg:
      def cursor(self):
        return Cursor()

      def commit(self):
        pass

    def fail_reporting(connection_id, api):
      calls.append(('reporting', connection_id))
      raise RuntimeError('test failure')

    self.application.pg = Pg()
    self.application.request_timeout = 11
    self.application._connect = lambda: calls.append(('connect',))
    self.application._reset_connections = lambda: calls.append(('reset',))
    self.application.verify_running = lambda: None
    self.application.print_ = lambda level, message: calls.append(('error', level))
    self.application._mark_fetch_time = (
      lambda connection_id, column: calls.append(('mark', connection_id, column)))
    self.application._load_reporting = fail_reporting

    self.application.on_timer()

    self.assertEqual(calls, [
      ('connect',),
      ('mark', 17, False),
      ('reporting', 123),
      ('error', 0),
      ('reset',),
      ('connect',),
    ])

  def test_on_timer_tries_another_token_for_the_same_counter(self):
    calls = []
    cursor = types.SimpleNamespace(execute=lambda *args: None,
      fetchall=lambda: [(17, 'expired', 123, 'A'), (18, 'working', 123, 'A')])
    self.application.pg = types.SimpleNamespace(
      cursor=lambda: nullcontext(cursor), commit=lambda: None)
    self.application.request_timeout = 11
    self.application.verify_running = lambda: None
    self.application._connect = lambda: None
    self.application._reset_connections = lambda: None
    self.application.print_ = lambda *args: None
    self.application._mark_fetch_time = lambda *args: calls.append(('mark', *args))
    self.application._process_logs = lambda counter, *args: calls.append(('logs', counter))
    def reporting(counter, api):
      if api.headers['Authorization'] == 'OAuth expired':
        raise RuntimeError('expired token')
      calls.append(('reporting', counter))
      return {}
    self.application._load_reporting = reporting
    self.application.on_timer()
    self.assertEqual(calls, [
      ('mark', 17, False), ('mark', 18, False), ('reporting', 123),
      ('logs', 123), ('mark', 18, True)])

  def test_parse_log_part(self):
    records = self.application._parse_log_part(17, log_part(), self.event_date)

    self.assertEqual(len(records), 1)
    visit_id, event_date, event_time, request_id, distribution_hash, payload, comparable = \
      records[0]
    self.assertEqual(visit_id, 123)
    self.assertEqual(event_date, self.event_date)
    self.assertEqual(event_time, datetime.datetime(
      2026, 9, 10, 9, 34, 56, tzinfo=datetime.timezone.utc))
    self.assertEqual(request_id, REQUEST_ID)
    self.assertIsNone(distribution_hash)
    self.assertTrue(comparable)
    self.assertEqual(json.loads(payload), {
      'landing_bounced': True,
      'landing_session_time': 42,
      'landing_page_views': 3,
      'landing_is_new_user': False,
      'external_metrics_connection_type_id': 1,
      'source_date': '2026-09-10',
      'reporting_comparable': True,
    })

  def test_processed_logs_always_reach_conversion_importer(self):
    text = log_part()
    api = types.SimpleNamespace(
      get_log_request=lambda request: {'status': 'processed', 'parts': [{'part_number': 0}]},
      download_log_part=lambda request, part: text,
      timezone=lambda: datetime.timezone.utc, clean_log_request=Mock())
    self.application.verify_running = Mock()
    self.application._save_log_request = Mock()
    self.application._publish_post_click_actions = Mock(return_value=[])
    self.application._process_log_request(17, self.event_date, 100, api, {})
    self.application.conversions.process.assert_called_once_with(17, self.event_date, text)
    api.clean_log_request.assert_called_once_with(100)

  def test_post_click_action_name_is_unique_per_visit(self):
    self.assertEqual(IMPORTER.format_post_click_action_name(17, 123), 'landing:ym_17-123')
    self.assertNotEqual(
      IMPORTER.format_post_click_action_name(17, 123),
      IMPORTER.format_post_click_action_name(17, 124))

  def test_parse_log_part_with_metrika_term(self):
    records = self.application._parse_log_part(
      17,
      log_part(request_id=METRIKA_TERM.replace(';', '%3B')),
      self.event_date)

    self.assertEqual(len(records), 1)
    self.assertEqual(records[0][3], REQUEST_ID)
    self.assertEqual(records[0][4], DISTRIBUTION_HASH)

  def test_logs_request_id_does_not_depend_on_ccid(self):
    records = self.application._parse_log_part(
      17,
      log_part(content='customer-value'),
      self.event_date)

    self.assertEqual(len(records), 1)
    self.assertFalse(records[0][6])
    self.assertFalse(json.loads(records[0][5])['reporting_comparable'])

  def test_filters_invalid_request_id(self):
    self.assertEqual(
      self.application._parse_log_part(
        17,
        log_part(request_id='invalid'),
        self.event_date),
      [])

  def test_log_watermark_includes_rows_without_request_id(self):
    records, watermark = self.application._parse_log_part_with_watermark(
      17,
      log_part(request_id='invalid'),
      self.event_date)

    self.assertEqual(records, [])
    self.assertEqual(
      watermark,
      datetime.datetime(2026, 9, 10, 9, 34, 56, tzinfo=datetime.timezone.utc))

  def test_process_logs_publishes_once_across_repeated_exports(self):
    class Ch:
      def __init__(self):
        self.request_states = {}
        self.logs = []

      def query(self, query, parameters):
        if f'FROM {IMPORTER.LOG_REQUESTS_TABLE}' in query:
          if 'source_date = %(event_date)s' not in query:
            rows = [
              (event_date, request_id, status)
              for (connection_id, event_date), (request_id, status) in
                self.request_states.items()
              if connection_id == parameters['metrika_id']
            ]
            return types.SimpleNamespace(result_rows=rows)

          key = (parameters['metrika_id'], parameters['event_date'])
          state = self.request_states.get(key)
          return types.SimpleNamespace(result_rows=[state] if state else [])

        if 'SELECT visit_id' in query:
          rows = [
            (row[1],) for row in self.logs
            if row[0] == parameters['metrika_id'] and
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

      def timezone(self):
        return datetime.timezone.utc

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
        return log_part(request_id=METRIKA_TERM)

      def clean_log_request(self, request_id):
        self.cleaned.append(request_id)

    self.application.days = 2
    self.application.running = True
    self.application.print_line = 0
    self.application.ch = Ch()
    self.application.chunks_count = 24
    self.application.print_ = lambda *args, **kwargs: None
    updated = []
    watermarks = []
    self.application._advance_logs_watermark = (
      lambda connection_id, event_time: watermarks.append((connection_id, event_time)))
    self.application._update_import_status = (
      lambda connection_id, event_date, reporting:
        updated.append((connection_id, event_date)))
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
      self.assertTrue(output_files[0].name.endswith(f'.24.{DISTRIBUTION_HASH % 24}'))
      output = output_files[0].read_text()
      self.assertTrue(output.endswith('\n'))
      lines = output.splitlines()
      self.assertEqual(lines[0], IMPORTER.POST_CLICK_ACTION_VERSION)
      self.assertEqual(lines[1].split('\t')[1:3], [REQUEST_ID, 'landing:ym_17-123'])

    self.assertEqual(len(api.created), 2)
    self.assertEqual(api.cleaned, [100, 101])
    self.assertEqual(len(self.application.ch.logs), 1)
    self.assertEqual(updated, [])
    self.assertEqual(watermarks, [])

  def test_process_logs_finishes_request_outside_date_window(self):
    old_date = datetime.datetime.now(datetime.timezone.utc).date() - datetime.timedelta(days=10)

    class Ch:
      def __init__(self):
        self.request_states = {(17, old_date): (100, 'created')}
        self.logs = []

      def query(self, query, parameters):
        if f'FROM {IMPORTER.LOG_REQUESTS_TABLE}' in query:
          if 'source_date = %(event_date)s' not in query:
            rows = [
              (event_date, request_id, status)
              for (connection_id, event_date), (request_id, status) in
                self.request_states.items()
              if connection_id == parameters['metrika_id']
            ]
            return types.SimpleNamespace(result_rows=rows)

          key = (parameters['metrika_id'], parameters['event_date'])
          state = self.request_states.get(key)
          return types.SimpleNamespace(result_rows=[state] if state else [])

        if 'SELECT visit_id' in query:
          rows = [
            (row[1],) for row in self.logs
            if row[0] == parameters['metrika_id'] and
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
        self.polls = []

      def timezone(self):
        return datetime.timezone.utc

      def create_log_request(self, event_date, attribution):
        self.created.append(event_date)
        return {'request_id': 101, 'status': 'created'}

      def get_log_request(self, request_id):
        self.polls.append(request_id)
        if request_id == 100 and self.polls.count(request_id) > 1:
          return {
            'request_id': request_id,
            'status': 'processed',
            'parts': [{'part_number': 0}],
          }
        return {'request_id': request_id, 'status': 'created'}

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
    watermarks = []
    reporting_calls = []
    self.application._load_reporting = (
      lambda connection_id, api, date1, date2:
        reporting_calls.append((connection_id, date1, date2)) or {
          'rows': {},
          'unsampled': True,
          'date1': date1,
          'date2': date2,
        })
    self.application._update_import_status = (
      lambda connection_id, event_date, reporting:
        updated.append((connection_id, event_date)))
    self.application._advance_logs_watermark = (
      lambda connection_id, event_time: watermarks.append((connection_id, event_time)))
    api = Api()

    with tempfile.TemporaryDirectory() as directory:
      self.application.in_dir = None
      self.application.out_dir = None
      self.application.markers_dir = None
      self.application.tmp_dir = str(pathlib.Path(directory) / 'tmp')
      self.application.post_click_dir = str(pathlib.Path(directory) / 'out')
      pathlib.Path(self.application.tmp_dir).mkdir()
      pathlib.Path(self.application.post_click_dir).mkdir()

      today = datetime.datetime.now(datetime.timezone.utc).date()
      reporting = {
        'rows': {},
        'unsampled': True,
        'date1': today - datetime.timedelta(days=1),
        'date2': today,
      }
      self.application._process_logs(17, api, reporting)
      self.application._process_logs(17, api, reporting)

      output_files = tuple(pathlib.Path(self.application.post_click_dir).iterdir())
      self.assertEqual(len(output_files), 1)

    self.assertEqual(api.cleaned, [100])
    self.assertEqual(len(api.created), 1)
    self.assertEqual(api.polls.count(100), 2)
    self.assertEqual(self.application.ch.request_states[(17, old_date)], (100, 'cleaned'))
    self.assertEqual(len(self.application.ch.logs), 1)
    self.assertEqual(reporting_calls, [(17, old_date, old_date)])
    self.assertEqual(updated, [])
    self.assertEqual(watermarks, [])

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
