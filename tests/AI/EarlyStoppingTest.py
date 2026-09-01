#!/usr/bin/python3.12

import importlib.util
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class EarlyStoppingTest(unittest.TestCase):
  def test_ignores_insignificant_improvements_and_restores_best_checkpoint(self):
    import torch

    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelTrainer import SegmentModelTrainer
    from segment_model.SyntheticSegmentData import generate_synthetic_dataset

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [10],
        'n_values': [1],
        'url_buckets': 16,
        'batch_size': 8,
        'batch_workers': 1,
        'ready_batches': 1,
      },
      'model': {
        'candidates': 1,
        'forest': {'trees': 1, 'depth': 1, 'features_per_node': 1},
      },
      'training': {
        'discovery_epochs': 1,
        'structuring_epochs': 1,
        'max_epochs': 10000,
        'early_stopping_patience': 2,
        'early_stopping_min_delta': 1e-6,
        'seed': 5,
      },
      'synthetic': {
        'samples': 32,
        'users': 4,
        'urls': 4,
        'true_segments': 1,
        'existing_channels': 1,
        'events_per_user': 4,
        'horizon_seconds': 100,
        'validation_fraction': 0.125,
        'final_test_fraction': 0.125,
        'seed': 7,
      },
    })
    trainer = SegmentModelTrainer(config, generate_synthetic_dataset(config))
    validation_losses = iter((0.5, 0.4, 0.3999995, 0.3999994))
    validation_states = []

    def validation_loss(*args, **kwargs):
      del args
      del kwargs
      validation_states.append({
        name: value.detach().cpu().clone()
        for name, value in trainer.model.state_dict().items()
      })
      return next(validation_losses)

    trainer._validation_loss = validation_loss
    history = trainer.train()
    self.assertEqual(4, len(history))
    self.assertTrue(trainer.training_summary['stopped_early'])
    self.assertEqual(1, trainer.training_summary['best_epoch'])
    self.assertEqual(0.4, trainer.training_summary['best_validation_loss'])
    self.assertEqual(1e-6, trainer.training_summary['min_delta'])
    restored = trainer.model.state_dict()
    for name, expected in validation_states[1].items():
      self.assertTrue(torch.equal(expected, restored[name].detach().cpu()), name)


if __name__ == '__main__':
  unittest.main()
