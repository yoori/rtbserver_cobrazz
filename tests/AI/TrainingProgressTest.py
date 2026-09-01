#!/usr/bin/python3.12

import io
import json
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.TrainingProgress import TrainingProgressReporter


class TrainingProgressTest(unittest.TestCase):
  def test_reports_wait_and_training_compute_separately(self):
    clock = FakeClock()
    output = io.StringIO()
    reporter = TrainingProgressReporter(3, interval_seconds=10.0, output=output, clock=clock)
    reporter.start()
    reporter.set_position('discovery', 2)
    reporter.begin_batch_wait()
    clock.advance(11.0)
    reporter.maybe_report()
    reporter.end_batch_wait()
    reporter.begin_training_compute()
    clock.advance(3.0)
    reporter.end_training_compute()
    reporter.close()
    records = [json.loads(line) for line in output.getvalue().splitlines()]
    self.assertGreaterEqual(len(records), 2)
    final = records[-1]
    self.assertEqual('training_progress', final['event'])
    self.assertEqual('discovery', final['stage'])
    self.assertEqual(2, final['epoch'])
    self.assertEqual(1, final['completed_batches'])
    self.assertEqual(3, final['total_batches'])
    self.assertEqual(11.0, final['batch_wait_seconds'])
    self.assertEqual(3.0, final['training_compute_seconds'])
    self.assertTrue(final['final'])

  def test_sends_same_records_to_callback(self):
    clock = FakeClock()
    output = io.StringIO()
    callback_records = []
    reporter = TrainingProgressReporter(
      1,
      interval_seconds=1.0,
      output=output,
      clock=clock,
      callback=callback_records.append)
    reporter.start()
    reporter.set_position('structuring', 4)
    reporter.begin_training_compute()
    clock.advance(2.0)
    reporter.end_training_compute()
    reporter.close()

    output_records = [
      json.loads(line)
      for line in output.getvalue().splitlines()
    ]
    self.assertEqual(output_records, callback_records)


class FakeClock:
  def __init__(self):
    self.value = 0.0

  def __call__(self):
    return self.value

  def advance(self, seconds):
    self.value += seconds


if __name__ == '__main__':
  unittest.main()
