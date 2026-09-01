#!/usr/bin/python3.12

import json
import os
import pathlib
import sys
import tempfile
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelRepository import (
  SegmentModelNotFound,
  SegmentModelRepository,
)


class SegmentModelRepositoryTest(unittest.TestCase):
  def test_lists_published_and_training_models(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      self.create_model(root, '20260901.120000')
      self.create_training_model(root, '~20260901.130000', os.getpid())
      self.create_training_model(root, '~20260901.110000', 99999999)

      repository = SegmentModelRepository(root)

      self.assertEqual(
        ['~20260901.130000', '20260901.120000', '~20260901.110000'],
        repository.all_model_ids())
      self.assertEqual('20260901.120000', repository.latest_model_id())
      self.assertEqual(
        'in_progress',
        repository.model_summary('~20260901.130000')['status'])
      interrupted = repository.model_properties('~20260901.110000')
      self.assertEqual('interrupted', interrupted['summary']['status'])
      self.assertEqual(
        'process_not_running',
        interrupted['traits']['interruption_reason'])

  def test_reads_summary_and_filters_enriched_segments(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      self.create_model(root, '20260901.120000')
      repository = SegmentModelRepository(root)

      summary = repository.model_summary('20260901.120000')
      self.assertEqual(2, summary['segments_count'])
      self.assertEqual(0.201, summary['soft_logloss'])
      self.assertEqual(1, summary['empty_segments'])

      result = repository.segments(
        '20260901.120000',
        0,
        100,
        'example.com')
      self.assertEqual(1, result['total'])
      self.assertEqual(0, result['items'][0]['segment_id'])
      self.assertEqual(0.25, result['items'][0]['average_activation'])
      self.assertEqual(
        0,
        repository.segment('20260901.120000', 0)['segment_id'])

  def test_rejects_paths_outside_repository(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      repository = SegmentModelRepository(temp_dir)
      with self.assertRaises(SegmentModelNotFound):
        repository.model_properties('../model')

  @staticmethod
  def create_training_model(root, model_id, pid):
    model_path = root / model_id
    model_path.mkdir()
    (model_path / 'traits.json').write_text(json.dumps({
      'status': 'in_progress',
      'pid': pid,
      'train_start': '2026-09-01T11:00:00Z',
      'progress': {
        'stage': 'discovery',
        'completed_batches': 2,
        'total_batches': 10,
      },
    }))

  @staticmethod
  def create_model(root, model_id):
    model_path = root / model_id
    model_path.mkdir()
    values = {
      'config.json': {
        'data': {'url_buckets': 100, 'windows_seconds': [60, 3600], 'n_values': [1, 2]},
        'model': {'candidates': 2, 'aggregation': 'softmax_max'},
        'training': {'device': 'cuda'},
      },
      'training.json': [{
        'epoch': 0,
        'stage': 'discovery',
        'total': 0.4,
        'ctr': 0.3,
        'validation_loss': 0.25,
        'best': True,
      }],
      'training-summary.json': {
        'epochs_completed': 1,
        'best_epoch': 0,
        'training_rows': 1000,
      },
      'metrics.json': {
        'soft_ctr': {'logloss': 0.201, 'roc_auc': 0.71, 'pr_auc': 0.02},
        'hard_ctr': {'logloss': 0.205, 'roc_auc': 0.69, 'pr_auc': 0.019},
        'soft_hard': {
          'segment_agreement_rate': 0.97,
          'mean_absolute_activation_difference': 0.03,
        },
        'recovery': {'url_f1': 0.8},
        'diagnostics': {
          'empty_segments': 1,
          'highly_similar_segment_pairs': 0,
          'candidates': [{
            'segment_id': 0,
            'average_activation': 0.25,
            'top_url_gates': [],
          }],
        },
      },
      'segments.json': [
        {
          'segment_id': 0,
          'urls': ['https://example.com/path'],
          'window_seconds': 3600,
          'min_visits': 2,
          'forest_split_count': 3,
          'relations': [],
        },
        {
          'segment_id': 1,
          'urls': [],
          'window_seconds': 60,
          'min_visits': 1,
          'forest_split_count': 0,
          'relations': [],
        },
      ],
      'traits.json': {
        'status': 'published',
        'train_start': '2026-09-01T10:00:00Z',
        'train_end': '2026-09-01T10:30:00Z',
      },
    }
    for file_name, value in values.items():
      (model_path / file_name).write_text(json.dumps(value))
    return model_path


if __name__ == '__main__':
  unittest.main()
