#!/usr/bin/env python3.12

import pathlib
import sys
import unittest
import unittest.mock


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from rtbserver_utils.PostgresFeatureNameResolver import PostgresFeatureNameResolver


class PostgresFeatureNameResolverTest(unittest.TestCase):
  def test_resolves_entity_features_in_batches(self):
    cursor = unittest.mock.MagicMock()
    cursor.__enter__.return_value = cursor
    connection = unittest.mock.MagicMock()
    connection.__enter__.return_value = connection
    connection.cursor.return_value = cursor

    def execute(query, parameters):
      ids = parameters[0]
      if 'FROM account' in query:
        rows = [(123, 'Publisher', 'Publisher')]
      elif 'FROM campaigncreative AS cc' in query:
        rows = [(55, 'Advertiser', 'Creative')]
      elif 'FROM campaigncreativegroup AS ccg' in query:
        rows = [(44, 'Advertiser', 'Group')]
      elif 'WITH RECURSIVE geochannel_paths' in query:
        rows = [(428449, None, 'Russia/Dagestan/khamamatyurt')]
      elif 'FROM creativesize' in query:
        rows = [(761, None, '320 x 480')]
      elif 'WITH requested_ids AS' in query:
        rows = [
          (614065, 'Channel account', 'Channel'),
          (614066, None, 'Device'),
          (427124, None, 'Khakass'),
          (1156234, None, 'Linux'),
        ]
      else:
        rows = []
      cursor.fetchall.return_value = [
        row for row in rows if row[0] in ids
      ]

    cursor.execute.side_effect = execute
    features = [
      'publisher:123',
      'ccg:44,ccid:55',
      'channel:614065',
      'device:614066',
      'geochannel:428449',
      'channel:427124',
      'channel:1156234',
      'size:761',
      'campaignfreqlog:3',
    ]

    with unittest.mock.patch(
        'rtbserver_utils.PostgresFeatureNameResolver.psycopg2.connect',
        return_value=connection) as connect:
      result = PostgresFeatureNameResolver('postgres connection').resolve(
        features)

    connect.assert_called_once_with('postgres connection')
    self.assertEqual(6, cursor.execute.call_count)
    self.assertEqual({
      'publisher:123': 'Publisher/Publisher',
      'ccg:44,ccid:55': 'Advertiser/Group, Advertiser/Creative',
      'channel:614065': 'Channel account/Channel',
      'device:614066': 'Device',
      'geochannel:428449': 'Russia/Dagestan/khamamatyurt',
      'channel:427124': 'Khakass',
      'channel:1156234': 'Linux',
      'size:761': '320 x 480',
    }, result)

  def test_ignores_non_entity_and_non_numeric_values(self):
    resolver = PostgresFeatureNameResolver('postgres connection')
    with unittest.mock.patch(
        'rtbserver_utils.PostgresFeatureNameResolver.psycopg2.connect') as connect:
      result = resolver.resolve(['domain:example.com', 'channel:not-an-id'])

    self.assertEqual({}, result)
    connect.assert_not_called()

  def test_resolves_campaign_names_in_one_query(self):
    cursor = unittest.mock.MagicMock()
    cursor.__enter__.return_value = cursor
    cursor.fetchall.return_value = [
      (123, 'First campaign'),
      (456, 'Second campaign'),
      (789, None),
    ]
    connection = unittest.mock.MagicMock()
    connection.__enter__.return_value = connection
    connection.cursor.return_value = cursor

    with unittest.mock.patch(
        'rtbserver_utils.PostgresFeatureNameResolver.psycopg2.connect',
        return_value=connection):
      result = PostgresFeatureNameResolver(
        'postgres connection').resolve_campaign_names([456, 123, 456, 789])

    cursor.execute.assert_called_once()
    self.assertEqual(([123, 456, 789],), cursor.execute.call_args.args[1])
    self.assertEqual({
      123: 'First campaign',
      456: 'Second campaign',
    }, result)


if __name__ == '__main__':
  unittest.main()
