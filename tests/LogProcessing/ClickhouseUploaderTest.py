#!/usr/bin/env python3

import csv
import importlib.util
import io
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest


REPOSITORY_ROOT = pathlib.Path(__file__).resolve().parents[2]


def load_clickhouse_uploader():
  module_path = REPOSITORY_ROOT / 'bin' / 'ClickhouseStatUploader.py'
  spec = importlib.util.spec_from_file_location('ClickhouseStatUploader', module_path)
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


CLICKHOUSE_UPLOADER = load_clickhouse_uploader()


class QueryExecutorStub:
  def __init__(self, applied_migrations = ''):
    self.applied_migrations = applied_migrations
    self.queries = []

  def execute(self, query):
    self.queries.append(query)
    if query.startswith('SELECT migration_id'):
      return self.applied_migrations
    return ''


class ClickhouseMigrationRunnerTest(unittest.TestCase):
  def test_applies_and_records_pending_migration(self):
    executor = QueryExecutorStub()

    CLICKHOUSE_UPLOADER.ClickhouseMigrationRunner(executor).run()

    self.assertEqual(executor.queries[0], CLICKHOUSE_UPLOADER.MIGRATIONS_CREATE_TABLE_QUERY)
    self.assertTrue(executor.queries[1].startswith('SELECT migration_id'))
    for index, migration in enumerate(CLICKHOUSE_UPLOADER.MIGRATIONS):
      query_index = 2 + index * 2
      self.assertEqual(executor.queries[query_index], migration.query)
      insert_query, record_json = executor.queries[query_index + 1].split('\n', 1)
      self.assertIn('INSERT INTO Migrations', insert_query)
      record = json.loads(record_json)
      self.assertEqual(record['component'], CLICKHOUSE_UPLOADER.MIGRATION_COMPONENT)
      self.assertEqual(record['migration_id'], migration.migration_id)
      self.assertEqual(record['name'], migration.name)
      self.assertEqual(record['checksum'], migration.checksum())

  def test_skips_applied_migration(self):
    applied_migrations = ''.join(
      json.dumps({
        'migration_id': migration.migration_id,
        'name': migration.name,
        'checksum': migration.checksum(),
      }) + '\n'
      for migration in CLICKHOUSE_UPLOADER.MIGRATIONS)
    executor = QueryExecutorStub(applied_migrations)

    CLICKHOUSE_UPLOADER.ClickhouseMigrationRunner(executor).run()

    self.assertEqual(len(executor.queries), 2)
    self.assertIn(
      "WHERE component = 'ClickhouseStatUploader'",
      executor.queries[1])

  def test_rejects_changed_applied_migration(self):
    migration = CLICKHOUSE_UPLOADER.MIGRATIONS[0]
    applied_migrations = json.dumps({
      'migration_id': migration.migration_id,
      'name': migration.name,
      'checksum': '0' * 64,
    }) + '\n'
    executor = QueryExecutorStub(applied_migrations)

    with self.assertRaisesRegex(Exception, 'checksum mismatch'):
      CLICKHOUSE_UPLOADER.ClickhouseMigrationRunner(executor).run()

  def test_rejects_unknown_applied_migration(self):
    applied_migrations = json.dumps({
      'migration_id': 999,
      'name': 'future_migration',
      'checksum': '0' * 64,
    }) + '\n'
    executor = QueryExecutorStub(applied_migrations)

    with self.assertRaisesRegex(Exception, 'Unknown applied'):
      CLICKHOUSE_UPLOADER.ClickhouseMigrationRunner(executor).run()


class ClickhouseAdapterTest(unittest.TestCase):
  def run_adapter(self, adapter_name, rows):
    input_files = []
    with tempfile.TemporaryDirectory() as temp_dir:
      for index, row in enumerate(rows):
        input_path = pathlib.Path(temp_dir) / ('input-' + str(index) + '.csv')
        with input_path.open('w', newline = '') as output:
          writer = csv.writer(output)
          writer.writerow(['header'])
          writer.writerow(row)
        input_files.append(str(input_path))

      result = subprocess.run(
        [sys.executable, str(REPOSITORY_ROOT / 'bin' / adapter_name)] + input_files,
        universal_newlines = True,
        capture_output = True,
        check = True)

    return list(csv.reader(io.StringIO(result.stdout)))

  def test_rimpression_adapter_accepts_old_and_new_rows(self):
    additional_info = json.dumps({
      'ssp_tag_id': 'ssp-tag',
      'ctr': 0.12,
      'viewability': 0.34,
      'vtr': 0.56,
    })
    old_row = [str(index) for index in range(30)]
    old_row[29] = additional_info
    page_keywords = 'first, "second"\nтретий'
    new_row = old_row[:29] + [page_keywords, additional_info]
    expected_post_actions = 'vstart,vview'
    newest_row = old_row[:29] + [
      page_keywords,
      expected_post_actions,
      additional_info,
    ]

    output = self.run_adapter(
      'RImpressionClickhouseAdapter.py',
      [old_row, new_row, newest_row])
    header = output[0]
    old_output = dict(zip(header, output[1]))
    new_output = dict(zip(header, output[2]))
    newest_output = dict(zip(header, output[3]))

    self.assertEqual(old_output['page_keywords'], '')
    self.assertEqual(new_output['page_keywords'], page_keywords)
    self.assertEqual(old_output['expected_post_actions'], '[]')
    self.assertEqual(new_output['expected_post_actions'], '[]')
    self.assertEqual(newest_output['page_keywords'], page_keywords)
    self.assertEqual(newest_output['expected_post_actions'], "['vstart','vview']")
    for row in (old_output, new_output, newest_output):
      self.assertEqual(row['ssp_tag_id'], 'ssp-tag')
      self.assertEqual(row['ssp_ctr'], '0.12')
      self.assertEqual(row['ssp_viewability'], '0.34')
      self.assertEqual(row['ssp_vtr'], '0.56')

  def test_rclick_adapter_supplies_empty_page_keywords(self):
    output = self.run_adapter(
      'RClickClickhouseAdapter.py',
      [['2026-09-11 12:00:00', 'request-id']])
    header = output[0]
    row = dict(zip(header, output[1]))

    self.assertEqual(row['request_id'], 'request-id')
    self.assertEqual(row['click_timestamp'], '2026-09-11 12:00:00')
    self.assertEqual(row['page_keywords'], '')
    self.assertEqual(row['expected_post_actions'], '[]')

  def test_rimpression_table_has_expected_post_actions(self):
    field = (
      'expected_post_actions '
      'SimpleAggregateFunction(groupUniqArrayArray, Array(String))')
    self.assertIn(field, CLICKHOUSE_UPLOADER.R_IMPRESSION_CREATE_TABLE_QUERY)
    self.assertIn(field, CLICKHOUSE_UPLOADER.MIGRATIONS[1].query)

  def test_rpostclick_adapter(self):
    landing_timestamp = '2026-09-11 12:34:56'
    output = self.run_adapter(
      'RPostClickClickhouseAdapter.py',
      [['request-id', landing_timestamp, '1', '42', '3', '0']])
    header = output[0]
    row = dict(zip(header, output[1]))

    self.assertEqual(row, {
      'request_id': 'request-id',
      'landing_timestamp': landing_timestamp,
      'landing_bounced': '1',
      'landing_session_time': '42',
      'landing_page_views': '3',
      'landing_is_new_user': '0',
    })

  def test_rpostclick_table_has_landing_timestamp(self):
    self.assertIn(
      "landing_timestamp SimpleAggregateFunction(any, Nullable(DateTime('UTC')))",
      CLICKHOUSE_UPLOADER.R_POST_CLICK_CREATE_TABLE_QUERY)

  def test_rpostimpression_adapter(self):
    fields = [
      'video_start_timestamp',
      'video_view_timestamp',
      'video_q1_timestamp',
      'video_mid_timestamp',
      'video_q3_timestamp',
      'video_complete_timestamp',
      'video_skip_timestamp',
      'video_pause_timestamp',
      'video_mute_timestamp',
      'video_unmute_timestamp',
      'video_resume_timestamp',
      'video_fullscreen_timestamp',
      'video_error_timestamp',
    ]
    timestamp = '2026-09-11 12:00:00'
    output = self.run_adapter(
      'RPostImpressionClickhouseAdapter.py',
      [[timestamp, 'request-id', 'vview']])
    header = output[0]
    row = dict(zip(header, output[1]))

    self.assertEqual(row['request_id'], 'request-id')
    self.assertEqual(row['video_view_timestamp'], timestamp)
    for field in fields:
      if field != 'video_view_timestamp':
        self.assertEqual(row[field], '')

  def test_rpostimpression_table_uses_non_null_timestamps(self):
    query = CLICKHOUSE_UPLOADER.R_POST_IMPRESSION_CREATE_TABLE_QUERY
    self.assertIn('CREATE TABLE IF NOT EXISTS RPostImpression', query)
    for field in (
        'video_start_timestamp', 'video_view_timestamp', 'video_q1_timestamp',
        'video_mid_timestamp', 'video_q3_timestamp', 'video_complete_timestamp',
        'video_skip_timestamp', 'video_pause_timestamp', 'video_mute_timestamp',
        'video_unmute_timestamp', 'video_resume_timestamp',
        'video_fullscreen_timestamp', 'video_error_timestamp'):
      self.assertIn(
        field + " SimpleAggregateFunction(any, Nullable(DateTime('UTC')))",
        query)


if __name__ == '__main__':
  unittest.main()
