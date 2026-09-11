#!/usr/bin/env python3.12

import datetime
import importlib.util
import json
import pathlib
import sys
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


def log_part(source='genius', request_id=REQUEST_ID, content='ccid:2527264'):
  header = (
    'ym:s:visitID\tym:s:date\tym:s:dateTimeUTC\tym:s:bounce\t'
    'ym:s:visitDuration\tym:s:pageViews\tym:s:isNewUser\t'
    'ym:s:lastsignUTMSource\tym:s:lastsignUTMTerm\t'
    'ym:s:lastsignUTMContent')
  row = (
    f'123\t2026-09-10\t2026-09-10T12:34:56\t1\t42\t3\t0\t'
    f'{source}\t{request_id}\t{content}')
  return header + '\n' + row + '\n'


class YandexPostClickImporterTest(unittest.TestCase):
  def setUp(self):
    self.application = object.__new__(IMPORTER.Application)
    self.application.sources = {'genius'}
    self.application.attribution = 'lastsign'
    self.event_date = datetime.date(2026, 9, 10)

  def test_reporting_api_contract(self):
    class Api:
      def reporting(self, params):
        return params

    params = self.application._reporting_page(
      Api(),
      'genius',
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
    self.assertIn("ym:s:<attribution>UTMSource=='genius'", params['filters'])

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

  def test_filters_source_and_invalid_request_id(self):
    self.assertEqual(
      self.application._parse_log_part(
        17,
        log_part(source='other'),
        self.event_date),
      [])
    self.assertEqual(
      self.application._parse_log_part(
        17,
        log_part(request_id='invalid'),
        self.event_date),
      [])


if __name__ == '__main__':
  unittest.main()
