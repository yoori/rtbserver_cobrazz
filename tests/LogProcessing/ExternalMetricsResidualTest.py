#!/usr/bin/env python3.12

import csv
import datetime
import json
import pathlib
import sys
import tempfile
import types
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'bin'))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'lib'))

from rtbserver_utils import ExternalMetricsResidual as residual
from YandexPostClickImporter import Application


class ClickHouse:
  def __init__(self):
    self.tables = {}
    self.fail_residual = False

  def insert(self, table, rows, column_names):
    if table == residual.RESIDUAL and self.fail_residual:
      self.fail_residual = False
      raise RuntimeError('crash after CSV publication')
    self.tables.setdefault(table, []).extend(dict(zip(column_names, row)) for row in rows)

  def final(self, table):
    keys = {
      residual.RESIDUAL: ('metrika_id', 'hour', 'cc_id', 'source_date',
        'target_scope', 'campaign_id', 'ccg_id'),
      residual.OUTBOX: ('batch_id',),
      residual.CLOSED: ('metrika_id', 'source_date'),
      residual.BOOTSTRAP: ('metrika_id',),
      residual.COVERAGE: ('metrika_id', 'source_date', 'visit_id'),
      'YandexPostClickLogsSyncV3': ('metrika_id', 'source_date', 'visit_id'),
    }[table]
    final = {}
    for row in self.tables.get(table, []):
      key = tuple(row.get(column) for column in keys)
      if row.get('version', 0) >= final.get(key, {}).get('version', 0):
        final[key] = row
    return list(final.values())

  def query(self, sql, parameters=None):
    parameters = parameters or {}
    table = sql.split(' FROM ')[1].split()[0]
    rows = self.final(table)
    if 'id' in parameters:
      rows = [r for r in rows if r['metrika_id'] == parameters['id']]
    if 'dates' in parameters:
      rows = [r for r in rows if r['source_date'] in parameters['dates']]
    if 'done = 0' in sql:
      rows = [r for r in rows if r['done'] == 0]
    if 'ORDER BY version' in sql:
      rows.sort(key=lambda r: r['version'])
    columns = sql[7:sql.index(' FROM ')].split(', ')
    return types.SimpleNamespace(result_rows=[
      tuple(1 if c == '1' else r[c] for c in columns) for r in rows])


class Postgres:
  def __init__(self):
    self.mapping = [(30, 10, 20, 'UTC')]
    self.seed = []

  def cursor(self):
    return self

  def __enter__(self):
    return self

  def __exit__(self, *args):
    pass

  def execute(self, query, parameters):
    if not query.startswith('SELECT '):
      raise AssertionError('Importer must not write PG statistics directly')
    self.result = self.seed if 'externalpostclickresidualseed' in query else self.mapping

  def fetchall(self):
    return self.result

  def commit(self):
    pass


class ExternalMetricsResidualTest(unittest.TestCase):
  def setUp(self):
    self.directory = tempfile.TemporaryDirectory()
    self.addCleanup(self.directory.cleanup)
    root = pathlib.Path(self.directory.name)
    (root / 'tmp').mkdir()
    (root / 'out').mkdir()
    self.app = types.SimpleNamespace(
      ch=ClickHouse(), pg=Postgres(), logs_expiry_hours=72, verify_running=lambda: None,
      in_dir=None, out_dir=None, markers_dir=None, tmp_dir=str(root / 'tmp'),
      estimation_dir=str(root / 'out'), running=True, print_line=0,
      print_=lambda *args, **kwargs: None, _csv_line=Application._csv_line)
    self.publisher = residual.ResidualPublisher(self.app)
    self.date = datetime.datetime.now(residual.UTC).date() - datetime.timedelta(days=1)
    self.hour = datetime.datetime.combine(self.date, datetime.time(12), residual.UTC)
    self.api = types.SimpleNamespace(timezone=lambda: residual.UTC)

  def reporting(self, metrics):
    return dict(rows={(self.hour, 30): metrics}, date1=self.date, date2=self.date)

  def snapshot(self, metrics):
    return dict(metrika_id=17, hour=self.hour.isoformat(), cc_id=30,
      source_date=self.date.isoformat(), period=self.date.isoformat(),
      adv_sdate=self.date.isoformat(), campaign_id=10, ccg_id=20, metrics=metrics,
      deadline=self.publisher.deadline(self.date, residual.UTC))

  def csv_rows(self):
    rows = []
    for path in sorted(pathlib.Path(self.app.estimation_dir).iterdir()):
      with path.open() as source:
        rows.extend(csv.DictReader(source))
    return rows

  def cover(self, visit=1):
    payload = json.dumps(dict(landing_bounced=True, landing_session_time=12,
      landing_page_views=2, landing_is_new_user=False))
    records = [(visit, self.date, self.hour, 'fake-request-with-no-RIM-profile', 0, payload, True)]
    self.publisher.remember(17, records, {visit: 30})
    return records

  def test_deadline_is_72_hours_after_counter_midnight(self):
    zone = datetime.timezone(datetime.timedelta(hours=3))
    self.assertEqual(self.publisher.deadline(datetime.date(2026, 10, 1), zone),
      '2026-10-04T21:00:00+00:00')

  def test_remainder_never_creates_negative_estimate(self):
    self.assertEqual(residual.remainder([1, 0, 0, 0, 2, 1, 0], [2, 2, 0, 0, 20, 4, 1]),
      [0] * 7)
    self.assertEqual(residual.remainder([3, 3, 3, 3, 30, 3, 3], [2, 0, 0, 0, 0, 0, 0]),
      [1, 1, 1, 1, 30, 3, 1])

  def test_half_hour_counter_timezone_keeps_local_source_date(self):
    zone = datetime.timezone(datetime.timedelta(hours=5, minutes=30))
    local_start = datetime.datetime.combine(self.date, datetime.time(), zone)
    self.hour = local_start.astimezone(residual.UTC).replace(minute=0)
    report = self.reporting([1, 1, 0, 0, 12, 2, 0])
    report['source_dates'] = {(self.hour, 30): self.date}
    payload = json.dumps(dict(landing_bounced=True, landing_session_time=12,
      landing_page_views=2, landing_is_new_user=False))
    records = [(1, self.date, local_start + datetime.timedelta(minutes=45),
      'request', 0, payload, True)]
    self.publisher.remember(17, records, {1: 30}, zone)
    self.publisher.reconcile(17, types.SimpleNamespace(timezone=lambda: zone), report)
    self.assertEqual(self.csv_rows(), [])

  def test_logs_before_reporting_do_not_create_negative_E(self):
    self.cover()
    self.publisher.reconcile(17, self.api, self.reporting([0] * 7))
    self.assertEqual(self.csv_rows(), [])
    self.publisher.reconcile(17, self.api, self.reporting([1, 1, 0, 0, 12, 2, 0]))
    self.assertEqual(self.csv_rows(), [])

  def test_no_RIM_ack_required_and_repeated_visit_does_not_subtract_twice(self):
    report = self.reporting([3, 2, 0, 0, 40, 5, 1])
    self.publisher.reconcile(17, self.api, report)
    self.cover()
    self.cover()
    self.publisher.reconcile(17, self.api, report)
    self.publisher.reconcile(17, self.api, report)
    self.assertEqual([int(r['visits']) for r in self.csv_rows()], [3, -1])
    self.assertEqual(self.publisher.rows(17)[0]['metrics'], [2, 1, 0, 0, 28, 3, 1])

  def test_expiry_clears_entire_vector_once_at_exact_boundary(self):
    row = self.snapshot([3, 2, 1, 2, 40.5, 5, 1])
    self.publisher.publish([], [row])
    deadline = residual.utc(row['deadline'])
    self.publisher.finalize(deadline - datetime.timedelta(microseconds=1))
    self.assertEqual(len(self.csv_rows()), 1)
    self.publisher.finalize(deadline)
    self.publisher.finalize(deadline + datetime.timedelta(days=1))
    rows = self.csv_rows()
    self.assertEqual(len(rows), 2)
    self.assertEqual([float(rows[1][c]) for c in residual.METRICS],
      [-3, -2, -1, -2, -40.5, -5, -1])
    self.assertEqual(self.publisher.rows(17)[0]['metrics'], [0] * 7)

  def test_closed_period_cannot_be_resurrected_by_reporting(self):
    row = self.snapshot([3, 1, 0, 0, 20, 3, 1])
    self.publisher.publish([], [row])
    self.publisher.finalize(residual.utc(row['deadline']))
    self.publisher.reconcile(17, self.api, self.reporting([100, 30, 0, 0, 300, 100, 50]))
    self.assertEqual(self.publisher.rows(17)[0]['metrics'], [0] * 7)
    self.assertEqual(len(self.csv_rows()), 2)

  def test_periods_share_PG_key_but_expire_independently(self):
    first = self.snapshot([3, 1, 0, 0, 20, 3, 1])
    next_date = self.date + datetime.timedelta(days=1)
    second = dict(first, source_date=next_date.isoformat(), period=next_date.isoformat(),
      deadline=self.publisher.deadline(next_date, residual.UTC))
    self.publisher.publish([], [first, second])
    self.publisher.finalize(residual.utc(first['deadline']))
    rows = self.csv_rows()
    self.assertEqual([int(row['visits']) for row in rows], [3, 3, -3])
    self.assertTrue(all('source_date' not in row for row in rows))
    keys = [tuple(row[key] for key in residual.CSV_HEADER
      if key not in ('batch_id', *residual.METRICS)) for row in rows]
    self.assertTrue(all(key == keys[0] for key in keys))
    self.assertEqual(len(self.publisher.rows(17)), 2)
    self.assertEqual(sum(row['metrics'][0] for row in self.publisher.rows(17)), 3)
    self.publisher.finalize(residual.utc(second['deadline']))
    self.assertEqual(sum(int(row['visits']) for row in self.csv_rows()), 0)

  def test_crash_after_csv_replays_same_batch_identity(self):
    self.app.ch.fail_residual = True
    with self.assertRaisesRegex(RuntimeError, 'crash after CSV'):
      self.publisher.publish([], [self.snapshot([3, 1, 0, 0, 20, 3, 1])])
    residual.ResidualPublisher(self.app).drain()
    rows = self.csv_rows()
    self.assertEqual(len(rows), 2)
    self.assertEqual(rows[0]['batch_id'], rows[1]['batch_id'])
    self.assertEqual(self.publisher.rows(17)[0]['metrics'][0], 3)
    self.publisher.drain()
    self.assertEqual(len(self.csv_rows()), 2)

  def test_original_keys_survive_creative_reparenting(self):
    report = self.reporting([3, 1, 0, 0, 40, 5, 1])
    self.publisher.reconcile(17, self.api, report)
    self.app.pg.mapping = [(30, 100, 200, 'Asia/Tokyo')]
    self.cover()
    self.publisher.reconcile(17, self.api, report)
    delta = self.csv_rows()[-1]
    self.assertEqual((delta['campaign_id'], delta['ccg_id']), ('10', '20'))
    self.assertEqual(delta['adv_sdate'], self.date.isoformat())

  def test_unavailable_legacy_coverage_freezes_migration_but_not_expiry(self):
    row = dict(self.snapshot([3, 1, 0, 0, 20, 3, 1]), target_scope='ccgpostclickstatsdaily')
    self.publisher.publish([], [row])
    self.app.ch.insert('YandexPostClickLogsSyncV3', [(17, self.date, 10)],
      ('metrika_id', 'source_date', 'visit_id'))
    self.publisher.reconcile(17, self.api, self.reporting([100, 0, 0, 0, 100, 100, 0]))
    self.assertEqual(len(self.csv_rows()), 1)
    self.publisher.finalize(residual.utc(row['deadline']))
    self.assertEqual(self.csv_rows()[-1]['target_scope'], 'ccgpostclickstatsdaily')
    self.assertEqual(self.publisher.rows(17)[0]['metrics'], [0] * 7)

  def test_reconstructed_coverage_replaces_migrated_scopes(self):
    row = dict(self.snapshot([3, 1, 0, 0, 20, 3, 1]), target_scope='ccgpostclickstatsdaily')
    self.publisher.publish([], [row])
    self.publisher.reconcile(17, self.api, self.reporting([5, 1, 0, 0, 20, 5, 1]))
    last = self.csv_rows()[1:]
    self.assertEqual({(r['target_scope'], int(r['visits'])) for r in last},
      {('ccgpostclickstatsdaily', -3), ('', 5)})

  def test_removed_reporting_row_emits_negative_delta(self):
    self.publisher.reconcile(17, self.api, self.reporting([3, 1, 0, 0, 20, 3, 1]))
    self.publisher.reconcile(17, self.api, dict(rows={}, date1=self.date, date2=self.date))
    self.assertEqual(self.csv_rows()[-1]['visits'], '-3')

  def test_counter_identity_is_not_a_PG_statistics_key(self):
    first = self.snapshot([2, 0, 0, 0, 20, 2, 0])
    second = dict(first, metrika_id=99)
    self.publisher.publish([], [first, second])
    rows = self.csv_rows()
    self.assertEqual(len(rows), 2)
    self.assertEqual({row['external_metrics_connection_type_id'] for row in rows}, {'1'})
    self.assertTrue(all('metrika_id' not in row for row in rows))
    self.assertTrue(all('external_metrics_connection_id' not in row for row in rows))
    self.assertEqual(len(self.publisher.rows()), 2)

  def test_bootstrap_replay_does_not_add_seed_twice(self):
    self.app.pg.seed = [(17, 'ccgpostclickstatsdaily', self.hour, 30, self.date, self.date,
      0, 20, 3, 1, 0, 0, 20, 3, 1)]
    self.app.ch.fail_residual = True
    with self.assertRaisesRegex(RuntimeError, 'crash after CSV'):
      self.publisher.bootstrap(17, residual.UTC)
    self.publisher.bootstrap(17, residual.UTC)
    self.publisher.bootstrap(17, residual.UTC)
    self.assertEqual(self.publisher.rows(17)[0]['metrics'][0], 3)
    self.assertEqual(self.csv_rows(), [])

  def test_partial_legacy_bootstrap_preserves_saved_keys_without_double_seed(self):
    row = self.snapshot([3, 1, 0, 0, 20, 3, 1])
    self.publisher._save_rows([row], 1)
    self.app.legacy_residual_keys = {residual.legacy_seed_key(10, row)}
    self.app.pg.seed = [
      (connection, '', self.hour, 30, self.date, self.date, 10, 20, 3, 1, 0, 0, 20, 3, 1)
      for connection in (10, 11)]
    self.publisher.bootstrap(17, residual.UTC)
    self.publisher.bootstrap(17, residual.UTC)
    self.assertEqual(self.publisher.rows(17)[0]['metrics'][0], 6)
    self.assertEqual(self.csv_rows(), [])


if __name__ == '__main__':
  unittest.main()
