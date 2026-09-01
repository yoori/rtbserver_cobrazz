#!/usr/bin/python3.12

import pathlib
import sys
import unittest

import numpy


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelConfig import SegmentModelConfig
from segment_model.SyntheticSegmentData import generate_synthetic_dataset
from segment_model.UrlHash import url_bucket


class SyntheticSegmentDataTest(unittest.TestCase):
  def test_builds_chronological_history_and_split(self):
    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [10, 100],
        'n_values': [1, 2],
        'url_buckets': 64,
        'batch_workers': 1,
        'ready_batches': 1,
      },
      'model': {
        'candidates': 4,
        'forest': {'trees': 2, 'depth': 2, 'features_per_node': 2},
      },
      'synthetic': {
        'samples': 40,
        'users': 5,
        'urls': 10,
        'true_segments': 2,
        'existing_channels': 2,
        'events_per_user': 20,
        'horizon_seconds': 1000,
        'validation_fraction': 0.25,
        'final_test_fraction': 0.25,
      },
    })
    dataset = generate_synthetic_dataset(config)
    expected_buckets = {url_bucket(url, config.data.url_buckets) for url in dataset.urls}
    self.assertEqual((40, len(expected_buckets), 2), dataset.history_counts.shape)
    self.assertEqual((40, 2), dataset.existing_channels.shape)
    self.assertEqual(20, len(dataset.train_indices))
    self.assertEqual(10, len(dataset.validation_indices))
    self.assertEqual(10, len(dataset.final_test_indices))
    self.assertLessEqual(
      dataset.timestamps[dataset.train_indices[-1]],
      dataset.timestamps[dataset.validation_indices[0]])
    self.assertLessEqual(
      dataset.timestamps[dataset.validation_indices[-1]],
      dataset.timestamps[dataset.final_test_indices[0]])
    for rule in dataset.true_rules:
      window_index = config.data.windows_seconds.index(rule.window_seconds)
      positions = {
        int(bucket): index
        for index, bucket in enumerate(dataset.history_url_ids)
      }
      rule_positions = [
        positions[url_bucket(dataset.urls[url_id], config.data.url_buckets)]
        for url_id in rule.url_ids
      ]
      counts = dataset.history_counts[:, rule_positions, window_index]
      activation_rate = numpy.mean(numpy.max(counts, axis=1) >= rule.min_visits)
      self.assertGreater(activation_rate, 0.05)


if __name__ == '__main__':
  unittest.main()
