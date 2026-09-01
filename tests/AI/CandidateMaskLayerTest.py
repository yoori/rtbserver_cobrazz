#!/usr/bin/python3.12

import importlib.util
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class CandidateMaskLayerTest(unittest.TestCase):
  def test_masks_candidates_without_changing_shape(self):
    import torch
    from segment_model.CandidateMaskLayer import CandidateMaskLayer

    layer = CandidateMaskLayer(4, 1)
    activations = torch.tensor([[0.83, 0.71, 0.24, 0.91]])
    self.assertTrue(torch.allclose(
      torch.tensor([[0.83, 0.0, 0.0, 0.0]]),
      layer(activations)))
    self.assertEqual((1, 4), tuple(layer(activations).shape))
    layer.set_mask([1, 1, 0, 0])
    self.assertTrue(torch.allclose(
      torch.tensor([[0.83, 0.71, 0.0, 0.0]]),
      layer(activations)))


if __name__ == '__main__':
  unittest.main()
