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
from unittest import mock


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

  def test_site_referrer_stats_adapter_skips_headers(self):
    first = ['2026-09-23'] + [str(index) for index in range(1, 20)]
    second = ['2026-09-24'] + [str(index) for index in range(20, 39)]

    output = self.run_adapter(
      'SiteReferrerStatsClickhouseAdapter.py',
      [first, second])

    self.assertEqual(output, [first, second])

  def test_site_referrer_stats_table_is_partitioned_and_summed(self):
    query = CLICKHOUSE_UPLOADER.SITE_REFERRER_STATS_CREATE_TABLE_QUERY
    self.assertIn('CREATE TABLE IF NOT EXISTS SiteReferrerStats', query)
    self.assertIn('site_id UInt32', query)
    self.assertIn('ENGINE = SummingMergeTree', query)
    self.assertIn('PARTITION BY toYYYYMM(sdate)', query)
    self.assertIn('ORDER BY (site_id, sdate, tag_id', query)


class InterrupterStub:
  def __init__(self):
    self.was_interrupted = False

  def interrupt(self):
    self.was_interrupted = True

  def interrupted(self):
    return self.was_interrupted


class RouteWorkerTest(unittest.TestCase):
  def test_interrupted_batch_is_not_moved_to_error(self):
    interrupter = InterrupterStub()

    class InterruptingProcessor:
      def process(self, process_files, log_date):
        del process_files, log_date
        interrupter.interrupt()
        raise Exception('interrupted')

    with tempfile.TemporaryDirectory() as temp_dir:
      check_root = pathlib.Path(temp_dir) / 'input'
      error_root = pathlib.Path(temp_dir) / 'error'
      check_root.mkdir()
      error_root.mkdir()
      input_file = check_root / 'RImpression_20260913120000-000000-000001.csv'
      input_file.write_text('header\n')
      config = CLICKHOUSE_UPLOADER.Config()
      config.batch = 1
      config.check_roots = [str(check_root)]
      config.error_root = str(error_root)

      CLICKHOUSE_UPLOADER.check_stat_files(
        interrupter,
        config = config,
        logger = mock.Mock(),
        processors = {'RImpression': InterruptingProcessor()})

      self.assertTrue(input_file.exists())
      self.assertEqual(list(error_root.iterdir()), [])

  def test_graceful_shutdown_signals_and_reaps_worker(self):
    logger = mock.Mock()
    with mock.patch.object(CLICKHOUSE_UPLOADER.os, 'killpg') as killpg:
      with mock.patch.object(
          CLICKHOUSE_UPLOADER.os, 'waitpid', return_value = (101, 0)) as waitpid:
        with mock.patch.object(
            CLICKHOUSE_UPLOADER, 'process_group_exists', return_value = False):
          with mock.patch.object(CLICKHOUSE_UPLOADER.time, 'monotonic', return_value = 0):
            CLICKHOUSE_UPLOADER.shutdown_route_workers(
              {101: '/input'}, {101}, logger, timeout = 1)

    killpg.assert_called_once_with(101, CLICKHOUSE_UPLOADER.signal.SIGTERM)
    waitpid.assert_called_once_with(101, CLICKHOUSE_UPLOADER.os.WNOHANG)

  def test_forked_route_worker_is_ready_in_own_process_group(self):
    config = CLICKHOUSE_UPLOADER.Config()
    workers = {}
    with tempfile.TemporaryDirectory() as temp_dir:
      check_root = str(pathlib.Path(temp_dir) / 'missing')
      worker_pid = CLICKHOUSE_UPLOADER.fork_route_worker(
        check_root, config, mock.Mock(), {}, workers)
      try:
        self.assertEqual(workers, {worker_pid: check_root})
        self.assertEqual(CLICKHOUSE_UPLOADER.os.getpgid(worker_pid), worker_pid)
      finally:
        CLICKHOUSE_UPLOADER.shutdown_route_workers(
          workers, set(workers), mock.Mock(), timeout = 2)

  def test_shutdown_terminates_complete_worker_process_group(self):
    ready_read_fd, ready_write_fd = CLICKHOUSE_UPLOADER.os.pipe()
    worker_pid = CLICKHOUSE_UPLOADER.os.fork()
    if worker_pid == 0:
      CLICKHOUSE_UPLOADER.os.close(ready_read_fd)
      try:
        CLICKHOUSE_UPLOADER.os.setsid()
        child_process = subprocess.Popen(['sleep', '60'])
        CLICKHOUSE_UPLOADER.os.write(ready_write_fd, b'1')
        CLICKHOUSE_UPLOADER.os.close(ready_write_fd)
        while True:
          CLICKHOUSE_UPLOADER.signal.pause()
      except BaseException:
        CLICKHOUSE_UPLOADER.os._exit(1)

    CLICKHOUSE_UPLOADER.os.close(ready_write_fd)
    try:
      self.assertEqual(CLICKHOUSE_UPLOADER.os.read(ready_read_fd, 1), b'1')
      CLICKHOUSE_UPLOADER.shutdown_route_workers(
        {worker_pid: '/input'}, {worker_pid}, mock.Mock(), timeout = 2)
      self.assertFalse(CLICKHOUSE_UPLOADER.process_group_exists(worker_pid))
    finally:
      CLICKHOUSE_UPLOADER.os.close(ready_read_fd)
      try:
        CLICKHOUSE_UPLOADER.os.killpg(worker_pid, CLICKHOUSE_UPLOADER.signal.SIGKILL)
      except ProcessLookupError:
        pass
      try:
        CLICKHOUSE_UPLOADER.os.waitpid(worker_pid, 0)
      except ChildProcessError:
        pass

  def test_shutdown_force_kills_and_reaps_stuck_workers(self):
    logger = mock.Mock()
    workers = {101: '/first', 102: '/second'}
    with mock.patch.object(CLICKHOUSE_UPLOADER.os, 'killpg') as killpg:
      with mock.patch.object(CLICKHOUSE_UPLOADER.os, 'kill') as kill_process:
        with mock.patch.object(
            CLICKHOUSE_UPLOADER.os,
            'waitpid',
            side_effect = lambda pid, options: (pid, options)) as waitpid:
          with mock.patch.object(CLICKHOUSE_UPLOADER.time, 'monotonic', return_value = 0):
            CLICKHOUSE_UPLOADER.shutdown_route_workers(
              workers, set(workers), logger, timeout = 0)

    expected_signals = {
      (101, CLICKHOUSE_UPLOADER.signal.SIGTERM),
      (102, CLICKHOUSE_UPLOADER.signal.SIGTERM),
      (101, CLICKHOUSE_UPLOADER.signal.SIGKILL),
      (102, CLICKHOUSE_UPLOADER.signal.SIGKILL),
    }
    actual_signals = {call.args for call in killpg.call_args_list}
    self.assertEqual(actual_signals, expected_signals)
    self.assertEqual(
      {call.args for call in kill_process.call_args_list},
      {
        (101, CLICKHOUSE_UPLOADER.signal.SIGKILL),
        (102, CLICKHOUSE_UPLOADER.signal.SIGKILL),
      })
    self.assertEqual(waitpid.call_count, 2)


if __name__ == '__main__':
  unittest.main()
