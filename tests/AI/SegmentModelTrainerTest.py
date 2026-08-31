#!/usr/bin/python3.12

import importlib.util
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
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelMetrics import finite_metrics
    from segment_model.SegmentModelTrainer import SegmentModelTrainer
    from segment_model.SyntheticSegmentData import generate_synthetic_dataset

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [10, 100],
        'n_values': [1, 2],
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
      'training': {
        'discovery_epochs': 1,
        'structuring_epochs': 1,
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
        'validation_fraction': 0.25,
        'seed': 7,
      },
    })
    dataset = generate_synthetic_dataset(config)
    trainer = SegmentModelTrainer(config, dataset)
    frequencies = numpy.sum(dataset.history_counts[dataset.train_indices, :, -1], axis=0)
    expected_urls = set(numpy.argsort(-frequencies)[:config.model.candidates].tolist())
    initial_logits = trainer.model.segment_layer.membership.url_logits.detach().cpu().numpy()
    selected_urls = set(numpy.flatnonzero(numpy.any(initial_logits > 0, axis=0)).tolist())
    self.assertEqual(expected_urls, selected_urls)
    with tempfile.TemporaryDirectory() as output_dir:
      history = trainer.train(output_dir)
      metrics, rules = trainer.evaluate()
      trainer.save(output_dir, history, metrics, rules)
      artifacts = {path.name for path in pathlib.Path(output_dir).iterdir()}
    self.assertEqual(['discovery', 'structuring'], [record['stage'] for record in history])
    self.assertTrue(finite_metrics(metrics))
    self.assertIn('checkpoint-discovery.pt', artifacts)
    self.assertIn('checkpoint-structuring.pt', artifacts)
    self.assertIn('segments.json', artifacts)
    self.assertIn('synthetic-ground-truth.json', artifacts)


if __name__ == '__main__':
  unittest.main()
