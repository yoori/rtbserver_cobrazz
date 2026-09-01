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
      'candidate_opening': {
        'enabled': True,
        'first_active_candidates': 2,
        'open_every_epochs': 10,
        'previous_candidate_lr_mode': 'full',
        'url_temperature_floor': 0.7,
        'joint_finetune_epochs': 5,
      },
      'temperatures': {
        'url': {'start': 3.0, 'end': 0.2, 'schedule': 'linear'},
      },
    })
    self.assertEqual((10, 60), config.data.windows_seconds)
    self.assertEqual(3, config.model.forest.trees)
    self.assertEqual('linear', config.url_temperature.schedule)
    self.assertEqual(1000000, config.data.url_buckets)
    self.assertEqual(10000, config.training.max_epochs)
    self.assertEqual(20, config.training.early_stopping_patience)
    self.assertEqual(1e-6, config.training.early_stopping_min_delta)
    self.assertEqual(0.8, config.loss.duplicate_jaccard_margin)
    self.assertEqual(0.9, config.loss.duplicate_activation_margin)
    self.assertFalse(config.loss.enable_candidate_reseed)
    self.assertTrue(config.candidate_opening.enabled)
    self.assertEqual(2, config.candidate_opening.first_active_candidates)
    self.assertEqual('full', config.candidate_opening.previous_candidate_lr_mode)
    self.assertEqual(0.7, config.candidate_opening.url_temperature_floor)

  def test_rejects_too_small_ready_queue(self):
    with self.assertRaisesRegex(ValueError, 'ready_batches'):
      SegmentModelConfig.from_dict({'data': {'batch_workers': 3, 'ready_batches': 2}})

  def test_validates_membership_seed_counts(self):
    with self.assertRaisesRegex(ValueError, 'random_single_seed'):
      SegmentModelConfig.from_dict({
        'model': {
          'membership': {
            'initialization': 'random_single_seed',
            'initial_urls_per_candidate': 2,
          },
        },
      })
    with self.assertRaisesRegex(ValueError, 'random_multi_seed'):
      SegmentModelConfig.from_dict({
        'model': {
          'membership': {
            'initialization': 'random_multi_seed',
            'initial_urls_per_candidate': 1,
          },
        },
      })

  def test_validates_duplicate_configuration(self):
    with self.assertRaisesRegex(ValueError, 'inside'):
      SegmentModelConfig.from_dict({'loss': {'duplicate_jaccard_margin': 1.1}})
    with self.assertRaisesRegex(ValueError, 'schedule'):
      SegmentModelConfig.from_dict({'loss': {'duplicate_regularization_start_epoch': -1}})
    with self.assertRaisesRegex(ValueError, 'duplicate_pairs'):
      SegmentModelConfig.from_dict({'loss': {'duplicate_pairs': -1}})
    with self.assertRaisesRegex(ValueError, 'max_epochs'):
      SegmentModelConfig.from_dict({
        'loss': {
          'duplicate_regularization_start_epoch': 2,
          'duplicate_regularization_ramp_epochs': 3,
        },
        'training': {
          'discovery_epochs': 1,
          'structuring_epochs': 1,
          'max_epochs': 5,
        },
      })

  def test_validates_candidate_opening_configuration(self):
    with self.assertRaisesRegex(ValueError, 'first_active_candidates'):
      SegmentModelConfig.from_dict({
        'model': {'candidates': 2},
        'candidate_opening': {'first_active_candidates': 3},
      })
    with self.assertRaisesRegex(ValueError, 'max_epochs'):
      SegmentModelConfig.from_dict({
        'model': {'candidates': 3},
        'training': {
          'discovery_epochs': 1,
          'structuring_epochs': 1,
          'max_epochs': 5,
        },
        'candidate_opening': {
          'enabled': True,
          'first_active_candidates': 1,
          'open_every_epochs': 2,
          'joint_finetune_epochs': 1,
        },
      })
    with self.assertRaisesRegex(ValueError, 'not implemented'):
      SegmentModelConfig.from_dict({
        'candidate_opening': {'reset_forest_on_candidate_open': True},
      })

if __name__ == '__main__':
  unittest.main()
