import dataclasses
import multiprocessing
import queue
import traceback

import numpy


@dataclasses.dataclass(frozen=True)
class BatchRequest:
  epoch: int
  batch_index: int
  include_existing_channels: bool


@dataclasses.dataclass
class SegmentBatch:
  history_counts: numpy.ndarray
  existing_channels: numpy.ndarray
  context_features: numpy.ndarray
  labels: numpy.ndarray
  sample_indices: numpy.ndarray
  timestamps: numpy.ndarray = None
  group_ids: numpy.ndarray = None


@dataclasses.dataclass
class WorkerFailure:
  request: object
  message: str
  traceback: str


def _batch_worker(batch_builder, task_queue, ready_queue):
  while True:
    request = task_queue.get()
    if request is None:
      return
    try:
      ready_queue.put((request, batch_builder(request)))
    except BaseException as error:
      ready_queue.put((request, WorkerFailure(request, str(error), traceback.format_exc())))


class ForkedBatchPool:
  def __init__(self, batch_builder, requests, workers=2, ready_batches=4, start_method='fork'):
    if workers <= 0:
      raise ValueError('workers must be positive')
    if ready_batches < workers:
      raise ValueError('ready_batches must be at least workers')
    self.batch_builder = batch_builder
    self.requests = iter(requests)
    self.workers = workers
    self.ready_batches = ready_batches
    self.context = multiprocessing.get_context(start_method)
    self.task_queue = None
    self.ready_queue = None
    self.processes = []
    self.scheduled = 0
    self.completed = 0
    self.exhausted = False
    self.closed = False

  def __enter__(self):
    self.start()
    return self

  def __exit__(self, exception_type, exception, exception_traceback):
    del exception_type
    del exception
    del exception_traceback
    self.close()

  def start(self):
    if self.closed:
      return
    if self.processes:
      return
    self.task_queue = self.context.Queue(maxsize=self.ready_batches)
    self.ready_queue = self.context.Queue(maxsize=self.ready_batches)
    for worker_index in range(self.workers):
      process = self.context.Process(
        name='SegmentBatchWorker-' + str(worker_index),
        target=_batch_worker,
        args=(self.batch_builder, self.task_queue, self.ready_queue))
      process.daemon = True
      process.start()
      self.processes.append(process)
    self._fill_tasks()

  def __iter__(self):
    self.start()
    return self

  def __next__(self):
    if self.closed:
      raise StopIteration
    if not self.processes:
      self.start()
    if self.completed == self.scheduled and self.exhausted:
      self.close()
      raise StopIteration
    while True:
      try:
        request, result = self.ready_queue.get(timeout=1.0)
        break
      except queue.Empty:
        failed = [process for process in self.processes if not process.is_alive()]
        if failed:
          self.close()
          raise RuntimeError('Batch worker stopped before returning a batch')
    self.completed += 1
    self._fill_tasks()
    if isinstance(result, WorkerFailure):
      self.close()
      raise RuntimeError(
        'Batch preparation failed for ' + repr(request) + ': ' + result.message + '\n' +
        result.traceback)
    return result

  def _fill_tasks(self):
    while not self.exhausted and self.scheduled - self.completed < self.ready_batches:
      try:
        request = next(self.requests)
      except StopIteration:
        self.exhausted = True
        return
      self.task_queue.put(request)
      self.scheduled += 1

  def close(self):
    if self.closed:
      return
    self.closed = True
    for process in self.processes:
      if process.is_alive():
        process.terminate()
    for process in self.processes:
      process.join(timeout=5.0)
    if self.task_queue is not None:
      self.task_queue.close()
    if self.ready_queue is not None:
      self.ready_queue.close()
    self.processes = []
