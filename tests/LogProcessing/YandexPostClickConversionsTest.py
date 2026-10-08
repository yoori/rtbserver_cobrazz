#!/usr/bin/env python3.12

import csv
import datetime
import io
import json
import pathlib
import sys
import tempfile
import types
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'lib'))
sys.path.insert(0, str(ROOT / 'bin'))

from rtbserver_utils import YandexPostClickConversions as conversions
from YandexPostClickImporter import (
  Application, parse_log_event_time, parse_metrika_term, request_chunk)


class ClickHouse:
  def __init__(self):
    self.tables = {}
    self.fail_names = False

  def insert(self, table, rows, column_names):
    if table == conversions.NAMES_TABLE and self.fail_names:
      self.fail_names = False
      raise RuntimeError('crash after CSV publication')
    self.tables.setdefault(table, []).extend(dict(zip(column_names, row)) for row in rows)

  def query(self, sql, parameters):
    table = sql.split(' FROM ')[1].split()[0]
    keys = {
      conversions.NAMES_TABLE: ('metrika_id', 'conversion_name'),
      conversions.VISITS_TABLE: ('metrika_id', 'event_date', 'visit_id', 'goal_id', 'serial'),
      conversions.GOALS_SYNC_TABLE: ('metrika_id', 'event_date', 'visit_id', 'goal_id', 'serial'),
    }[table]
    rows = {}
    for row in self.tables.get(table, []):
      if 'counter' in parameters and row['metrika_id'] != parameters['counter']:
        continue
      if 'date' in parameters and row['event_date'] != parameters['date']:
        continue
      rows[tuple(row[key] for key in keys)] = row
    columns = sql[7:sql.index(' FROM ')].split(', ')
    return types.SimpleNamespace(result_rows=[
      tuple(row[column] for column in columns) for row in rows.values()])


class YandexPostClickConversionsTest(unittest.TestCase):
  def setUp(self):
    self.directory = tempfile.TemporaryDirectory()
    self.addCleanup(self.directory.cleanup)
    self.root = pathlib.Path(self.directory.name)
    self.app = types.SimpleNamespace(
      ch=ClickHouse(), chunks_count=24, verify_running=lambda: None, running=True,
      in_dir=None, markers_dir=None, tmp_dir=str(self.root / 'tmp'),
      out_dir=str(self.root / 'out'), post_click_dir=str(self.root / 'actions'),
      print_line=0, print_=lambda *args, **kwargs: None,
      _csv_line=Application._csv_line, _field=Application._field)
    # There is intentionally no PG connection and no Reporting snapshot in this fixture.
    self.importer = conversions.ConversionImporter(
      self.app, parse_metrika_term, parse_log_event_time, request_chunk)
    self.goals = [
      {'id': 9, 'name': 'Purchase', 'type': 'action'},
      {'id': 10, 'name': 'Zero goal', 'type': 'action'}]
    self.api = types.SimpleNamespace(counter_id=111, goals=lambda: self.goals)
    self.date = datetime.date(2026, 9, 10)

  def csv_rows(self):
    result = []
    for path in sorted(pathlib.Path(self.importer.names_dir).glob('*.csv')):
      with path.open() as source:
        result.extend(csv.DictReader(source))
    return result

  def actions(self):
    result = []
    for path in sorted(pathlib.Path(self.app.post_click_dir).glob('PostClickAction.*')):
      result.extend(line for line in path.read_text().splitlines()[1:] if line)
    return result

  def log(self, content=None, goal=9):
    fields = ['ym:s:visitID', 'ym:s:dateTimeUTC', 'ym:s:lastsignUTMTerm', *conversions.GOAL_FIELDS]
    row = [123, '2026-09-10T12:34:56', 'AAAAAAAAAAAAAAAAAAAAAA..',
      str([goal, goal]), '[11, 12]',
      "['2026-09-10T12:35:00', '2026-09-10T12:36:00']"]
    if content is not None:
      fields.append('ym:s:lastsignUTMContent')
      row.append(content)
    output = io.StringIO()
    writer = csv.writer(output, delimiter='\t')
    writer.writerow(fields)
    writer.writerow(row)
    return output.getvalue()

  def test_all_goals_without_creatives_or_pg(self):
    self.importer.prepare(self.api)
    rows = self.csv_rows()
    self.assertEqual(len(rows), 2)
    self.assertEqual(set(rows[0]), set(conversions.NAMES_CSV_COLUMNS))
    self.assertEqual({row['conversion_name'] for row in rows}, {'ym_111_9', 'ym_111_10'})
    self.assertEqual({row['external_metrics_connection_type_id'] for row in rows}, {'1'})
    self.assertNotIn('found', rows[0])
    self.importer.prepare(self.api)
    self.assertEqual(len(self.csv_rows()), 2)

  def test_dictionary_is_independent_of_access_token(self):
    self.importer.prepare(self.api)
    self.importer.prepare(self.api)
    rows = self.csv_rows()
    self.assertEqual(len(rows), 2)
    self.assertEqual({row['external_metrics_connection_type_id'] for row in rows}, {'1'})

  def test_rename_uses_monotonic_version(self):
    self.importer.prepare(self.api)
    old = self.app.ch.tables[conversions.NAMES_TABLE][0]
    old['version'] = datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=1)
    self.goals[0]['name'] = 'Renamed purchase'
    self.importer.prepare(self.api)
    row = self.app.ch.tables[conversions.NAMES_TABLE][-1]
    self.assertEqual(row['name'], 'Renamed purchase')
    self.assertGreater(row['version'], old['version'])
    self.assertEqual(len(self.csv_rows()), 3)

  def test_goal_without_ccid_and_repeated_delivery(self):
    self.importer.prepare(self.api)
    self.importer.process(111, self.date, self.log())
    actions = self.actions()
    self.assertEqual(len(actions), 3)
    payloads = [json.loads(action.split('\t')[3]) for action in actions]
    self.assertEqual(sum(payload['visits'] for payload in payloads), 1)
    self.assertEqual(sum(payload['goals'] for payload in payloads), 2)
    self.assertEqual({payload['external_metrics_connection_type_id'] for payload in payloads}, {1})
    self.assertTrue(all('external_metrics_connection_id' not in payload for payload in payloads))
    self.assertTrue(all('cc_id' not in payload for payload in payloads))
    self.importer.process(111, self.date, self.log('ccid:999999'))
    self.assertEqual(len(self.actions()), 3)
    self.assertEqual(len(self.csv_rows()), 2)

  def test_unknown_goal_gets_fallback_without_renaming_known_goal(self):
    self.importer.prepare(self.api)
    self.importer.process(111, self.date, self.log(goal=99))
    rows = self.csv_rows()
    self.assertEqual(len(rows), 3)
    self.assertEqual(rows[-1]['name'], 'ym_111_99')
    self.importer.goals[111] = {}
    self.importer.process(111, self.date, self.log())
    self.assertEqual(len(self.csv_rows()), 3)

  def test_names_csv_is_durable_before_snapshot(self):
    self.app.ch.fail_names = True
    with self.assertRaisesRegex(RuntimeError, 'crash after CSV'):
      self.importer.prepare(self.api)
    self.assertEqual(len(self.csv_rows()), 2)
    self.importer.prepare(self.api)
    self.assertEqual(len(self.csv_rows()), 4)
    self.assertEqual(len(self.app.ch.tables[conversions.NAMES_TABLE]), 2)


if __name__ == '__main__':
  unittest.main()
