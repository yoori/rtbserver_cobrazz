#!/usr/bin/python3.12

import pathlib
import sys
import unittest

import numpy


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SyntheticSegmentData import _hash_history_counts
from segment_model.UrlHash import url_bucket


class UrlHashTest(unittest.TestCase):
  def test_hash_is_stable_and_inside_the_configured_range(self):
    first = url_bucket('https://example.com/path', 1000000)
    second = url_bucket('https://example.com/path', 1000000)
    self.assertEqual(first, second)
    self.assertGreaterEqual(first, 0)
    self.assertLess(first, 1000000)

  def test_reverse_dictionary_preserves_all_urls_after_a_collision(self):
    counts = numpy.asarray([
      [[1.0], [2.0], [4.0]],
    ], dtype=numpy.float32)
    hashed, active_buckets, url_bucket_ids, dictionary = _hash_history_counts(
      counts,
      ['a.com', 'b.com', 'c.com'],
      1)
    numpy.testing.assert_array_equal(active_buckets, [0])
    numpy.testing.assert_array_equal(url_bucket_ids, [0, 0, 0])
    numpy.testing.assert_array_equal(hashed, [[[7.0]]])
    self.assertEqual(['a.com', 'b.com', 'c.com'], dictionary[0])


if __name__ == '__main__':
  unittest.main()
