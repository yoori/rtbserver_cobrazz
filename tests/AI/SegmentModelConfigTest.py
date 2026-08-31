#!/usr/bin/python3.12

import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelConfig import SegmentModelConfig


class SegmentModelConfigTest(unittest.TestCase):
  def test_loads_nested_configuration(self):
    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [10, 60],
        'n_values': [1, 2],
        'batch_workers': 2,
        'ready_batches': 3,
      },
      'model': {
        'candidates': 4,
        'forest': {'trees': 3, 'depth': 2, 'features_per_node': 2},
      },
      'temperatures': {
        'url': {'start': 3.0, 'end': 0.2, 'schedule': 'linear'},
      },
    })
    self.assertEqual((10, 60), config.data.windows_seconds)
    self.assertEqual(3, config.model.forest.trees)
    self.assertEqual('poisson', config.model.forest.bootstrap)
    self.assertEqual('linear', config.url_temperature.schedule)

  def test_rejects_too_small_ready_queue(self):
    with self.assertRaisesRegex(ValueError, 'ready_batches'):
      SegmentModelConfig.from_dict({'data': {'batch_workers': 3, 'ready_batches': 2}})


if __name__ == '__main__':
  unittest.main()
