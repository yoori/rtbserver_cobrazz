#!/usr/bin/env python3.12

import asyncio
import decimal
import pathlib
import sys
import types
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

try:
  import fastapi
  FASTAPI_AVAILABLE = True
except ModuleNotFoundError:
  FASTAPI_AVAILABLE = False
  fastapi = types.ModuleType('fastapi')
  fastapi.FastAPI = object
  fastapi.HTTPException = Exception
  fastapi.Query = lambda *args, **kwargs: None
  responses = types.ModuleType('fastapi.responses')
  responses.FileResponse = object
  responses.HTMLResponse = object
  responses.Response = object
  fastapi.responses = responses
  sys.modules['fastapi'] = fastapi
  sys.modules['fastapi.responses'] = responses

from rtbserver_utils.CTRModelWebApplication import (
  create_application,
  duration_text,
  feature_importance_item,
  render_index_page,
)


class CTRModelWebApplicationTest(unittest.TestCase):
  def model_properties(self, features_importance):
    summary = {
      'id': '20260823.120000',
      'status': 'published',
      'train_start': '2026-08-23T10:00:00Z',
      'train_end': '2026-08-23T12:00:00Z',
      'algorithm_id': 'catboost',
      'method': 'catboost',
      'feature_groups': [['tag'], ['geoch', 'userch']],
      'feature_groups_count': 2,
      'features_importance_count': len(features_importance),
    }
    return {
      'summary': summary,
      'config': {},
      'traits': {
        'features_importance': features_importance,
        'logloss_history': [
          {'step': 1, 'train': '0.0123', 'test': '0.0203'},
          {'step': 2, 'train': '0.0119', 'test': '0.0056'},
        ],
        'dataset_sizes': {
          'train': {'rows': 1000000, 'clicks': 2001},
          'test': {'rows': 300000, 'clicks': 570},
          'final_test': {'rows': 200000, 'clicks': 390},
        },
        'ctr_thresholds': [
          {
            'ctr_goal': '0',
            'impressions': 200000,
            'clicks': 390,
            'actual_ctr': '0.00195',
            'average_predicted_ctr': '0.002104321',
          },
          {
            'ctr_goal': '0.001',
            'impressions': 150000,
            'clicks': 350,
            'actual_ctr': '0.002333333333',
            'average_predicted_ctr': '0.002671234',
          },
        ],
        'validation': {'logloss': decimal.Decimal('0.125')},
      },
    }

  def test_renders_model_list_and_new_traits_format(self):
    properties = self.model_properties([{
      'score': decimal.Decimal('0.00008901938322533171'),
      'feature': 'channel:614065',
      'name': 'Account <one>/Channel & one',
      'yes_share': decimal.Decimal('12.5123456789'),
      'yes_ctr': decimal.Decimal('0.004123987'),
      'no_ctr': decimal.Decimal('0.001987654'),
    }])

    page = render_index_page([properties['summary']], properties)

    self.assertIn('20260823.120000', page)
    self.assertIn('Train start', page)
    self.assertIn('2026-08-23T10:00:00Z', page)
    self.assertIn('Train end', page)
    self.assertIn('2026-08-23T12:00:00Z', page)
    self.assertIn('0.00008901938322533171', page)
    self.assertIn('channel:614065', page)
    self.assertIn('Account &lt;one&gt;/Channel &amp; one', page)
    self.assertIn('Yes share, %', page)
    self.assertIn('12.512345', page)
    self.assertIn('0.004123', page)
    self.assertIn('0.001987', page)
    self.assertNotIn('12.512346', page)
    self.assertIn('Logloss history', page)
    self.assertIn('Train Logloss', page)
    self.assertIn('Test Logloss', page)
    self.assertIn('0.012300000', page)
    self.assertIn('class="chart-line train"', page)
    self.assertIn('class="chart-line test"', page)
    self.assertIn('Dataset sizes', page)
    self.assertIn('<td>Train</td>', page)
    self.assertIn('1000000', page)
    self.assertIn('2001', page)
    self.assertIn('0.002001', page)
    self.assertIn('<td>Final test</td>', page)
    self.assertIn('CTR threshold calibration', page)
    self.assertIn('Actual CTR', page)
    self.assertIn('Average predicted CTR', page)
    self.assertIn('0.001950', page)
    self.assertIn('0.002104', page)
    self.assertNotIn('0.002105', page)
    self.assertIn('tag, geoch + userch', page)
    self.assertIn('&quot;logloss&quot;:0.125', page)
    self.assertIn('aria-current="page"', page)

  def test_renders_legacy_traits_format(self):
    properties = self.model_properties([{
      '17.64012480599441': 'channel:3604081',
    }])

    page = render_index_page([properties['summary']], properties)

    self.assertIn('17.64012480599441', page)
    self.assertIn('channel:3604081', page)

  def test_renders_three_model_components(self):
    properties = self.model_properties([])
    properties['summary'].update({
      'components_count': 3,
      'published_component': 'stable_common',
    })
    component_base = {
      'feature_groups': [['campaign']],
      'features_importance': [],
      'logloss_history': [],
      'dataset_sizes': {},
      'ctr_thresholds': [],
    }
    properties['traits'] = {
      'components': {
        'common': {
          **component_base,
          'file': 'common.cbm',
          'metrics_prediction': 'sigmoid(common)',
        },
        'campaign_correction': {
          **component_base,
          'file': 'campaign-correction.cbm',
          'metrics_prediction': 'sigmoid(common + campaign_correction)',
        },
        'stable_common': {
          **component_base,
          'file': 'model.cbm',
          'metrics_prediction': 'sigmoid(stable_common)',
          'published': True,
        },
      },
    }

    page = render_index_page([properties['summary']], properties)

    self.assertIn('>Common</h2>', page)
    self.assertIn('>Campaign correction</h2>', page)
    self.assertIn('>Stable common</h2>', page)
    self.assertIn('campaign-correction.cbm', page)
    self.assertIn('sigmoid(common + campaign_correction)', page)
    self.assertIn('component-published', page)
    self.assertEqual(3, page.count('class="model-component"'))
    self.assertIn('class="component-sidebar"', page)
    self.assertIn('href="#component-stable-common" aria-current="page"', page)
    self.assertIn(
      '<article class="model-component" id="component-common" hidden>',
      page)

  def test_renders_flat_models_and_campaign_name(self):
    properties = self.model_properties([])
    properties['summary']['components_count'] = 4
    properties['traits'] = {
      'models': [
        {'name': 'common', 'kind': 'common', 'features_importance': []},
        {
          'name': 'common_denoise',
          'kind': 'denoise_residual',
          'features_importance': [],
        },
        {
          'name': 'common_stable',
          'kind': 'common_stable',
          'runtime': True,
          'features_importance': [],
        },
        {
          'name': 'campaign_123',
          'kind': 'campaign',
          'runtime': True,
          'db_campaign_id': 123,
          'campaign_name': 'Campaign <name>',
          'weight': '0.7',
          'status': 'completed',
          'train_start': '2026-08-23T11:00:00Z',
          'train_end': '2026-08-23T11:30:00Z',
          'features_importance': [],
        },
      ],
    }

    page = render_index_page([properties['summary']], properties)

    self.assertIn('Common denoise', page)
    self.assertIn('Common stable', page)
    self.assertIn('Campaign 123 — Campaign &lt;name&gt;', page)
    self.assertIn('Models within bundle', page)
    self.assertIn('Core models<span>3</span>', page)
    self.assertIn('Campaign models<span>1</span>', page)
    self.assertIn('id="component-status-filter"', page)
    self.assertIn(
      'href="#component-common-stable" aria-current="page"',
      page)
    self.assertIn(
      'class="component-link component-link-campaign"',
      page)
    self.assertIn(
      'title="Campaign 123 — Campaign &lt;name&gt;"',
      page)
    self.assertIn(
      '.component-link-campaign span { font-size: 12px; }',
      page)
    self.assertIn('campaign_123 123 campaign &lt;name&gt;', page)
    self.assertIn('2026-08-23T11:00:00Z', page)
    self.assertIn('2026-08-23T11:30:00Z', page)
    self.assertIn('.shell { height: 100vh;', page)
    self.assertIn('.sidebar { height: 100%; overflow-y: auto;', page)
    self.assertIn('main { min-width: 0; height: 100%; overflow-y: auto;', page)
    self.assertIn('.shell { height: auto; min-height: 100vh;', page)

  def test_renders_in_progress_model_with_only_train_start(self):
    properties = {
      'summary': {
        'id': '~20260824.155515',
        'status': 'in_progress',
        'train_start': '2026-08-24T15:55:15Z',
      },
      'config': {},
      'traits': {},
    }

    page = render_index_page([properties['summary']], properties)

    self.assertIn('Training in progress', page)
    self.assertIn('2026-08-24T15:55:15Z', page)
    self.assertIn(
      'data-model-id="~20260824.155515" data-model-status="in_progress"',
      page)
    self.assertIn('data-state-signature="', page)
    self.assertIn('data-refresh-message>Live updates every 5 s', page)
    self.assertIn('const refreshInterval = 5000;', page)
    self.assertIn("cache: 'no-store'", page)
    self.assertIn("document.addEventListener('visibilitychange'", page)
    self.assertIn("modelId.startsWith('~')", page)
    self.assertIn('restoreUiState(importedMain, state);', page)
    self.assertIn('mainScroll: main.scrollTop', page)
    self.assertIn('main.scrollTop = state.mainScroll;', page)
    self.assertNotIn('Train end', page)
    self.assertNotIn('Feature importance', page)
    self.assertNotIn('>Config<', page)

  def test_renders_in_progress_model_plan_and_model_timestamps(self):
    properties = {
      'summary': {
        'id': '~20260824.155515',
        'status': 'in_progress',
        'train_start': '2026-08-24T15:55:15Z',
        'models_count': 4,
        'campaign_models_count': 1,
        'completed_models_count': 1,
      },
      'config': {},
      'traits': {
        'prepare': {
          'status': 'completed',
          'train_start': '2026-08-24T15:55:15Z',
          'train_end': '2026-08-24T15:59:00Z',
          'train_steps': [
            {
              'id': 'feature_selection_export_001',
              'title': 'Feature selection: export dataset 1/10',
              'started': '2026-08-24T15:56:00Z',
              'ended': '2026-08-24T15:57:00Z',
            },
          ],
        },
        'models': [
          {
            'name': 'common',
            'kind': 'common',
            'status': 'completed',
            'train_start': '2026-08-24T16:00:00Z',
            'train_end': '2026-08-24T16:30:00Z',
            'train_steps': [
              {
                'id': 'training_fit_001',
                'title': 'Common training: fit and validate 1/30',
                'started': '2026-08-24T16:01:00Z',
                'ended': '2026-08-24T16:02:00Z',
              },
            ],
          },
          {
            'name': 'common_denoise',
            'kind': 'denoise_residual',
            'status': 'planned',
          },
          {
            'name': 'common_stable',
            'kind': 'common_stable',
            'status': 'planned',
          },
          {
            'name': 'campaign_123',
            'kind': 'campaign',
            'status': 'training',
            'db_campaign_id': 123,
            'campaign_name': 'Campaign <name>',
            'eligible_training_impressions': 150000,
            'train_start': '2026-08-24T16:31:00Z',
            'train_steps': [
              {
                'id': 'campaign_training_fit_001',
                'title': 'Campaign residual training: fit 1/30',
                'started': '2026-08-24T16:32:00Z',
                'ended': None,
              },
            ],
          },
        ],
      },
    }

    page = render_index_page([properties['summary']], properties)

    self.assertIn('<dt>Planned models</dt><dd>4</dd>', page)
    self.assertIn('<dt>Campaign models</dt><dd>1</dd>', page)
    self.assertIn('<dt>Completed models</dt><dd>1</dd>', page)
    self.assertIn('Campaign 123 — Campaign &lt;name&gt;', page)
    self.assertIn('150000', page)
    self.assertIn('2026-08-24T16:31:00Z', page)
    self.assertIn('component-status-training', page)
    self.assertIn('Models within bundle', page)
    self.assertIn('data-component-group="prepare"', page)
    self.assertIn('<h3>Prepare<span>1</span></h3>', page)
    self.assertIn('Feature selection: export dataset 1/10', page)
    self.assertIn(
      'Feature selection: export dataset 1/10 : 1m 00s',
      page)
    self.assertIn(
      '2026-08-24T15:56:00Z → 2026-08-24T15:57:00Z',
      page)
    self.assertIn('train-step-completed', page)
    self.assertIn('train-step-active', page)
    self.assertIn(
      '.train-step-completed .train-step-title { text-decoration: line-through; }',
      page)
    self.assertIn(
      'href="#component-campaign-123" aria-current="page"',
      page)
    self.assertNotIn('Feature importance', page)

  def test_renders_interrupted_training_and_model_phase(self):
    properties = {
      'summary': {
        'id': '~20260824.155515',
        'status': 'interrupted',
        'train_start': '2026-08-24T15:55:15Z',
        'train_end': '2026-08-24T16:05:00Z',
        'models_count': 2,
        'campaign_models_count': 1,
        'completed_models_count': 1,
        'interrupted_models_count': 1,
      },
      'config': {},
      'traits': {
        'status': 'interrupted',
        'models': [
          {
            'name': 'common_stable',
            'kind': 'common_stable',
            'status': 'completed',
          },
          {
            'name': 'campaign_123',
            'kind': 'campaign',
            'status': 'interrupted',
            'db_campaign_id': 123,
            'campaign_name': 'Interrupted campaign',
            'train_start': '2026-08-24T16:00:00Z',
            'train_end': '2026-08-24T16:05:00Z',
            'train_steps': [{
              'id': 'campaign_training_fit_001',
              'title': 'Campaign residual training: fit 1/30',
              'started': '2026-08-24T16:03:00Z',
              'ended': None,
            }],
          },
        ],
      },
    }

    page = render_index_page([properties['summary']], properties)

    self.assertIn('Training interrupted', page)
    self.assertNotIn('data-refresh-message>Live updates every 5 s', page)
    self.assertIn('<dt>Interrupted models</dt><dd>1</dd>', page)
    self.assertIn('component-status-interrupted', page)
    self.assertIn('train-step-interrupted', page)
    self.assertIn('Interrupted campaign', page)
    self.assertIn('2026-08-24T16:05:00Z', page)
    self.assertNotIn('>Config<', page)

  def test_formats_completed_step_duration(self):
    self.assertEqual(
      '7s',
      duration_text(
        '2026-08-24T15:56:00Z',
        '2026-08-24T15:56:07Z'))
    self.assertEqual(
      '1h 02m 03s',
      duration_text(
        '2026-08-24T15:56:00Z',
        '2026-08-24T16:58:03Z'))
    self.assertEqual('', duration_text('invalid', 'invalid'))

  def test_invalid_scores_do_not_break_bar_scale(self):
    for score in ('invalid', 'NaN', 'Infinity'):
      with self.subTest(score=score):
        item = feature_importance_item({
          'score': score,
          'feature': 'channel:1',
          'name': None,
        })
        self.assertEqual(decimal.Decimal(0), item['score'])
        self.assertEqual('', item['name'])

  @unittest.skipUnless(FASTAPI_AVAILABLE, 'FastAPI is not installed')
  def test_index_route_returns_html_response(self):
    properties = self.model_properties([{
      'score': 1,
      'feature': 'tag:1',
      'name': 'Account/Tag',
    }])

    class Repository:
      def all_model_ids(self):
        return [properties['summary']['id']]

      def model_ids(self):
        return [properties['summary']['id']]

      def model_summary(self, model_id):
        self.assert_model_id(model_id)
        return properties['summary']

      def model_properties(self, model_id):
        self.assert_model_id(model_id)
        return properties

      @staticmethod
      def assert_model_id(model_id):
        if model_id != properties['summary']['id']:
          raise AssertionError('Unexpected model id')

    application = create_application(Repository())
    route = next(route for route in application.routes if route.path == '/')
    response = asyncio.run(route.endpoint(model_id=properties['summary']['id']))

    self.assertEqual('text/html', response.media_type)
    self.assertIn(b'Account/Tag', response.body)


if __name__ == '__main__':
  unittest.main()
