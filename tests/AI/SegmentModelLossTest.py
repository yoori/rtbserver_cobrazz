#!/usr/bin/python3.12

import importlib.util
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class SegmentModelLossTest(unittest.TestCase):
  def test_url_regularization_only_updates_observed_buckets(self):
    import torch
    from segment_model.SegmentCTRModel import SegmentCTRModel
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentModelLoss import segment_model_loss

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [60],
        'n_values': [1],
        'url_buckets': 5,
        'batch_workers': 1,
        'ready_batches': 1,
      },
      'model': {
        'candidates': 1,
        'forest': {
          'trees': 1,
          'depth': 1,
          'features_per_node': 1,
        },
      },
      'loss': {
        'sparsity': 1.0,
        'binarization': 1.0,
        'diversity': 0.0,
        'duplicate_existing': 0.0,
        'diversity_pairs': 0,
      },
      'synthetic': {'true_segments': 1},
    })
    model = SegmentCTRModel(config, 5, 0)
    counts = torch.tensor([[[1.0], [0.0]], [[0.0], [1.0]]])
    active_url_ids = torch.tensor([1, 3])
    output = model(
      counts,
      torch.empty((2, 0)),
      torch.empty((2, 0)),
      {
        'url': 1.0,
        'window': 1.0,
        'threshold': 1.0,
        'activation': 1.0,
        'aggregation': 1.0,
        'forest_feature': 1.0,
        'forest_split': 1.0,
      },
      active_url_ids)
    segment_model_loss(output, torch.tensor([0.0, 1.0]), config).total.backward()
    gradient = model.segment_layer.membership.url_logits.grad[0]
    self.assertGreater(float(torch.sum(torch.abs(gradient[active_url_ids]))), 0.0)
    self.assertEqual(0.0, float(torch.sum(torch.abs(gradient[[0, 2, 4]]))))


if __name__ == '__main__':
  unittest.main()
