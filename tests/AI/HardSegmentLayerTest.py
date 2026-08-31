#!/usr/bin/python3.12

import importlib.util
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class HardSegmentLayerTest(unittest.TestCase):
  def test_executes_extracted_production_semantics(self):
    import torch
    from segment_model.HardSegmentLayer import HardSegmentLayer
    from segment_model.SegmentRuleExtractor import SegmentRule

    rules = [SegmentRule(0, ['u0', 'u1'], (0, 1), 60, 2)]
    layer = HardSegmentLayer(rules, 3, [10, 60], 'softmax_max')
    counts = torch.tensor([
      [[0.0, 2.0], [0.0, 1.0], [0.0, 10.0]],
      [[0.0, 1.0], [0.0, 1.0], [0.0, 10.0]],
    ])
    self.assertEqual([[1.0], [0.0]], layer(counts).tolist())


if __name__ == '__main__':
  unittest.main()
