#!/usr/bin/env python3.12

import json
import pathlib
import sys
import tempfile
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from rtbserver_utils.CatBoostTrainer import CatBoostTrainer


class ModelStub:
  def __init__(self, feature_importance=None):
    self.feature_importance = feature_importance or []

  def save_model(self, file_name):
    pathlib.Path(file_name).write_bytes(b'catboost-model')

  def get_feature_importance(self):
    return self.feature_importance


class FailingModelStub:
  def save_model(self, file_name):
    pathlib.Path(file_name).write_bytes(b'incomplete-model')
    raise RuntimeError('save failed')


class CatBoostTrainerTest(unittest.TestCase):
  def test_save_campaign_manager_model(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      feature_config = temp_path / 'features.json'
      feature_config.write_text(json.dumps({
        'features_dimension': 14,
        'features': [['publisher'], ['campaign', 'ccid']],
      }))

      output_dir = temp_path / 'CTRConfig'
      trainer = CatBoostTrainer(features_config_file=feature_config)
      result_dir = trainer.save_campaign_manager_model(
        ModelStub(),
        output_dir,
        timestamp='20260819.120000',
        algorithm_id='test_catboost')

      self.assertEqual(output_dir / '20260819.120000', result_dir)
      self.assertEqual(b'catboost-model', (result_dir / 'model.cbm').read_bytes())
      with (result_dir / 'config.json').open() as input_file:
        config = json.load(input_file)

      self.assertEqual(2, config['version'])
      self.assertEqual(0, config['default_weight'])
      algorithm = config['algorithms'][0]
      self.assertEqual('test_catboost', algorithm['id'])
      self.assertEqual(1, algorithm['weight'])
      model = algorithm['models'][0]
      self.assertEqual('catboost', model['method'])
      self.assertEqual(16384, model['features_size'])
      self.assertEqual(
        [['publisher'], ['campaign', 'ccid']],
        model['features'])
      self.assertEqual('model.cbm', model['file'])
      with (result_dir / 'traits.json').open() as input_file:
        traits = json.load(input_file)
      self.assertEqual({'features_importance': []}, traits)
      self.assertEqual([], list(temp_path.glob('.CTRConfig.*')))

  def test_dictionary_filters_features_and_generates_traits(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      feature_config = temp_path / 'features.json'
      feature_config.write_text(json.dumps({
        'features_dimension': 14,
        'features': [
          ['publisher'],
          ['tag'],
          ['group', 'ccid'],
          ['campaign_freq_log'],
        ],
      }))
      feature_dictionary = temp_path / 'features.csv'
      feature_dictionary.write_text(
        '2,publisher:123\n'
        '3,"ccg:44,ccid:55"\n'
        '3,campaignfreqlog:3\n')

      output_dir = temp_path / 'CTRConfig'
      trainer = CatBoostTrainer(features_config_file=feature_config)
      result_dir = trainer.save_campaign_manager_model(
        ModelStub([0.0, 1.25, 2.5]),
        output_dir,
        timestamp='20260819.120000',
        feature_dictionary_file=feature_dictionary)

      with (result_dir / 'config.json').open() as input_file:
        config = json.load(input_file)
      self.assertEqual(
        [['publisher'], ['group', 'ccid'], ['campaign_freq_log']],
        config['algorithms'][0]['models'][0]['features'])

      with (result_dir / 'traits.json').open() as input_file:
        traits = json.load(input_file)
      self.assertEqual(
        {
          'features_importance': [
            {'2.5': 'ccg:44,ccid:55'},
            {'2.5': 'campaignfreqlog:3'},
            {'1.25': 'publisher:123'},
          ],
        },
        traits)

  def test_failed_save_is_not_published(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      feature_config = temp_path / 'features.json'
      feature_config.write_text(json.dumps({
        'features_dimension': 14,
        'features': [['publisher']],
      }))

      output_dir = temp_path / 'CTRConfig'
      trainer = CatBoostTrainer(features_config_file=feature_config)
      with self.assertRaisesRegex(RuntimeError, 'save failed'):
        trainer.save_campaign_manager_model(
          FailingModelStub(),
          output_dir,
          timestamp='20260819.120000')

      self.assertFalse((output_dir / '20260819.120000').exists())
      self.assertEqual([], list(temp_path.glob('.CTRConfig.*')))

  def test_rejects_feature_size_mismatch(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      feature_config = temp_path / 'features.json'
      feature_config.write_text(json.dumps({
        'features_dimension': 15,
        'features': [['publisher']],
      }))

      with self.assertRaisesRegex(ValueError, 'Feature dimension mismatch'):
        CatBoostTrainer(
          features_dimension=14,
          features_config_file=feature_config)


if __name__ == '__main__':
  unittest.main()
