#!/usr/bin/python3.12

import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.CandidateOpeningScheduler import CandidateOpeningScheduler
from segment_model.SegmentModelConfig import CandidateOpeningConfig


class CandidateOpeningSchedulerTest(unittest.TestCase):
  def test_opens_candidates_and_reduces_previous_learning_rates(self):
    scheduler = CandidateOpeningScheduler(
      CandidateOpeningConfig(
        enabled=True,
        first_active_candidates=1,
        open_every_epochs=20,
        previous_candidate_lr_mode='reduced',
        previous_candidate_lr_multiplier=0.1),
      4)
    self.assertEqual((True, False, False, False), scheduler.state(0).candidate_mask)
    self.assertEqual((1.0, 0.0, 0.0, 0.0), scheduler.state(0).learning_rate_multipliers)
    self.assertEqual((True, True, False, False), scheduler.state(20).candidate_mask)
    self.assertEqual((0.1, 1.0, 0.0, 0.0), scheduler.state(20).learning_rate_multipliers)
    self.assertEqual((True, True, True, True), scheduler.state(60).candidate_mask)
    self.assertEqual((0.1, 0.1, 0.1, 1.0), scheduler.state(60).learning_rate_multipliers)

  def test_joint_finetune_uses_one_reduced_learning_rate_and_anneals_floor(self):
    config = CandidateOpeningConfig(
      enabled=True,
      first_active_candidates=1,
      open_every_epochs=20,
      previous_candidate_lr_mode='reduced',
      previous_candidate_lr_multiplier=0.1,
      url_temperature_floor=0.5,
      joint_finetune_epochs=20,
      joint_finetune_lr_multiplier=0.2)
    scheduler = CandidateOpeningScheduler(config, 4)
    self.assertEqual(0.5, scheduler.url_temperature(0.1, 40, 'exponential'))
    self.assertEqual((0.2, 0.2, 0.2, 0.2), scheduler.state(60).learning_rate_multipliers)
    self.assertTrue(scheduler.state(60).joint_finetune_active)
    self.assertAlmostEqual(0.1, scheduler.url_temperature(0.1, 80, 'exponential'))
    self.assertTrue(scheduler.state(80).joint_finetune_complete)


if __name__ == '__main__':
  unittest.main()
