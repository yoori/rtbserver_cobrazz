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
  feature_importance_item,
  render_index_page,
)


class CTRModelWebApplicationTest(unittest.TestCase):
  def model_properties(self, features_importance):
    summary = {
      'id': '20260823.120000',
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
        'validation': {'logloss': decimal.Decimal('0.125')},
      },
    }

  def test_renders_model_list_and_new_traits_format(self):
    properties = self.model_properties([{
      'score': decimal.Decimal('0.00008901938322533171'),
      'feature': 'channel:614065',
      'name': 'Account <one>/Channel & one',
    }])

    page = render_index_page([properties['summary']], properties)

    self.assertIn('20260823.120000', page)
    self.assertIn('0.00008901938322533171', page)
    self.assertIn('channel:614065', page)
    self.assertIn('Account &lt;one&gt;/Channel &amp; one', page)
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
