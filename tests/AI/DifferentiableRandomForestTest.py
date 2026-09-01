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
    with torch.no_grad():
      forest.leaf_logits.copy_(torch.arange(12, dtype=torch.float32).reshape(3, 4))
    features = torch.rand(5, 4, requires_grad=True)
    logits = forest(features, 1.0, 1.0)
    self.assertEqual((5,), tuple(logits.shape))
    logits.sum().backward()
    self.assertIsNotNone(features.grad)
    self.assertTrue(torch.isfinite(features.grad).all())
    self.assertGreater(float(torch.sum(torch.abs(features.grad))), 0.0)

  def test_adds_tree_contributions_to_global_bias(self):
    import torch
    from segment_model.DifferentiableRandomForest import DifferentiableRandomForest
    from segment_model.SegmentModelConfig import ForestConfig

    forest = DifferentiableRandomForest(
      1,
      ForestConfig(trees=2, depth=1, features_per_node=1, seed=1),
      binary_features=1)
    with torch.no_grad():
      forest.global_bias.fill_(0.75)
      forest.leaf_logits.copy_(torch.tensor([[1.0, 2.0], [3.0, 4.0]]))
    logits = forest(torch.tensor([[0.0], [1.0]]), 0.1, 0.1, hard=True)
    self.assertEqual([4.75, 6.75], logits.tolist())

  def test_binary_features_always_split_at_one_half(self):
    import torch
    from segment_model.DifferentiableRandomForest import DifferentiableRandomForest
    from segment_model.SegmentModelConfig import ForestConfig

    forest = DifferentiableRandomForest(
      1,
      ForestConfig(trees=1, depth=1, features_per_node=1, seed=1),
      binary_features=1)
    with torch.no_grad():
      forest.split_thresholds.fill_(100.0)
      forest.leaf_logits[0] = torch.tensor([-2.0, 2.0])
    logits = forest(torch.tensor([[0.0], [1.0]]), 0.1, 0.1, hard=True)
    self.assertEqual([-2.0, 2.0], logits.tolist())

  def test_learns_ctr_from_the_expected_fixed_segment(self):
    import torch
    from segment_model.DifferentiableRandomForest import DifferentiableRandomForest
    from segment_model.SegmentModelConfig import ForestConfig

    torch.manual_seed(7)
    forest = DifferentiableRandomForest(
      1,
      ForestConfig(
        trees=4,
        depth=1,
        features_per_node=1,
        seed=1),
      binary_features=1)
    optimizer = torch.optim.Adam(forest.parameters(), lr=0.1)
    features = torch.tensor([[0.0], [1.0]])
    expected_ctr = torch.tensor([0.0, 0.01])
    for _ in range(1000):
      optimizer.zero_grad(set_to_none=True)
      logits = forest(features, 0.1, 0.1)
      loss = torch.nn.functional.binary_cross_entropy_with_logits(logits, expected_ctr)
      loss.backward()
      optimizer.step()
    probabilities = torch.sigmoid(forest(features, 0.1, 0.1, hard=True)).detach()
    self.assertLess(float(probabilities[0]), 0.001)
    self.assertAlmostEqual(0.01, float(probabilities[1]), delta=0.001)


if __name__ == '__main__':
  unittest.main()
