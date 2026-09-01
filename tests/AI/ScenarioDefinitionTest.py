#!/usr/bin/python3.12

import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.ScenarioDefinition import SegmentModelScenario


def union_scenario():
  return SegmentModelScenario.from_dict({
    'rows': 400,
    'uid_prefix': 'user-',
    'timestamp_start': '2026-01-01T00:00:00Z',
    'timestamp_step_seconds': 1,
    'validation_fraction': 0.2,
    'cohorts': [
      {
        'name': 'even-positive',
        'uid_mod': 2,
        'uid_remainder': 0,
        'profile_variants': [
          [{'url': 'a.com', 'ages_seconds': [60]}],
          [{'url': 'b.com', 'ages_seconds': [120]}],
        ],
        'click_every_n_per_variant': 100,
      },
      {
        'name': 'odd-zero',
        'uid_mod': 2,
        'uid_remainder': 1,
        'profile_variants': [
          [{'url': 'c.com', 'ages_seconds': [60]}],
          [{'url': 'd.com', 'ages_seconds': [120]}],
        ],
        'click_every_n_per_variant': 0,
      },
    ],
    'expected_segments': [
      {
        'urls': ['a.com', 'b.com'],
        'window_seconds': 3600,
        'min_visits': 1,
      },
    ],
  })


class ScenarioDefinitionTest(unittest.TestCase):
  def test_profile_variants_have_equal_click_rate(self):
    scenario = union_scenario()
    clicks_by_variant = [0, 0]
    rows_by_variant = [0, 0]
    for numeric_uid in range(scenario.rows):
      sample = scenario.sample(numeric_uid, scenario.timestamp(numeric_uid))
      if sample['cohort_name'] != 'even-positive':
        self.assertEqual(0, sample['clicked'])
        continue
      variant = sample['variant_id']
      rows_by_variant[variant] += 1
      clicks_by_variant[variant] += sample['clicked']
      self.assertLess(sample['navigations'][0]['timestamp'], scenario.timestamp(numeric_uid))
    self.assertEqual([100, 100], rows_by_variant)
    self.assertEqual([1, 1], clicks_by_variant)

  def test_scenario_round_trip_preserves_profiles(self):
    scenario = union_scenario()
    restored = SegmentModelScenario.from_dict(scenario.to_dict())
    for numeric_uid in range(8):
      timestamp = scenario.timestamp(numeric_uid)
      self.assertEqual(
        scenario.sample(numeric_uid, timestamp),
        restored.sample(numeric_uid, timestamp))
    self.assertEqual(('a.com', 'b.com', 'c.com', 'd.com'), restored.urls)

  def test_overlapping_cohorts_are_rejected(self):
    value = union_scenario().to_dict()
    value['cohorts'][1]['uid_remainder'] = 0
    with self.assertRaisesRegex(ValueError, 'exactly one'):
      SegmentModelScenario.from_dict(value)


if __name__ == '__main__':
  unittest.main()
