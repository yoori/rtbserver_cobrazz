#!/usr/bin/python3.12

import importlib.util
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class DifferentiableRandomForestTest(unittest.TestCase):
  def test_routes_gradients_through_tree_levels(self):
    import torch
    from segment_model.DifferentiableRandomForest import DifferentiableRandomForest
    from segment_model.SegmentModelConfig import ForestConfig

    forest = DifferentiableRandomForest(
      4,
      ForestConfig(trees=3, depth=2, features_per_node=2, seed=1))
    features = torch.rand(5, 4, requires_grad=True)
    logits = forest(features, 1.0, 1.0)
    self.assertEqual((5,), tuple(logits.shape))
    logits.sum().backward()
    self.assertIsNotNone(features.grad)
    self.assertTrue(torch.isfinite(features.grad).all())


if __name__ == '__main__':
  unittest.main()
