#!/usr/bin/python3.12

import contextlib
import io
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.ScenarioDefinition import SegmentModelScenario
from segment_model.ScenarioSeeder import seed_scenario


class FakeClickHouseClient:
  def __init__(self):
    self.rows = []

  def execute(self, query, data=None):
    if query.startswith('DROP TABLE'):
      self.rows = []
      return b''
    if query.lstrip().startswith('CREATE TABLE'):
      return b''
    if query == 'SELECT count() FROM RImpression':
      return str(len(self.rows)).encode('ascii')
    if query.lstrip().startswith('INSERT INTO'):
      self.rows.extend(line.split('\t') for line in data.decode('utf-8').splitlines())
      return b''
    if query.startswith('SELECT count(), countIf(click_timestamp IS NOT NULL)'):
      clicks = sum(int(row[3]) for row in self.rows)
      minimum = min(int(row[0]) for row in self.rows)
      maximum = max(int(row[0]) for row in self.rows)
      return ('\t'.join(map(str, (len(self.rows), clicks, minimum, maximum))) + '\n').encode(
        'ascii')
    raise AssertionError('Unexpected query: ' + query)


class ScenarioSeederTest(unittest.TestCase):
  def test_seeder_uses_the_same_click_rule_as_profiles(self):
    scenario = SegmentModelScenario.from_dict({
      'rows': 400,
      'uid_prefix': 'user-',
      'timestamp_start': 10000,
      'timestamp_step_seconds': 1,
      'validation_fraction': 0.2,
      'cohorts': [
        {
          'name': 'even',
          'uid_mod': 2,
          'uid_remainder': 0,
          'profile_variants': [
            [{'url': 'a.com', 'ages_seconds': [60]}],
            [{'url': 'b.com', 'ages_seconds': [60]}],
          ],
          'click_every_n_per_variant': 100,
        },
        {
          'name': 'odd',
          'uid_mod': 2,
          'uid_remainder': 1,
          'profile_variants': [[{'url': 'c.com', 'ages_seconds': [60]}]],
          'click_every_n_per_variant': 0,
        },
      ],
    })
    client = FakeClickHouseClient()
    with contextlib.redirect_stdout(io.StringIO()):
      seed_scenario(client, scenario, chunk_rows=70, reset=True)
    self.assertEqual(400, len(client.rows))
    self.assertEqual(2, sum(int(row[3]) for row in client.rows))
    for row in client.rows:
      numeric_uid = int(row[0])
      self.assertEqual(scenario.user_id(numeric_uid), row[2])
      self.assertEqual(scenario.clicked(numeric_uid), int(row[3]))


if __name__ == '__main__':
  unittest.main()
