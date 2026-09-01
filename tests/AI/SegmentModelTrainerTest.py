#!/usr/bin/python3.12

import importlib.util
import math
import pathlib
import sys
import tempfile
import unittest

import numpy


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class SegmentModelTrainerTest(unittest.TestCase):
  def test_trains_both_stages_and_writes_artifacts(self):
    import torch
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelMetrics import finite_metrics
    from segment_model.SegmentModelTrainer import SegmentModelTrainer
    from segment_model.SyntheticSegmentData import generate_synthetic_dataset

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [10, 100],
        'n_values': [1, 2],
        'url_buckets': 32,
        'batch_size': 8,
        'batch_workers': 1,
        'ready_batches': 2,
      },
      'model': {
        'candidates': 2,
        'forest': {
          'trees': 2,
          'depth': 2,
          'features_per_node': 2,
        },
      },
      'loss': {
        'url_duplicate': 0.0,
        'activation_duplicate': 0.0,
      },
      'training': {
        'discovery_epochs': 1,
        'structuring_epochs': 1,
        'max_epochs': 4,
        'early_stopping_patience': 2,
        'seed': 5,
      },
      'synthetic': {
        'samples': 32,
        'users': 8,
        'urls': 8,
        'true_segments': 2,
        'existing_channels': 2,
        'events_per_user': 10,
        'horizon_seconds': 1000,
        'validation_fraction': 0.125,
        'final_test_fraction': 0.125,
        'seed': 7,
      },
    })
    dataset = generate_synthetic_dataset(config)
    trainer = SegmentModelTrainer(config, dataset)
    train_ctr = float(numpy.mean(dataset.labels[dataset.train_indices]))
    epsilon = torch.finfo(trainer.model.forest.global_bias.dtype).eps
    clipped_ctr = min(1.0 - epsilon, max(epsilon, train_ctr))
    expected_bias = math.log(clipped_ctr / (1.0 - clipped_ctr))
    self.assertAlmostEqual(expected_bias, float(trainer.model.forest.global_bias), places=6)
    self.assertGreater(float(trainer.model.forest.leaf_logits.std()), 0.0)
    self.assertLess(float(trainer.model.forest.leaf_logits.abs().max()), 0.01)
    self.assertLess(float(trainer.model.forest.feature_logits.abs().max()), 0.01)
    self.assertEqual(4, trainer.model.forest.input_features)
    frequencies = numpy.sum(dataset.history_counts[dataset.train_indices, :, -1], axis=0)
    ranking = dataset.history_url_ids[numpy.argsort(-frequencies, kind='stable')]
    expected_urls = set(ranking[:config.model.candidates].tolist())
    initial_logits = trainer.model.segment_layer.membership.url_logits.detach().cpu().numpy()
    selected_urls = set(numpy.flatnonzero(numpy.any(initial_logits > 0, axis=0)).tolist())
    self.assertEqual(expected_urls, selected_urls)
    with tempfile.TemporaryDirectory() as output_dir:
      history = trainer.train(output_dir)
      metrics, rules = trainer.evaluate()
      trainer.save(output_dir, history, metrics, rules)
      artifacts = {path.name for path in pathlib.Path(output_dir).iterdir()}
    self.assertEqual('discovery', history[0]['stage'])
    self.assertTrue(all(record['stage'] == 'structuring' for record in history[1:]))
    self.assertEqual(0.0, history[0]['regularization_scale'])
    self.assertGreater(history[1]['regularization_scale'], 0.0)
    self.assertTrue(all(record['duplicate_regularization_scale'] == 0.0 for record in history))
    self.assertTrue(all(record['checkpoint_eligible'] for record in history))
    self.assertEqual(config.url_temperature.start, history[0]['temperatures']['url'])
    self.assertTrue(all('validation_loss' in record for record in history))
    self.assertTrue(all('duplicate_diagnostics' in record for record in history))
    self.assertTrue(finite_metrics(metrics))
    self.assertIn('candidate_duplicates', metrics['diagnostics'])
    self.assertIn('checkpoint-discovery.pt', artifacts)
    self.assertIn('checkpoint-best.pt', artifacts)
    self.assertIn('segments.json', artifacts)
    self.assertIn('synthetic-ground-truth.json', artifacts)
    self.assertIn('training-summary.json', artifacts)
    self.assertIn('url-bucket-dictionary.json', artifacts)
    self.assertEqual(len(dataset.train_indices), trainer.training_summary['training_rows'])
    self.assertAlmostEqual(train_ctr, trainer.training_summary['training_ctr'], places=6)

  def test_fixed_candidate_opening_freezes_closed_parameters_and_records_schedule(self):
    import torch
    from segment_model.SegmentModelData import BatchRequest
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelTrainer import SegmentModelTrainer
    from segment_model.SyntheticSegmentData import SyntheticBatchBuilder
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
        'candidates': 3,
        'forest': {'trees': 1, 'depth': 1, 'features_per_node': 3},
      },
      'loss': {
        'sparsity': 1.0,
        'url_duplicate': 0.0,
        'activation_duplicate': 0.0,
      },
      'training': {
        'discovery_epochs': 1,
        'structuring_epochs': 1,
        'max_epochs': 5,
        'early_stopping_patience': 2,
        'weight_decay': 0.2,
      },
      'candidate_opening': {
        'enabled': True,
        'first_active_candidates': 1,
        'open_every_epochs': 1,
        'previous_candidate_lr_mode': 'reduced',
        'previous_candidate_lr_multiplier': 0.1,
        'url_temperature_floor': 0.6,
      },
      'synthetic': {
        'samples': 24,
        'users': 4,
        'urls': 4,
        'true_segments': 1,
        'existing_channels': 0,
        'events_per_user': 4,
        'horizon_seconds': 100,
        'validation_fraction': 0.125,
        'final_test_fraction': 0.125,
      },
    })
    dataset = generate_synthetic_dataset(config)
    trainer = SegmentModelTrainer(config, dataset)
    initial_closed = trainer.model.segment_layer.membership.url_logits[1:].detach().clone()
    opening_state = trainer._apply_candidate_opening(0)
    builder = SyntheticBatchBuilder(dataset, dataset.train_indices, config.data.batch_size)
    batch = builder(BatchRequest(0, 0, False))
    trainer._train_batch(
      batch,
      trainer.temperatures(0.0, opening_state),
      opening_state,
      regularization_scale=1.0,
      duplicate_regularization_scale=0.0)
    self.assertTrue(torch.equal(
      initial_closed,
      trainer.model.segment_layer.membership.url_logits[1:].detach()))
    history = trainer.train()
    self.assertEqual([1, 0, 0], history[0]['candidate_opening']['candidate_mask'])
    self.assertEqual([1, 1, 0], history[1]['candidate_opening']['candidate_mask'])
    self.assertEqual([1, 1, 1], history[2]['candidate_opening']['candidate_mask'])
    self.assertFalse(history[0]['checkpoint_eligible'])
    self.assertFalse(history[1]['checkpoint_eligible'])
    self.assertTrue(history[2]['checkpoint_eligible'])
    self.assertEqual([1, 2], [
      event['candidate']
      for event in trainer.candidate_opening_events
    ])
    self.assertGreaterEqual(history[0]['temperatures']['url'], 0.6)

  def test_initializes_requested_membership_modes(self):
    import torch
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelTrainer import SegmentModelTrainer
    from segment_model.SyntheticSegmentData import generate_synthetic_dataset

    expected_selected = {
      'random_single_seed': 1,
      'symmetric_with_noise': 0,
      'random_multi_seed': 2,
    }
    for mode, selected_per_candidate in expected_selected.items():
      with self.subTest(mode=mode):
        config = SegmentModelConfig.from_dict({
          'data': {
            'windows_seconds': [10],
            'n_values': [1],
            'url_buckets': 64,
            'batch_workers': 1,
            'ready_batches': 1,
          },
          'model': {
            'candidates': 2,
            'membership': {
              'initialization': mode,
              'initial_urls_per_candidate': 2 if mode == 'random_multi_seed' else 1,
              'unselected_logit': -2.0,
              'selected_logit': 1.0,
              'logit_std': 0.0,
            },
            'forest': {'trees': 1, 'depth': 1, 'features_per_node': 1},
          },
          'synthetic': {
            'samples': 16,
            'users': 4,
            'urls': 4,
            'true_segments': 1,
            'existing_channels': 0,
            'events_per_user': 4,
            'horizon_seconds': 100,
            'validation_fraction': 0.125,
            'final_test_fraction': 0.125,
          },
        })
        dataset = generate_synthetic_dataset(config)
        trainer = SegmentModelTrainer(config, dataset)
        observed = trainer.model.segment_layer.membership.url_logits[
          :, dataset.history_url_ids]
        selected = torch.sum(observed > 0, dim=1).tolist()
        self.assertEqual([selected_per_candidate] * config.model.candidates, selected)

  def test_duplicate_regularization_schedule_and_components(self):
    from segment_model.SegmentModelTrainer import _duplicate_components
    from segment_model.SegmentModelTrainer import _duplicate_regularization_scale

    self.assertEqual(0.0, _duplicate_regularization_scale(4, 9, 10, 5, 15))
    self.assertEqual(0.0, _duplicate_regularization_scale(5, 0, 10, 5, 15))
    self.assertAlmostEqual(0.5, _duplicate_regularization_scale(12, 5, 10, 5, 15))
    self.assertEqual(1.0, _duplicate_regularization_scale(20, 0, 10, 5, 15))
    self.assertEqual([[0, 1, 2], [4, 5]], _duplicate_components([(0, 1), (1, 2), (4, 5)]))

  def test_optional_reseed_reinitializes_weaker_candidate_once(self):
    import torch
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelTrainer import SegmentModelTrainer
    from segment_model.SyntheticSegmentData import generate_synthetic_dataset

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [10, 20],
        'n_values': [1, 2],
        'url_buckets': 64,
        'batch_workers': 1,
        'ready_batches': 1,
      },
      'model': {
        'candidates': 2,
        'membership': {
          'initialization': 'random_single_seed',
          'initial_urls_per_candidate': 1,
          'unselected_logit': -2.0,
          'selected_logit': 1.0,
          'logit_std': 0.0,
        },
        'forest': {'trees': 1, 'depth': 1, 'features_per_node': 1},
      },
      'loss': {'enable_candidate_reseed': True},
      'synthetic': {
        'samples': 16,
        'users': 4,
        'urls': 4,
        'true_segments': 1,
        'existing_channels': 0,
        'events_per_user': 4,
        'horizon_seconds': 100,
        'validation_fraction': 0.125,
        'final_test_fraction': 0.125,
      },
    })
    trainer = SegmentModelTrainer(config, generate_synthetic_dataset(config))
    before = trainer.model.segment_layer.membership.url_logits.detach().clone()
    with torch.no_grad():
      trainer.model.segment_layer.window_logits[1].fill_(9.0)
      trainer.model.segment_layer.threshold_logits[1].fill_(9.0)
    reseeded = trainer._reseed_duplicate_candidates([(0, 1)], [0.9, 0.1], 20)
    after = trainer.model.segment_layer.membership.url_logits.detach()
    self.assertEqual([1], reseeded)
    self.assertTrue(torch.equal(before[0], after[0]))
    self.assertFalse(torch.equal(before[1], after[1]))
    self.assertEqual(
      [0.0, config.model.choice_initial_logit],
      sorted(trainer.model.segment_layer.window_logits[1].detach().tolist()))
    self.assertEqual(
      [0.0, config.model.choice_initial_logit],
      sorted(trainer.model.segment_layer.threshold_logits[1].detach().tolist()))
    self.assertEqual([], trainer._reseed_duplicate_candidates([(0, 1)], [0.9, 0.1], 21))


if __name__ == '__main__':
  unittest.main()
