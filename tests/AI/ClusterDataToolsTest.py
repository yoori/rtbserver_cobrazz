#!/usr/bin/python3.12

import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from ai_agent.ClusterDataTools import PostgresClient
from ai_agent.ClusterDataTools import normalize_select_query
from ai_agent.ClusterDataTools import parse_relation_name


class ClusterDataToolsTest(unittest.TestCase):
  def test_accepts_single_select_or_with_query(self):
    self.assertEqual(
      "SELECT ';' AS value",
      normalize_select_query("SELECT ';' AS value;"))
    self.assertEqual(
      '/* comment */ WITH source AS (SELECT 1) SELECT * FROM source',
      normalize_select_query(
        '/* comment */ WITH source AS (SELECT 1) SELECT * FROM source'))

  def test_rejects_writes_and_multiple_statements(self):
    with self.assertRaisesRegex(ValueError, 'only SELECT'):
      normalize_select_query('DELETE FROM users')
    with self.assertRaisesRegex(ValueError, 'multiple'):
      normalize_select_query('SELECT 1; SELECT 2')

  def test_parses_relation_names(self):
    self.assertEqual(
      ('public', 'campaign'),
      parse_relation_name('campaign', 'public'))
    self.assertEqual(
      ('stat', 'campaign'),
      parse_relation_name('stat.campaign', 'public'))
    with self.assertRaisesRegex(ValueError, 'namespace.name'):
      parse_relation_name('public.campaign;DROP', 'public')

  def test_parses_postgres_csv_and_truncates_rows(self):
    client = PostgresClient(
      'dbname=stat user=ro',
      max_rows=1)
    self.assertEqual({
      'columns': ['id', 'value'],
      'rows': [['1', None]],
      'row_count': 1,
      'truncated': True,
    }, client._parse_csv(b'id,value\n1,\\N\n2,text\n'))

  def test_connection_string_is_converted_to_libpq_environment(self):
    self.assertEqual({
      'PGHOST': 'postdb00',
      'PGPORT': '5432',
      'PGDATABASE': 'stat',
      'PGUSER': 'ro',
      'PGPASSWORD': 'secret',
    }, PostgresClient._connection_environment(
      'host=postdb00 port=5432 dbname=stat user=ro password=secret'))


if __name__ == '__main__':
  unittest.main()
