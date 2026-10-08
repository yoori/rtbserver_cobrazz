#!/usr/bin/env python3.12

"""Opt-in integration tests using a disposable, network-isolated ClickHouse container."""

import datetime
import json
import logging
import os
import pathlib
import subprocess
import sys
import types
import unittest
import uuid

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'lib'))
sys.path.insert(0, str(ROOT / 'bin'))

from clickhouse_connect.driver.binding import finalize_query
from rtbserver_utils import ExternalMetricsResidual as residual
from rtbserver_utils import YandexPostClickConversions as conversions
from rtbserver_utils.YandexPostClickMigration import migrate
from YandexPostClickImporter import INTERNAL_CLICKHOUSE_DDL
import ClickhouseStatUploader as uploader


class DockerClickHouse:
  def __init__(self, container, database):
    self.container = container
    self.database = database

  def execute(self, sql):
    return subprocess.run(['docker', 'exec', '-i', self.container, 'clickhouse-client',
      '--database', self.database, '--date_time_input_format=best_effort'], input=sql,
      text=True, capture_output=True, check=True).stdout

  def command(self, sql):
    result = self.execute(sql).strip()
    return int(result) if result.isdecimal() else result

  def query(self, sql, parameters=None):
    data = json.loads(self.execute(finalize_query(sql, parameters, residual.UTC) + ' FORMAT JSON'))
    def value(raw, kind):
      if kind.startswith(('UInt', 'Int')):
        return int(raw)
      if kind == 'Date':
        return datetime.date.fromisoformat(raw)
      if kind.startswith('DateTime'):
        return datetime.datetime.fromisoformat(raw).replace(tzinfo=residual.UTC)
      return raw
    return types.SimpleNamespace(result_rows=[
      tuple(value(row[column['name']], column['type']) for column in data['meta'])
      for row in data['data']])

  def insert(self, table, rows, column_names):
    data = '\n'.join(json.dumps(dict(zip(column_names, row)), default=str) for row in rows)
    self.execute(f'INSERT INTO {table} FORMAT JSONEachRow\n' + data)


class Postgres:
  def cursor(self):
    return self

  def __enter__(self):
    return self

  def __exit__(self, *args):
    pass

  def execute(self, sql):
    assert sql.startswith('SELECT ')

  def fetchall(self):
    return [(17, 111), (18, 111)]

  def commit(self):
    pass


@unittest.skipUnless(
  os.getenv('YANDEX_MIGRATION_CH_CONTAINER'), 'requires test ClickHouse container')
class YandexPostClickMigrationTest(unittest.TestCase):
  def setUp(self):
    self.ch = DockerClickHouse(os.environ['YANDEX_MIGRATION_CH_CONTAINER'], 'default')
    database = 'metrics_types_test_' + uuid.uuid4().hex
    self.ch.command('CREATE DATABASE ' + database)
    self.ch.database = database
    self.addCleanup(self.ch.command, 'DROP DATABASE ' + database)
    self.app = types.SimpleNamespace(ch=self.ch, pg=Postgres())
    for statement in INTERNAL_CLICKHOUSE_DDL + residual.DDL + conversions.DDL:
      self.ch.command(statement)
    self.date = datetime.date(2026, 10, 1)
    self.time = datetime.datetime(2026, 10, 1, tzinfo=residual.UTC)

  def test_migration_collapses_counter_state_but_sums_existing_E(self):
    for new in ('YandexPostClickLogsSyncV3', 'YandexPostClickLogRequestsV3'):
      old = new.replace('V3', 'V2')
      sql = next(q for q in INTERNAL_CLICKHOUSE_DDL if 'CREATE TABLE' in q and new in q)
      sql = sql.replace(new, old).replace('metrika_id', 'external_metrics_connection_id')
      sql = sql.replace('source_date, request_id)', 'source_date)')
      self.ch.command(sql)
    self.ch.insert('YandexPostClickLogsSyncV2', [
      (17, 123, self.date, 'request', 0, 1, self.time),
      (18, 123, self.date, 'request', 0, 1, self.time)],
      ('external_metrics_connection_id', 'visit_id', 'source_date', 'request_id', 'payload_hash',
        'reporting_comparable', 'published_at'))
    self.ch.insert('YandexPostClickLogRequestsV2', [
      (17, self.date, 100, 'created', self.time), (18, self.date, 101, 'created', self.time)],
      ('external_metrics_connection_id', 'source_date', 'request_id', 'status', 'version'))
    self.ch.command("CREATE TABLE ExternalMetricsResidual (connection UInt64, payload String) "
      'ENGINE=ReplacingMergeTree ORDER BY connection')
    rows = []
    for connection in (17, 18):
      row = dict(connection_id=connection, hour=self.time.isoformat(), cc_id=30,
        source_date=self.date.isoformat(), period=self.date.isoformat(),
        target_scope='', campaign_id=10, ccg_id=20, adv_sdate=self.date.isoformat(),
        metrics=[3, 1, 0, 0, 20.5, 4, 1], deadline='2026-10-05T00:00:00+00:00')
      rows.append((connection, json.dumps(row)))
    self.ch.insert('ExternalMetricsResidual', rows, ('connection', 'payload'))
    migrate(self.app)
    migrate(self.app)
    self.assertEqual(self.ch.query(
      'SELECT metrika_id, visit_id FROM YandexPostClickLogsSyncV3 FINAL').result_rows, [(111, 123)])
    self.assertEqual(self.ch.query('SELECT request_id FROM YandexPostClickLogRequestsV3 FINAL '
      'ORDER BY request_id').result_rows, [(100,), (101,)])
    rows = residual.ResidualPublisher(self.app).rows(111)
    self.assertEqual(len(rows), 1)
    self.assertEqual(rows[0]['metrics'], [6, 2, 0, 0, 41, 8, 2])

  def test_pending_legacy_outbox_prevents_partial_upgrade(self):
    self.ch.command('CREATE TABLE ExternalMetricsResidualOutbox '
      '(done UInt8) ENGINE=ReplacingMergeTree ORDER BY done')
    self.ch.insert('ExternalMetricsResidualOutbox', [(0,)], ('done',))
    with self.assertRaisesRegex(RuntimeError, 'Drain the previous importer'):
      migrate(self.app)

  def test_interrupted_V2_upgrade_does_not_lose_published_visits(self):
    sql = next(q for q in INTERNAL_CLICKHOUSE_DDL
      if 'CREATE TABLE' in q and 'YandexPostClickLogsSyncV3' in q)
    self.ch.command(sql.replace('YandexPostClickLogsSyncV3', 'YandexPostClickLogsSyncV2')
      .replace('metrika_id', 'external_metrics_connection_id'))
    self.ch.command(sql.replace('YandexPostClickLogsSyncV3', 'YandexPostClickLogsSync')
      .replace('metrika_id', 'ymref_id').replace('source_date', 'event_date'))
    self.ch.insert('YandexPostClickLogsSync', [(17, 123, self.date, 'request', 0, 1, self.time)],
      ('ymref_id', 'visit_id', 'event_date', 'request_id', 'payload_hash',
        'reporting_comparable', 'published_at'))
    migrate(self.app)
    self.assertEqual(self.ch.query(
      'SELECT metrika_id, visit_id FROM YandexPostClickLogsSyncV3 FINAL').result_rows, [(111, 123)])

  def test_research_migration_keeps_rows_and_converts_source_identity(self):
    self.ch.command(uploader.R_POST_CONV_CLICK_CREATE_TABLE_QUERY.replace(
      'external_metrics_connection_type_id UInt16', 'external_metrics_connection_id UInt64'))
    self.ch.insert('RPostConvClick', [('request', self.time, 'ym_111_9', 'event', 1, 0,
      17, self.date)], ('request_id', 'timestamp', 'conversion_name', 'action_name',
        'visits', 'goals', 'external_metrics_connection_id', 'source_date'))
    instance = object.__new__(uploader.RPostConvClickUploader)
    instance.create_table_query = None
    instance.clickhouse_conn = ''
    instance.logger = logging.getLogger('test')
    original = uploader.ClickhouseQueryExecutor
    uploader.ClickhouseQueryExecutor = lambda *args, **kwargs: self.ch
    try:
      instance.init_storage()
      instance.init_storage()
    finally:
      uploader.ClickhouseQueryExecutor = original
    self.assertEqual(self.ch.query('SELECT external_metrics_connection_type_id, visits '
      'FROM RPostConvClick FINAL').result_rows, [(1, 1)])


if __name__ == '__main__':
  unittest.main()
