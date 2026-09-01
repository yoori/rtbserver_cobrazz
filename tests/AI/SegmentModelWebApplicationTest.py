#!/usr/bin/python3.12

import asyncio
import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelWebApplication import (
  create_application,
  render_index_page,
)


class SegmentModelWebApplicationTest(unittest.TestCase):
  def test_renders_training_quality_diagnostics_and_segments(self):
    properties = self.properties()
    segments = {
      'total': 1,
      'items': [{
        'segment_id': 7,
        'urls': ['https://example.com/<unsafe>'],
        'window_seconds': 3600,
        'min_visits': 2,
        'average_activation': 0.25,
        'forest_split_count': 3,
        'relations': [],
      }],
    }

    page = render_index_page(
      [properties['summary']],
      properties,
      segments,
      None,
      '/segments')

    self.assertIn('Training', page)
    self.assertIn('Soft versus hard', page)
    self.assertIn('Diagnostics', page)
    self.assertIn('Agreement rate', page)
    self.assertIn('URL F1', page)
    self.assertIn('Max URL Jaccard', page)
    self.assertIn('Activation duplicate', page)
    self.assertIn('Candidate opening', page)
    self.assertIn('LR multiplier', page)
    self.assertIn('1010', page)
    self.assertIn('Segment models', page)
    self.assertIn('https://example.com/&lt;unsafe&gt;', page)
    self.assertNotIn('https://example.com/<unsafe>', page)
    self.assertIn('/segments/models/20260901.120000/segments/7/view', page)
    self.assertIn('class="chart-line', page)

  def test_exposes_api_below_configured_url_path(self):
    application = create_application(FakeRepository(), '/segments')
    endpoints = {
      route.path: route.endpoint
      for route in application.routes
      if hasattr(route, 'endpoint')
    }

    health = asyncio.run(endpoints['/segments/health']())
    models = asyncio.run(endpoints['/segments/models']())
    segment = asyncio.run(
      endpoints['/segments/models/{model_id}/segments/{segment_id}'](
        '20260901.120000',
        7))

    self.assertEqual({'status': 'ok'}, self.response_json(health))
    self.assertEqual(
      '20260901.120000',
      self.response_json(models)['items'][0]['id'])
    self.assertEqual(7, self.response_json(segment)['segment_id'])

  @staticmethod
  def response_json(response):
    import json
    return json.loads(response.body)

  @staticmethod
  def properties():
    return {
      'summary': {
        'id': '20260901.120000',
        'status': 'published',
        'train_start': '2026-09-01T10:00:00Z',
        'train_end': '2026-09-01T10:30:00Z',
        'segments_count': 1,
        'soft_logloss': 0.201,
      },
      'traits': {},
      'config': {
        'data': {
          'url_buckets': 100,
          'windows_seconds': [60, 3600],
          'n_values': [1, 2],
        },
        'model': {'candidates': 2, 'aggregation': 'softmax_max'},
        'training': {'device': 'cuda'},
        'candidate_opening': {'enabled': True, 'mode': 'fixed'},
      },
      'training': [{
        'epoch': 0,
        'stage': 'discovery',
        'total': 0.4,
        'ctr': 0.3,
        'validation_loss': 0.25,
        'sparsity': 0.01,
        'binarization': 0.02,
        'url_duplicate': 0.03,
        'activation_duplicate': 0.04,
        'duplicate_regularization_scale': 0.5,
        'candidate_opening': {
          'candidate_mask': [1, 0],
          'active_candidates': 1,
        },
        'best': True,
      }],
      'training_summary': {
        'epochs_completed': 1,
        'best_epoch': 0,
        'training_rows': 1000,
      },
      'metrics': {
        'soft_ctr': {'logloss': 0.201, 'roc_auc': 0.71, 'pr_auc': 0.02},
        'hard_ctr': {'logloss': 0.205, 'roc_auc': 0.69, 'pr_auc': 0.019},
        'soft_hard': {
          'segment_agreement_rate': 0.97,
          'mean_absolute_activation_difference': 0.03,
        },
        'recovery': {'url_f1': 0.8},
        'diagnostics': {
          'empty_segments': 0,
          'url_gate_fraction_ambiguous': 0.1,
          'candidate_opening': {
            'enabled': True,
            'mode': 'fixed',
            'candidate_mask': [1, 1],
            'active_candidates': 2,
            'joint_finetune_active': False,
            'candidates': [{
              'index': 0,
              'opened': True,
              'epoch_opened': 0,
              'learning_rate_multiplier': 0.1,
              'extracted_urls': ['a.com', 'b.com'],
              'top_url_gates': [{'url': 'a.com', 'gate': 0.9}],
              'forest_soft_importance': 0.7,
              'forest_hard_split_count': 1010,
            }],
          },
          'candidate_duplicates': {
            'max_pairwise_url_jaccard': 0.95,
            'mean_pairwise_url_jaccard': 0.4,
            'max_activation_similarity': 0.99,
            'mean_activation_similarity': 0.5,
            'number_of_pairs_jaccard_above_0_8': 1,
            'number_of_pairs_jaccard_above_0_95': 0,
            'number_of_activation_duplicate_pairs': 1,
            'number_of_reseed_duplicate_pairs': 0,
            'most_similar_pairs': [],
          },
        },
      },
      'source': None,
      'files': ['config.json', 'metrics.json', 'segments.json'],
    }


class FakeRepository:
  def all_model_ids(self):
    return ['20260901.120000']

  def model_summary(self, model_id):
    return {
      'id': model_id,
      'status': 'published',
      'segments_count': 1,
    }

  def latest_model_id(self):
    return '20260901.120000'

  def model_properties(self, model_id):
    result = SegmentModelWebApplicationTest.properties()
    result['summary']['id'] = model_id
    return result

  def segments(self, model_id, offset, limit, search):
    del model_id
    del offset
    del limit
    del search
    return {'total': 1, 'items': [self.segment('', 7)]}

  def segment(self, model_id, segment_id):
    del model_id
    return {
      'segment_id': segment_id,
      'urls': [],
      'relations': [],
    }

  def model_file(self, model_id, file_name):
    del model_id
    del file_name
    raise AssertionError('not used')


if __name__ == '__main__':
  unittest.main()
