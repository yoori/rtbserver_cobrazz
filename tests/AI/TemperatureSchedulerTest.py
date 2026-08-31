#!/usr/bin/python3.12

import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelConfig import TemperatureSchedule
from segment_model.TemperatureScheduler import TemperatureScheduler


class TemperatureSchedulerTest(unittest.TestCase):
  def test_preserves_warmup_before_annealing(self):
    scheduler = TemperatureScheduler(TemperatureSchedule(2.0, 0.5, 'linear', 0.2, 0.8))
    self.assertEqual(2.0, scheduler.value(0.1))
    self.assertAlmostEqual(1.25, scheduler.value(0.5))
    self.assertEqual(0.5, scheduler.value(0.9))


if __name__ == '__main__':
  unittest.main()
