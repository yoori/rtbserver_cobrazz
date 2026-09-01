import json
import sys
import time


class TrainingProgressReporter:
  def __init__(
      self,
      total_batches,
      interval_seconds=10.0,
      output=None,
      clock=None,
      callback=None):
    if interval_seconds <= 0:
      raise ValueError('progress reporting interval must be positive')
    self.total_batches = total_batches
    self.interval_seconds = interval_seconds
    self.output = output if output is not None else sys.stdout
    self.clock = clock if clock is not None else time.monotonic
    self.callback = callback
    self.started_at = None
    self.last_report_at = None
    self.batch_wait_started_at = None
    self.training_compute_started_at = None
    self.batch_wait_seconds = 0.0
    self.training_compute_seconds = 0.0
    self.completed_batches = 0
    self.stage = None
    self.epoch = None

  def start(self):
    if self.started_at is not None:
      return
    self.started_at = self.clock()
    self.last_report_at = self.started_at

  def set_position(self, stage, epoch):
    self.stage = stage
    self.epoch = epoch

  def begin_batch_wait(self):
    self.batch_wait_started_at = self.clock()

  def end_batch_wait(self):
    if self.batch_wait_started_at is not None:
      self.batch_wait_seconds += self.clock() - self.batch_wait_started_at
      self.batch_wait_started_at = None

  def begin_training_compute(self):
    self.training_compute_started_at = self.clock()

  def end_training_compute(self, completed=True):
    if self.training_compute_started_at is not None:
      self.training_compute_seconds += self.clock() - self.training_compute_started_at
      self.training_compute_started_at = None
    if completed:
      self.completed_batches += 1

  def maybe_report(self):
    now = self.clock()
    if now - self.last_report_at >= self.interval_seconds:
      self._report(final=False, now=now)
      self.last_report_at = now

  def close(self):
    if self.started_at is not None:
      self._report(final=True)

  def snapshot(self, final=False, now=None):
    now = self.clock() if now is None else now
    elapsed = now - self.started_at
    batch_wait = self.batch_wait_seconds
    if self.batch_wait_started_at is not None:
      batch_wait += now - self.batch_wait_started_at
    training_compute = self.training_compute_seconds
    if self.training_compute_started_at is not None:
      training_compute += now - self.training_compute_started_at
    return {
      'event': 'training_progress',
      'stage': self.stage,
      'epoch': self.epoch,
      'completed_batches': self.completed_batches,
      'total_batches': self.total_batches,
      'training_elapsed_seconds': round(elapsed, 3),
      'batch_wait_seconds': round(batch_wait, 3),
      'training_compute_seconds': round(training_compute, 3),
      'other_seconds': round(max(0.0, elapsed - batch_wait - training_compute), 3),
      'final': final,
    }

  def _report(self, final, now=None):
    record = self.snapshot(final, now)
    if self.callback is not None:
      self.callback(record)
    print(json.dumps(record, sort_keys=True), file=self.output, flush=True)
