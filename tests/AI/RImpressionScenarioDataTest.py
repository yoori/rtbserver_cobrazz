#!/usr/bin/python3.12

import datetime
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

import numpy


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.RImpressionScenarioData import RImpressionBatchBuilder
from segment_model.RImpressionScenarioData import build_history_counts
from segment_model.RImpressionScenarioData import make_rimpression_scenario_source
from segment_model.ScenarioDefinition import SegmentModelScenario
from segment_model.SegmentModelConfig import SegmentModelConfig
from segment_model.SegmentModelData import BatchRequest


def daily_profile(navigations):
  counts = {}
  for navigation in navigations:
    date = datetime.datetime.fromtimestamp(
      navigation['timestamp'],
      datetime.timezone.utc).strftime('%Y-%m-%d')
    key = (date, navigation['url'])
    counts[key] = counts.get(key, 0) + 1
  return [
    {'date': date, 'url': url, 'count': count}
    for (date, url), count in sorted(counts.items())
  ]


def test_scenario():
  return SegmentModelScenario.from_dict({
    'rows': 8,
    'uid_prefix': 'user-',
    'timestamp_start': 10000,
    'timestamp_step_seconds': 10,
    'validation_fraction': 0.25,
    'final_test_fraction': 0.25,
    'cohorts': [
      {
        'name': 'even',
        'uid_mod': 2,
        'uid_remainder': 0,
        'profile_variants': [
          [{'url': 'a.com', 'ages_seconds': [30, 90]}],
          [{'url': 'b.com', 'ages_seconds': [30]}],
        ],
        'click_every_n_per_variant': 2,
      },
      {
        'name': 'odd',
        'uid_mod': 2,
        'uid_remainder': 1,
        'profile_variants': [
          [{'url': 'c.com', 'ages_seconds': [30]}],
        ],
        'click_every_n_per_variant': 0,
      },
    ],
  })


class RImpressionScenarioDataTest(unittest.TestCase):
  def test_label_statistics_use_only_the_builder_range(self):
    scenario = test_scenario()
    builder = RImpressionBatchBuilder(
      scenario,
      [86400],
      'http://unused-clickhouse',
      'http://unused-expression-matcher',
      range_begin=2,
      range_end=6)
    with mock.patch(
        'segment_model.RImpressionScenarioData.ClickHouseClient.execute',
        return_value=b'4\t1\n') as execute:
      self.assertEqual((4, 1), builder.label_statistics())
    query = execute.call_args.args[0]
    self.assertIn('impression_id >= 2', query)
    self.assertIn('impression_id < 6', query)

  def test_history_counts_use_only_events_before_impression(self):
    impression = int(datetime.datetime(
      2026, 1, 3, 12, tzinfo=datetime.timezone.utc).timestamp())
    counts = build_history_counts(
      [
        {'url': 'a.com', 'date': '2026-01-02', 'count': 2},
        {'url': 'a.com', 'date': '2026-01-01', 'count': 3},
        {'url': 'a.com', 'date': '2026-01-03', 'count': 100},
        {'url': 'b.com', 'date': '2026-01-04', 'count': 100},
      ],
      impression,
      {'a.com': 0, 'b.com': 1},
      [86400, 172800])
    numpy.testing.assert_array_equal(counts, [[2.0, 5.0], [0.0, 0.0]])

  def test_batch_builder_validates_clicks_and_builds_groups(self):
    scenario = test_scenario()
    builder = RImpressionBatchBuilder(
      scenario,
      [86400, 172800],
      'http://unused-clickhouse',
      'http://unused-expression-matcher')
    impressions = []
    profiles = []
    for numeric_uid in range(scenario.rows):
      timestamp = scenario.timestamp(numeric_uid)
      sample = scenario.sample(numeric_uid, timestamp)
      impressions.append((
        numeric_uid,
        timestamp,
        scenario.user_id(numeric_uid),
        sample['clicked']))
      profiles.append(daily_profile(sample['navigations']))
    batch = builder._make_batch(impressions, profiles)
    self.assertEqual((8, 3, 2), batch.history_counts.shape)
    self.assertEqual([0, 1, 0, 1, 0, 1, 0, 1], batch.group_ids.tolist())
    self.assertEqual([0, 0, 1, 0, 0, 0, 1, 0], batch.variant_ids.tolist())
    self.assertEqual([1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0], batch.labels.tolist())

  def test_profile_requests_are_batched_by_impression_day(self):
    scenario = test_scenario()
    builder = RImpressionBatchBuilder(
      scenario,
      [86400],
      'http://unused-clickhouse',
      ['http://em-1', 'http://em-2'])
    day = 1767312000
    impressions = [
      (0, day + 10, 'user-0', 1),
      (1, day + 20, 'user-1', 0),
      (2, day + 86410, 'user-2', 1),
    ]

    def profiles(user_ids, timestamp):
      return {user_id: [{'date': '2026-01-01', 'url': 'a.com', 'count': 1}]
              for user_id in user_ids}

    with mock.patch(
        'segment_model.RImpressionScenarioData.ExpressionMatcherClient.profiles',
        side_effect=profiles) as request:
      result = builder._read_profiles(impressions)
    self.assertEqual(3, len(result))
    self.assertEqual(2, request.call_count)
    self.assertEqual((['user-0', 'user-1'], day), request.call_args_list[0].args)
    self.assertEqual((['user-2'], day + 86400), request.call_args_list[1].args)

  def test_batch_builder_rejects_a_mismatched_click(self):
    scenario = test_scenario()
    builder = RImpressionBatchBuilder(
      scenario,
      [86400],
      'http://unused-clickhouse',
      'http://unused-expression-matcher')
    timestamp = scenario.timestamp(0)
    profile = daily_profile(scenario.sample(0, timestamp)['navigations'])
    with self.assertRaisesRegex(RuntimeError, 'click does not match'):
      builder._make_batch([(0, timestamp, 'user-0', 0)], [profile])

  def test_prepared_batch_cache_avoids_source_reload(self):
    scenario = test_scenario()
    with tempfile.TemporaryDirectory() as cache_dir:
      builder = RImpressionBatchBuilder(
        scenario,
        [86400],
        'http://unused-clickhouse',
        'http://unused-expression-matcher',
        batch_rows=4,
        range_begin=4,
        range_end=8,
        cache_dir=cache_dir,
        cache_prefix='validation')
      timestamp = scenario.timestamp(4)
      sample = scenario.sample(4, timestamp)
      batch = builder._make_batch(
        [(4, timestamp, 'user-4', sample['clicked'])],
        [daily_profile(sample['navigations'])])
      cache_file = builder._cache_file(0)
      builder._write_cache(cache_file, batch)
      restored = builder(BatchRequest(9, 0, True))
    numpy.testing.assert_array_equal(batch.history_counts, restored.history_counts)
    numpy.testing.assert_array_equal(batch.sample_indices, restored.sample_indices)

  def test_streaming_source_uses_chronological_ranges_without_materialization(self):
    scenario = test_scenario()
    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [86400],
        'n_values': [1],
      },
      'model': {
        'candidates': 2,
        'membership': {'initialization': 'random_single_seed'},
      },
    })
    with tempfile.TemporaryDirectory() as cache_dir, mock.patch(
        'segment_model.RImpressionScenarioData._verify_rimpression_source'):
      source = make_rimpression_scenario_source(
        config,
        scenario,
        'http://clickhouse',
        'http://expression-matcher',
        batch_rows=2,
        cache_dir=cache_dir)
    self.assertIsNone(source.dataset.history_counts)
    self.assertEqual((0, 4), (
      source.training_builder.range_begin,
      source.training_builder.range_end))
    self.assertEqual((4, 6), (
      source.validation_builder.range_begin,
      source.validation_builder.range_end))
    self.assertEqual((6, 8), (
      source.final_test_builder.range_begin,
      source.final_test_builder.range_end))
    self.assertEqual(2, source.training_builder.batches_per_epoch)
    self.assertEqual(1, source.validation_builder.batches_per_epoch)
    self.assertEqual(1, source.final_test_builder.batches_per_epoch)


if __name__ == '__main__':
  unittest.main()
