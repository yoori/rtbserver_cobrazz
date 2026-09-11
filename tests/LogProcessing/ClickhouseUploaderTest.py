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
    migration = CLICKHOUSE_UPLOADER.MIGRATIONS[0]

    CLICKHOUSE_UPLOADER.ClickhouseMigrationRunner(executor).run()

    self.assertEqual(executor.queries[0], CLICKHOUSE_UPLOADER.MIGRATIONS_CREATE_TABLE_QUERY)
    self.assertTrue(executor.queries[1].startswith('SELECT migration_id'))
    self.assertEqual(executor.queries[2], migration.query)
    insert_query, record_json = executor.queries[3].split('\n', 1)
    self.assertIn('INSERT INTO ClickhouseUploaderMigrations', insert_query)
    record = json.loads(record_json)
    self.assertEqual(record['migration_id'], migration.migration_id)
    self.assertEqual(record['name'], migration.name)
    self.assertEqual(record['checksum'], migration.checksum())

  def test_skips_applied_migration(self):
    migration = CLICKHOUSE_UPLOADER.MIGRATIONS[0]
    applied_migrations = json.dumps({
      'migration_id': migration.migration_id,
      'name': migration.name,
      'checksum': migration.checksum(),
    }) + '\n'
    executor = QueryExecutorStub(applied_migrations)

    CLICKHOUSE_UPLOADER.ClickhouseMigrationRunner(executor).run()

    self.assertEqual(len(executor.queries), 2)

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

    output = self.run_adapter(
      'RImpressionClickhouseAdapter.py',
      [old_row, new_row])
    header = output[0]
    old_output = dict(zip(header, output[1]))
    new_output = dict(zip(header, output[2]))

    self.assertEqual(old_output['page_keywords'], '')
    self.assertEqual(new_output['page_keywords'], page_keywords)
    for row in (old_output, new_output):
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

  def test_rpostclick_adapter(self):
    output = self.run_adapter(
      'RPostClickClickhouseAdapter.py',
      [['request-id', '1', '42', '3', '0']])
    header = output[0]
    row = dict(zip(header, output[1]))

    self.assertEqual(row, {
      'request_id': 'request-id',
      'landing_bounced': '1',
      'landing_session_time': '42',
      'landing_page_views': '3',
      'landing_is_new_user': '0',
    })


if __name__ == '__main__':
  unittest.main()
