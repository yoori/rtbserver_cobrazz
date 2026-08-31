#!/usr/bin/python3.12

import importlib.util
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class SegmentLayerTest(unittest.TestCase):
  def test_hard_activation_uses_max_url_count(self):
    import torch
    from segment_model.DifferentiableSegmentLayer import DifferentiableSegmentLayer

    layer = DifferentiableSegmentLayer(1, 3, [10, 60], [1, 2], 'softmax_max')
    with torch.no_grad():
      layer.membership.url_logits[0] = torch.tensor([4.0, 4.0, -4.0])
      layer.window_logits[0] = torch.tensor([-4.0, 4.0])
      layer.threshold_logits[0] = torch.tensor([-4.0, 4.0])
    counts = torch.tensor([
      [[0.0, 2.0], [0.0, 1.0], [0.0, 10.0]],
      [[0.0, 1.0], [0.0, 1.0], [0.0, 10.0]],
    ])
    self.assertEqual([[1.0], [0.0]], layer.hard_activations(counts).tolist())

  def test_soft_activation_approaches_hard_rule(self):
    import torch
    from segment_model.DifferentiableSegmentLayer import DifferentiableSegmentLayer

    layer = DifferentiableSegmentLayer(1, 2, [60], [1], 'softmax_max')
    with torch.no_grad():
      layer.membership.url_logits[0] = torch.tensor([10.0, -10.0])
    counts = torch.tensor([[[2.0], [0.0]], [[0.0], [100.0]]])
    output = layer(counts, {
      'url': 0.1,
      'window': 0.1,
      'threshold': 0.1,
      'aggregation': 0.05,
      'activation': 0.05,
    })
    hard = layer.hard_activations(counts)
    self.assertTrue(torch.allclose(output.activations, hard, atol=1e-3))
    output.activations.sum().backward()
    self.assertTrue(torch.isfinite(layer.membership.url_logits.grad).all())


if __name__ == '__main__':
  unittest.main()
