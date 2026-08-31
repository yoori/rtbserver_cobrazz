#!/usr/bin/python3.12

import importlib.util
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))
TORCH_AVAILABLE = importlib.util.find_spec('torch') is not None


@unittest.skipUnless(TORCH_AVAILABLE, 'PyTorch is not installed in the source environment')
class SegmentRuleExtractorTest(unittest.TestCase):
  def test_extracts_rule_directly_from_model_parameters(self):
    import torch
    from segment_model.SegmentCTRModel import SegmentCTRModel
    from segment_model.SegmentModelConfig import SegmentModelConfig
    from segment_model.SegmentRuleExtractor import extract_segment_rules

    config = SegmentModelConfig.from_dict({
      'data': {
        'windows_seconds': [60, 300],
        'n_values': [1, 3],
        'batch_workers': 1,
        'ready_batches': 1,
      },
      'model': {
        'candidates': 1,
        'forest': {'trees': 2, 'depth': 1, 'features_per_node': 1},
      },
      'synthetic': {'true_segments': 1},
    })
    model = SegmentCTRModel(config, 3, 0)
    with torch.no_grad():
      model.segment_layer.membership.url_logits[0] = torch.tensor([2.0, -1.0, 0.1])
      model.segment_layer.window_logits[0] = torch.tensor([-1.0, 1.0])
      model.segment_layer.threshold_logits[0] = torch.tensor([2.0, -2.0])
    rule = extract_segment_rules(model, ['a', 'b', 'c'])[0]
    self.assertEqual(['a', 'c'], rule.urls)
    self.assertEqual(300, rule.window_seconds)
    self.assertEqual(1, rule.min_visits)


if __name__ == '__main__':
  unittest.main()
