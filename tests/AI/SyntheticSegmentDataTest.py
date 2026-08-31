#!/usr/bin/python3.12

import pathlib
import sys
import unittest

import numpy


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelConfig import SegmentModelConfig
from segment_model.SyntheticSegmentData import generate_synthetic_dataset


class SyntheticSegmentDataTest(unittest.TestCase):
  def test_builds_chronological_history_and_split(self):
    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [10, 100],
        'n_values': [1, 2],
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
      },
    })
    dataset = generate_synthetic_dataset(config)
    self.assertEqual((40, 10, 2), dataset.history_counts.shape)
    self.assertEqual((40, 2), dataset.existing_channels.shape)
    self.assertEqual(30, len(dataset.train_indices))
    self.assertEqual(10, len(dataset.validation_indices))
    self.assertLessEqual(
      dataset.timestamps[dataset.train_indices[-1]],
      dataset.timestamps[dataset.validation_indices[0]])
    for rule in dataset.true_rules:
      window_index = config.data.windows_seconds.index(rule.window_seconds)
      counts = dataset.history_counts[:, list(rule.url_ids), window_index]
      activation_rate = numpy.mean(numpy.max(counts, axis=1) >= rule.min_visits)
      self.assertGreater(activation_rate, 0.05)


if __name__ == '__main__':
  unittest.main()
