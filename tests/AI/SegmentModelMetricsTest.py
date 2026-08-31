#!/usr/bin/python3.12

import pathlib
import sys
import unittest

import numpy


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelMetrics import ctr_metrics
from segment_model.SegmentModelMetrics import match_segment_rules
from segment_model.SegmentRuleExtractor import SegmentRule
from segment_model.SyntheticSegmentData import SyntheticRule


class SegmentModelMetricsTest(unittest.TestCase):
  def test_ctr_metrics_are_exact_for_simple_ranking(self):
    metrics = ctr_metrics(numpy.asarray([0, 0, 1, 1]), numpy.asarray([0.1, 0.2, 0.8, 0.9]))
    self.assertEqual(1.0, metrics['roc_auc'])
    self.assertEqual(1.0, metrics['pr_auc'])
    self.assertLess(metrics['logloss'], 0.2)

  def test_recovery_matching_is_permutation_invariant(self):
    learned = [SegmentRule(0, ['u3'], (3,), 60, 2), SegmentRule(1, ['u1', 'u2'], (1, 2), 300, 1)]
    truth = [SyntheticRule(10, (1, 2), 300, 1, 1.0), SyntheticRule(11, (3,), 60, 2, -1.0)]
    metrics = match_segment_rules(learned, truth)
    self.assertEqual(1.0, metrics['url_f1'])
    self.assertEqual(1.0, metrics['window_accuracy'])
    self.assertEqual(1.0, metrics['n_accuracy'])


if __name__ == '__main__':
  unittest.main()
