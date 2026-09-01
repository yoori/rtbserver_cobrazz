#!/usr/bin/python3.12

import pathlib
import sys
import unittest

import numpy


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelData import BatchRequest
from segment_model.SegmentModelData import ForkedBatchPool
from segment_model.SegmentModelData import SegmentBatch


class TestBatchBuilder:
  def __call__(self, request):
    value = request.epoch * 100 + request.batch_index
    existing = 1.0 if request.include_existing_channels else 0.0
    return SegmentBatch(
      history_counts=numpy.full((1, 1, 1), value, dtype=numpy.float32),
      history_url_ids=numpy.asarray([0], dtype=numpy.int64),
      existing_channels=numpy.full((1, 1), existing, dtype=numpy.float32),
      context_features=numpy.empty((1, 0), dtype=numpy.float32),
      labels=numpy.zeros(1, dtype=numpy.float32),
      sample_indices=numpy.asarray([value], dtype=numpy.int64))


class ForkedBatchPoolTest(unittest.TestCase):
  def test_prefetches_requests_with_multiple_forked_workers(self):
    requests = [
      BatchRequest(0, 0, False),
      BatchRequest(0, 1, False),
      BatchRequest(1, 0, True),
      BatchRequest(1, 1, True),
    ]
    with ForkedBatchPool(TestBatchBuilder(), requests, workers=2, ready_batches=3) as pool:
      self.assertEqual(3, pool.scheduled)
      batches = list(pool)
    values = sorted(int(batch.sample_indices[0]) for batch in batches)
    self.assertEqual([0, 1, 100, 101], values)
    existing_by_value = {
      int(batch.sample_indices[0]): float(batch.existing_channels[0, 0])
      for batch in batches
    }
    self.assertEqual(0.0, existing_by_value[0])
    self.assertEqual(1.0, existing_by_value[100])


if __name__ == '__main__':
  unittest.main()
