#!/usr/bin/env python3.12

import json
import pathlib
import sys
import tempfile
import unittest
import unittest.mock

import numpy
from catboost import CatBoostClassifier, Pool


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from rtbserver_utils.CatBoostTrainer import CatBoostTrainer
from rtbserver_utils.CTRModelTraits import section_value
from rtbserver_utils.CatBoostModelEvaluator import (
  ctr_threshold_statistics,
  evaluate_model,
)
from rtbserver_utils.CatBoostTrainChunk import train_chunk


class ModelStub:
  def __init__(self, feature_importance=None):
    self.feature_importance = feature_importance or []
    self.unused_features_dropped = False

  def save_model(self, file_name):
    if not self.unused_features_dropped:
      raise RuntimeError('unused features were not dropped')
    pathlib.Path(file_name).write_bytes(b'catboost-model')

  def get_feature_importance(self):
    return self.feature_importance

  def drop_unused_features(self):
    self.unused_features_dropped = True


class FailingModelStub(ModelStub):
  def save_model(self, file_name):
    if not self.unused_features_dropped:
      raise RuntimeError('unused features were not dropped')
    pathlib.Path(file_name).write_bytes(b'incomplete-model')
    raise RuntimeError('save failed')


class FeatureNameResolverStub:
  def resolve(self, features):
    return {
      feature: 'Account/' + feature
      for feature in features
      if feature != 'campaignfreqlog:3'
    }


class FeatureStatisticsStub:
  total_impressions = 100
  total_clicks = 10

  def get(self, index):
    return {
      2: (20, 4),
    }.get(index, (0, 0))


class CatBoostTrainerTest(unittest.TestCase):
  def test_real_catboost_baseline_survives_continued_training(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      svm_file = temp_path / 'train.libsvm'
      svm_file.write_text('\n'.join(
        str(index % 2) + ' 1:' + str(index % 3)
        for index in range(20)) + '\n')
      baseline_file = temp_path / 'baseline'
      baseline_file.write_text('\n'.join(
        str((index % 5 - 2) / 10)
        for index in range(20)) + '\n')
      first_model = temp_path / 'first.cbm'
      second_model = temp_path / 'second.cbm'

      first_metrics = train_chunk(
        svm_file,
        first_model,
        iterations=2,
        baseline_file=baseline_file)
      second_metrics = train_chunk(
        svm_file,
        second_model,
        iterations=2,
        baseline_file=baseline_file,
        merge_model=first_model)
      raw_predictions_file = temp_path / 'raw-predictions'
      model_raw_predictions_file = temp_path / 'model-raw-predictions'
      evaluation = evaluate_model(
        second_model,
        svm_file,
        baseline_file=baseline_file,
        raw_predictions_file=raw_predictions_file,
        model_raw_predictions_file=model_raw_predictions_file)
      weighted_evaluation = evaluate_model(
        second_model,
        svm_file,
        baseline_file=baseline_file,
        prediction_weights=[0, 0.5, 1])

      self.assertGreater(first_metrics['Logloss'], 0)
      self.assertGreater(second_metrics['Logloss'], 0)
      self.assertGreater(evaluation['Logloss'], 0)
      self.assertEqual(
        [0, 0.5, 1],
        [item['weight'] for item in weighted_evaluation['weighted_logloss']])
      self.assertEqual(
        20,
        len(raw_predictions_file.read_text().splitlines()))
      numpy.testing.assert_allclose(
        numpy.loadtxt(raw_predictions_file) -
        numpy.loadtxt(model_raw_predictions_file),
        numpy.loadtxt(baseline_file),
        rtol=1e-14,
        atol=1e-14)

  def test_real_catboost_accepts_chunk_without_clicks(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      svm_file = temp_path / 'all-negative.libsvm'
      svm_file.write_text('\n'.join(
        '0 1:' + str(index % 3)
        for index in range(20)) + '\n')
      model_file = temp_path / 'model.cbm'

      metrics = train_chunk(svm_file, model_file, iterations=2)

      self.assertGreater(metrics['Logloss'], 0)
      self.assertTrue(model_file.is_file())

  def test_real_catboost_accepts_ssp_ctr_soft_labels(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      svm_file = temp_path / 'ssp-ctr.libsvm'
      svm_file.write_text('\n'.join(
        str(0.001 * (index + 1)) + ' 1:' + str(index % 3)
        for index in range(20)) + '\n')
      model_file = temp_path / 'model.cbm'

      metrics = train_chunk(
        svm_file,
        model_file,
        iterations=2,
        loss_function='CrossEntropy')

      self.assertGreater(metrics['Logloss'], 0)
      self.assertTrue(model_file.is_file())

  def test_save_campaign_manager_model_bundle(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      common_config = temp_path / 'common.json'
      common_config.write_text(json.dumps({
        'features_dimension': 14,
        'features': [['publisher'], ['campaign']],
      }))
      correction_config = temp_path / 'correction.json'
      correction_config.write_text(json.dumps({
        'features_dimension': 14,
        'features': [['campaign'], ['campaign', 'publisher']],
      }))
      common_trainer = CatBoostTrainer(features_config_file=common_config)
      correction_trainer = CatBoostTrainer(
        features_config_file=correction_config)
      output_dir = temp_path / 'CTRConfig'

      result_dir = common_trainer.save_campaign_manager_model_bundle(
        output_dir,
        [
          {
            'name': 'common',
            'trainer': common_trainer,
            'model': ModelStub(),
            'dataset_sizes': {'train': {'rows': 10, 'clicks': 1}},
            'traits': {
              'kind': 'common',
              'runtime': False,
              'properties': [
                {'train_logloss': 0.1},
                {'val_logloss': 0.2},
              ],
            },
          },
          {
            'name': 'common_denoise',
            'trainer': correction_trainer,
            'model': ModelStub(),
            'traits': {'kind': 'denoise_residual', 'runtime': False},
          },
          {
            'name': 'common_stable',
            'trainer': common_trainer,
            'model': ModelStub(),
            'traits': {
              'kind': 'common_stable',
              'runtime': True,
              'status': 'completed',
              'train_start': '2026-08-24T12:15:00Z',
              'train_end': '2026-08-24T12:45:00Z',
            },
          },
          {
            'name': 'common_ssp_ctr',
            'trainer': common_trainer,
            'model': ModelStub(),
            'traits': {
              'kind': 'common_ssp_ctr',
              'runtime': False,
              'properties': [{'ssp_ctr_logloss': 0.0125}],
            },
          },
          {
            'name': 'campaign_123',
            'trainer': correction_trainer,
            'model': ModelStub(),
            'traits': {
              'kind': 'campaign',
              'runtime': True,
              'db_campaign_id': 123,
              'runtime_campaign_group_id': 123,
              'campaign_name': 'Campaign name',
              'weight': 0.7,
            },
          },
        ],
        timestamp='20260824.120000',
        algorithm_id='aligned_catboost',
        prepare={
          'status': 'completed',
          'train_steps': [{
            'id': 'export_001',
            'title': 'Export 1/1',
            'started': '2026-08-24T12:00:00Z',
            'ended': '2026-08-24T12:05:00Z',
          }],
        },
        train_start='2026-08-24T12:00:00Z',
        train_end='2026-08-24T13:00:00Z')

      self.assertTrue((result_dir / 'common.cbm').is_file())
      self.assertTrue((result_dir / 'common_denoise.cbm').is_file())
      self.assertTrue((result_dir / 'model.cbm').is_file())
      self.assertTrue((result_dir / 'common_ssp_ctr.cbm').is_file())
      self.assertTrue((result_dir / 'campaign_123.cbm').is_file())
      self.assertEqual(
        '123\n',
        (result_dir / 'campaign_123.campaigns').read_text())
      config = json.loads((result_dir / 'config.json').read_text())
      self.assertEqual(4, config['version'])
      algorithm = config['algorithms'][0]
      self.assertEqual('logit_sum', algorithm['aggregation'])
      self.assertEqual(2, len(algorithm['models']))
      self.assertEqual('as_is', algorithm['models'][0]['predict_postprocess'])
      self.assertEqual('model.cbm', algorithm['models'][0]['file'])
      self.assertEqual(0.7, algorithm['models'][1]['weight'])
      self.assertEqual(
        'campaign_123.campaigns',
        algorithm['models'][1]['campaigns_whitelist_file'])
      traits = json.loads((result_dir / 'traits.json').read_text())
      self.assertEqual([
        'common',
        'common_denoise',
        'common_stable',
        'common_ssp_ctr',
        'campaign_123',
      ], [model['name'] for model in traits['models']])
      self.assertEqual(
        'Campaign name',
        traits['models'][4]['campaign_name'])
      self.assertEqual(
        '2026-08-24T12:15:00Z',
        traits['models'][2]['train_start'])
      self.assertEqual(
        '2026-08-24T12:45:00Z',
        traits['models'][2]['train_end'])
      prepare_traits = json.loads(
        (result_dir / traits['prepare']['artifact']).read_text())
      self.assertEqual(
        '2026-08-24T12:05:00Z',
        section_value(
          prepare_traits,
          'processing_steps')[0]['ended'])
      common_traits = json.loads(
        (result_dir / traits['models'][0]['artifact']).read_text())
      self.assertEqual(2, common_traits['artifact_version'])
      self.assertEqual([
        'properties',
        'feature_groups',
        'datasets',
        'ctr_thresholds',
        'training_report',
        'feature_importance',
      ], [section['id'] for section in common_traits['sections']])
      self.assertNotIn('properties', common_traits)
      self.assertNotIn('features_importance', common_traits)
      self.assertEqual(
        [{'train_logloss': 0.1}, {'val_logloss': 0.2}],
        section_value(common_traits, 'properties'))
      ssp_traits = json.loads(
        (result_dir / traits['models'][3]['artifact']).read_text())
      self.assertEqual(
        [{'ssp_ctr_logloss': 0.0125}],
        section_value(ssp_traits, 'properties'))
      self.assertNotIn('properties', traits['models'][0])

  def test_save_campaign_manager_model(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      feature_config = temp_path / 'features.json'
      feature_config.write_text(json.dumps({
        'features_dimension': 14,
        'features': [['publisher'], ['campaign', 'ccid']],
      }))

      output_dir = temp_path / 'CTRConfig'
      staging_dir = output_dir / '~20260819.120000'
      staging_dir.mkdir(parents=True)
      (staging_dir / 'traits.json').write_text(json.dumps({
        'status': 'in_progress',
        'train_start': '2026-08-19T12:00:00Z',
      }))
      trainer = CatBoostTrainer(features_config_file=feature_config)
      result_dir = trainer.save_campaign_manager_model(
        ModelStub(),
        output_dir,
        timestamp='20260819.120000',
        staging_dir=staging_dir,
        algorithm_id='test_catboost',
        train_start='2026-08-19T12:00:00Z',
        train_end='2026-08-19T13:30:00Z')

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
      self.assertEqual({
        'features_importance': [],
        'status': 'published',
        'train_start': '2026-08-19T12:00:00Z',
        'train_end': '2026-08-19T13:30:00Z',
      }, traits)
      self.assertFalse(staging_dir.exists())
      self.assertEqual([], list(temp_path.glob('.CTRConfig.*')))
      self.assertEqual([], list(output_dir.glob('~*')))

  def test_saved_campaign_manager_model_drops_unused_features(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      feature_config = temp_path / 'features.json'
      feature_config.write_text(json.dumps({
        'features_dimension': 4,
        'features': [['publisher']],
      }))
      svm_file = temp_path / 'train.libsvm'
      svm_file.write_text('\n'.join(
        str(index % 2) + ' 1:' + str(index % 2) + ' 16:0'
        for index in range(20)) + '\n')

      model = CatBoostClassifier(iterations=2, depth=2, verbose=False)
      model.fit(Pool('libsvm://' + str(svm_file)))
      features = numpy.zeros((1, 16), dtype=numpy.float32)
      features[0, 0] = 1
      expected_prediction = model.predict_proba(features)

      trainer = CatBoostTrainer(features_config_file=feature_config)
      result_dir = trainer.save_campaign_manager_model(
        model,
        temp_path / 'CTRConfig',
        timestamp='20260826.120000')

      saved_model = CatBoostClassifier()
      saved_model.load_model(str(result_dir / 'model.cbm'))
      self.assertLess(len(saved_model._get_float_feature_indices()), 16)
      numpy.testing.assert_array_equal(
        expected_prediction,
        saved_model.predict_proba(features))

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
        feature_dictionary_file=feature_dictionary,
        feature_name_resolver=FeatureNameResolverStub())

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
            {
              'score': 2.5,
              'feature': 'ccg:44,ccid:55',
              'name': 'Account/ccg:44,ccid:55',
            },
            {
              'score': 2.5,
              'feature': 'campaignfreqlog:3',
            },
            {
              'score': 1.25,
              'feature': 'publisher:123',
              'name': 'Account/publisher:123',
            },
          ],
        },
        traits)

  def test_traits_score_is_written_without_exponent(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      traits_file = pathlib.Path(temp_dir) / 'traits.json'
      CatBoostTrainer.write_model_traits_(traits_file, [{
        'score': 8.901938322533171e-05,
        'feature': 'channel:614065',
        'name': 'Account/Channel',
      }])

      text = traits_file.read_text()
      self.assertIn('"score": 0.00008901938322533171', text)
      self.assertNotIn('e-', text)
      self.assertEqual(
        8.901938322533171e-05,
        json.loads(text)['features_importance'][0]['score'])

  def test_model_traits_include_feature_statistics(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      feature_config = temp_path / 'features.json'
      feature_config.write_text(json.dumps({
        'features_dimension': 14,
        'features': [['publisher']],
      }))
      feature_dictionary = temp_path / 'features.csv'
      feature_dictionary.write_text('2,publisher:123\n')
      trainer = CatBoostTrainer(features_config_file=feature_config)

      _, traits = trainer.model_traits_(
        ModelStub([0, 1]),
        feature_dictionary,
        FeatureStatisticsStub())

      self.assertEqual(1, len(traits))
      self.assertEqual(20, traits[0]['yes_share'])
      self.assertEqual('0.2', format(traits[0]['yes_ctr'], 'f'))
      self.assertEqual('0.075', format(traits[0]['no_ctr'], 'f'))

      traits_file = temp_path / 'traits.json'
      trainer.write_model_traits_(
        traits_file,
        traits,
        logloss_history=[
          {'step': 1, 'train': 0.0123, 'test': 0.0203},
          {'step': 2, 'train': 0.0119, 'test': 0.0056},
        ],
        dataset_sizes={
          'train': {'rows': 1000000, 'clicks': 2000},
          'test': {'rows': 300000, 'clicks': 570},
          'final_test': {'rows': 200000, 'clicks': 390},
        },
        ctr_thresholds=[{
          'ctr_goal': 0,
          'impressions': 200000,
          'clicks': 390,
          'actual_ctr': 0.00195,
          'average_predicted_ctr': 0.0021,
        }])
      written_traits = json.loads(traits_file.read_text())
      written_trait = written_traits['features_importance'][0]
      self.assertEqual(20, written_trait['yes_share'])
      self.assertEqual(0.2, written_trait['yes_ctr'])
      self.assertEqual(0.075, written_trait['no_ctr'])
      self.assertEqual([
        {'step': 1, 'train': 0.0123, 'test': 0.0203},
        {'step': 2, 'train': 0.0119, 'test': 0.0056},
      ], written_traits['logloss_history'])
      self.assertEqual({
        'train': {'rows': 1000000, 'clicks': 2000},
        'test': {'rows': 300000, 'clicks': 570},
        'final_test': {'rows': 200000, 'clicks': 390},
      }, written_traits['dataset_sizes'])
      self.assertEqual([{
        'ctr_goal': 0,
        'impressions': 200000,
        'clicks': 390,
        'actual_ctr': 0.00195,
        'average_predicted_ctr': 0.0021,
      }], written_traits['ctr_thresholds'])

  def test_ctr_threshold_statistics_use_strict_greater_than(self):
    statistics = ctr_threshold_statistics(
      numpy.asarray([0, 0.0005, 0.001, 0.0011, 0.03, 0.031]),
      numpy.asarray([1, 0, 1, 1, 0, 1]))

    self.assertEqual(0, statistics[0]['ctr_goal'])
    self.assertEqual(5, statistics[0]['impressions'])
    self.assertEqual(3, statistics[0]['clicks'])
    self.assertAlmostEqual(0.0636, statistics[0]['predicted_ctr_sum'])
    self.assertEqual(0.001, statistics[1]['ctr_goal'])
    self.assertEqual(3, statistics[1]['impressions'])
    self.assertEqual(2, statistics[1]['clicks'])
    self.assertAlmostEqual(0.0621, statistics[1]['predicted_ctr_sum'])
    self.assertEqual(0.03, statistics[30]['ctr_goal'])
    self.assertEqual(1, statistics[30]['impressions'])
    self.assertEqual(1, statistics[30]['clicks'])

  def test_ctr_threshold_statistics_are_aggregated_across_sets(self):
    evaluations = [
      {
        'ctr_thresholds': [{
          'ctr_goal': 0,
          'impressions': 100,
          'clicks': 2,
          'predicted_ctr_sum': 3,
        }],
      },
      {
        'ctr_thresholds': [{
          'ctr_goal': 0,
          'impressions': 300,
          'clicks': 6,
          'predicted_ctr_sum': 5,
        }],
      },
    ]

    self.assertEqual([{
      'ctr_goal': 0,
      'impressions': 400,
      'clicks': 8,
      'actual_ctr': 0.02,
      'average_predicted_ctr': 0.02,
    }], CatBoostTrainer.aggregate_ctr_thresholds_(evaluations))

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
      self.assertEqual([], list(output_dir.glob('~*')))

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

  def test_split_is_deterministic_and_keeps_sets_disjoint(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      source = temp_path / 'source.libsvm'
      source.write_text(''.join(
        '0 1:' + str(index) + '\n'
        for index in range(1, 31)))
      filtered = temp_path / 'filtered.libsvm'
      filtered.write_text(''.join(
        '0 2:' + str(index) + '\n'
        for index in range(1, 31)))
      trainer = CatBoostTrainer(features_dimension=4)

      full_dir = temp_path / 'full'
      full_dir.mkdir()
      full_chunks, full_validations = trainer.split_svm_(
        source, 30, 10, 2, 3, full_dir)
      filtered_dir = temp_path / 'filtered'
      filtered_dir.mkdir()
      filtered_chunks, filtered_validations = trainer.split_svm_(
        filtered, 30, 10, 2, 3, filtered_dir)

      def values(paths):
        result = []
        for path in paths:
          result.append([
            int(line.split()[1].split(':')[1])
            for line in path.read_text().splitlines()
          ])
        return result

      full_chunk_values = values(full_chunks)
      full_validation_values = values(full_validations)
      self.assertEqual([10, 10, 4], [len(value) for value in full_chunk_values])
      self.assertEqual(
        [2, 2, 2],
        [len(value) for value in full_validation_values])
      self.assertEqual(
        full_chunk_values,
        values(filtered_chunks))
      self.assertEqual(
        full_validation_values,
        values(filtered_validations))

      all_values = [
        value
        for group in full_chunk_values + full_validation_values
        for value in group
      ]
      self.assertEqual(list(range(1, 31)), sorted(all_values))
      self.assertEqual(30, len(set(all_values)))

  def test_split_limits_training_rows_without_losing_validations(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      source = temp_path / 'source.libsvm'
      source.write_text(''.join(
        '0 1:' + str(index) + '\n'
        for index in range(1, 31)))
      output_dir = temp_path / 'split'
      output_dir.mkdir()
      trainer = CatBoostTrainer(features_dimension=4)

      chunks, validations = trainer.split_svm_(
        source,
        30,
        10,
        2,
        3,
        output_dir,
        15)

      self.assertEqual(
        [10, 5],
        [len(path.read_text().splitlines()) for path in chunks])
      self.assertEqual(
        [2, 2, 2],
        [len(path.read_text().splitlines()) for path in validations])

  def test_train_chunk_reads_and_removes_metrics_file(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      trainer = CatBoostTrainer(features_dimension=4)
      output_model = temp_path / 'model.cbm'

      def run(command, check):
        self.assertTrue(check)
        self.assertEqual(
          'Logloss',
          command[command.index('--loss-function') + 1])
        metrics_file = pathlib.Path(
          command[command.index('--metrics-file') + 1])
        metrics_file.write_text('{"Logloss": 0.125}\n')

      with unittest.mock.patch(
          'rtbserver_utils.CatBoostTrainer.subprocess.run',
          side_effect=run):
        metrics = trainer.train_chunk_(
          temp_path / 'train.libsvm',
          output_model,
          10)

      self.assertEqual({'Logloss': 0.125}, metrics)
      self.assertFalse(pathlib.Path(str(output_model) + '.metrics.json').exists())

  def test_fit_sequence_stops_after_consecutive_non_improving_steps(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      trainer = CatBoostTrainer(features_dimension=4)
      losses = iter([1.0, 0.9, 0.91, 0.92, 0.93, 0.94, 0.95, 0.8])
      trained_chunks = []

      def train_chunk(chunk, output_model, iterations, initial_model):
        trained_chunks.append(chunk)
        pathlib.Path(output_model).write_text(str(len(trained_chunks)))
        return {'Logloss': len(trained_chunks) / 10}

      def evaluate_model_sets(model_file, validation_files):
        return {'Logloss': next(losses), 'sets': []}

      trainer.train_chunk_ = train_chunk
      trainer.evaluate_model_sets_ = evaluate_model_sets
      chunks = [temp_path / ('chunk-' + str(index)) for index in range(8)]
      best_model, best_logloss, trained_steps, history = trainer.fit_sequence_(
        chunks,
        [temp_path / 'validation'],
        10,
        5,
        temp_path,
        'Test')

      self.assertEqual(7, len(trained_chunks))
      self.assertEqual(7, trained_steps)
      self.assertEqual(0.9, best_logloss)
      self.assertEqual('2', best_model.read_text())
      self.assertEqual(7, len(history))
      self.assertEqual(
        {'step': 1, 'train': 0.1, 'test': 1.0},
        history[0])
      self.assertEqual(
        {'step': 7, 'train': 0.7, 'test': 0.95},
        history[-1])

  def test_aligned_training_uses_previous_correction_as_stable_baseline(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      stable_trainer = CatBoostTrainer(features_dimension=4)
      correction_trainer = CatBoostTrainer(features_dimension=4)
      events = []
      stable_baselines = []

      def correction_train(
          svm_file,
          output_model,
          iterations,
          initial_model=None,
          baseline_file=None,
          merge_model=None,
      ):
        del iterations, initial_model
        events.append(('correction_train', pathlib.Path(svm_file).name))
        self.assertIsNotNone(baseline_file)
        if pathlib.Path(svm_file).name == 'correction-1.libsvm':
          self.assertIsNotNone(merge_model)
        pathlib.Path(output_model).write_text(
          'correction:' + pathlib.Path(svm_file).name)
        return {'Logloss': 0.2}

      def stable_train(
          svm_file,
          output_model,
          iterations,
          initial_model=None,
          baseline_file=None,
          merge_model=None,
      ):
        del iterations, initial_model
        events.append(('stable_train', pathlib.Path(svm_file).name))
        stable_baselines.append(baseline_file)
        if pathlib.Path(svm_file).name == 'stable-1.libsvm':
          self.assertIsNotNone(merge_model)
        pathlib.Path(output_model).write_text(
          'stable:' + pathlib.Path(svm_file).name)
        return {'Logloss': 0.3}

      def correction_predict(
          model_file,
          svm_file,
          output_file,
          baseline_file=None,
      ):
        del model_file, baseline_file
        events.append(('correction_predict', pathlib.Path(svm_file).name))
        pathlib.Path(output_file).write_text('0.1\n')

      def correction_predict_combined(
          model_file,
          svm_file,
          model_output_file,
          combined_output_file,
          baseline_file,
      ):
        del model_file, baseline_file
        events.append(('correction_predict', pathlib.Path(svm_file).name))
        pathlib.Path(model_output_file).write_text('0.1\n')
        pathlib.Path(combined_output_file).write_text('0.3\n')

      def stable_predict(
          model_file,
          svm_file,
          output_file,
          baseline_file=None,
      ):
        del model_file, svm_file, baseline_file
        pathlib.Path(output_file).write_text('0.4\n')

      final_evaluation = {
        'Logloss': 0.1,
        'ctr_thresholds': [{
          'ctr_goal': 0,
          'impressions': 1,
          'clicks': 0,
          'predicted_ctr_sum': 0.1,
        }],
      }
      correction_trainer.train_chunk_ = correction_train
      correction_trainer.predict_raw_ = correction_predict
      correction_trainer.predict_raw_and_combined_ = correction_predict_combined
      correction_trainer.evaluate_model_sets_ = lambda model, inputs: {
        'Logloss': 0.1,
        'sets': [],
      }
      correction_trainer.evaluate_model_with_ctr_thresholds_ = (
        lambda model, svm, baseline: final_evaluation)
      stable_trainer.train_chunk_ = stable_train
      stable_trainer.predict_raw_ = stable_predict
      stable_trainer.evaluate_model_sets_ = lambda model, inputs: {
        'Logloss': 0.1,
        'sets': [],
      }
      stable_trainer.evaluate_model_with_ctr_thresholds_ = (
        lambda model, svm: final_evaluation)

      chunks = []
      for index in range(2):
        stable_svm = temp_path / ('stable-' + str(index) + '.libsvm')
        correction_svm = temp_path / ('correction-' + str(index) + '.libsvm')
        common_baseline = temp_path / ('common-' + str(index) + '.baseline')
        common_baseline.write_text('0.2\n')
        chunks.append((stable_svm, correction_svm, common_baseline))

      class CatBoostClassifierStub:
        def load_model(self, model_file):
          self.model_file = model_file

      aligned_work = temp_path / 'aligned-work'
      aligned_work.mkdir()
      with unittest.mock.patch(
          'rtbserver_utils.CatBoostTrainer.CatBoostClassifier',
          CatBoostClassifierStub):
        result = stable_trainer.train_aligned_from_chunks_(
          chunks,
          correction_trainer,
          [(temp_path / 'correction-validation', temp_path / 'common-validation')],
          [temp_path / 'stable-validation'],
          [(temp_path / 'correction-final', temp_path / 'common-final')],
          [temp_path / 'stable-final'],
          fit_iterations=1,
          patience=2,
          work_dir=aligned_work,
          fit_steps=2)

      self.assertEqual([
        ('correction_train', 'correction-0.libsvm'),
        ('stable_train', 'stable-0.libsvm'),
        ('correction_predict', 'correction-1.libsvm'),
        ('correction_train', 'correction-1.libsvm'),
        ('stable_train', 'stable-1.libsvm'),
      ], events)
      self.assertIsNone(stable_baselines[0])
      self.assertIsNotNone(stable_baselines[1])
      self.assertIn('campaign_correction', result)
      self.assertIn('stable_common', result)

  def test_residual_training_merges_chunks_and_optimizes_weight(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      work_dir = temp_path / 'work'
      work_dir.mkdir()
      trainer = CatBoostTrainer(features_dimension=4)
      baselines = []
      merge_models = []

      def train_chunk(
          svm_file,
          output_model,
          iterations,
          initial_model=None,
          baseline_file=None,
          merge_model=None,
      ):
        del svm_file, iterations, initial_model
        baselines.append(pathlib.Path(baseline_file).name)
        merge_models.append(merge_model)
        pathlib.Path(output_model).write_text('model')
        return {'Logloss': 0.2}

      def predict_raw(model_file, svm_file, output_file, baseline_file=None):
        del model_file, svm_file, baseline_file
        pathlib.Path(output_file).write_text('0.3\n')

      trainer.train_chunk_ = train_chunk
      trainer.predict_raw_ = predict_raw
      trainer.evaluate_model_sets_ = lambda model, inputs: {
        'Logloss': 0.1,
        'sets': [],
      }
      trainer.optimize_prediction_weight_ = lambda model, inputs: {
        'weight': 0.65,
        'base_logloss': 0.12,
        'combined_logloss': 0.1,
      }
      trainer.evaluate_model_with_ctr_thresholds_ = (
        lambda model, svm, baseline, weight: {
          'Logloss': 0.1,
          'ctr_thresholds': [],
        })
      chunks = []
      for index in range(2):
        baseline = temp_path / ('stable-' + str(index) + '.baseline')
        baseline.write_text('0.2\n')
        chunks.append((temp_path / ('chunk-' + str(index)), baseline))

      class CatBoostClassifierStub:
        def load_model(self, model_file):
          self.model_file = model_file

      with unittest.mock.patch(
          'rtbserver_utils.CatBoostTrainer.CatBoostClassifier',
          CatBoostClassifierStub):
        result = trainer.train_residual_from_chunks_(
          chunks,
          [(temp_path / 'validation', temp_path / 'validation-baseline')],
          [(temp_path / 'final', temp_path / 'final-baseline')],
          fit_iterations=1,
          patience=2,
          work_dir=work_dir,
          fit_steps=2)

      self.assertEqual('stable-0.baseline', baselines[0])
      self.assertEqual('combined-002.baseline', baselines[1])
      self.assertIsNone(merge_models[0])
      self.assertIsNotNone(merge_models[1])
      self.assertEqual(0.65, result['weight'])
      self.assertEqual(0.12, result['base_logloss'])
      self.assertEqual(0.1, result['combined_logloss'])

  def test_feature_selection_unions_independent_chunk_models(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      source = temp_path / 'source.libsvm'
      source.write_text('0 1:1\n')
      chunks = [
        temp_path / ('chunk-' + str(index))
        for index in range(7)
      ]
      validations = [
        temp_path / ('validation-' + str(index))
        for index in range(3)
      ]
      trainer = CatBoostTrainer(features_dimension=4)
      trained_chunks = []
      initial_models = []

      def split_svm(*args):
        self.assertEqual(30, args[-1])
        return chunks[:3], validations

      def train_chunk(
          svm_file,
          output_model,
          iterations,
          initial_model=None,
          baseline_file=None,
          merge_model=None,
      ):
        del iterations, baseline_file, merge_model
        trained_chunks.append(svm_file)
        initial_models.append(initial_model)
        pathlib.Path(output_model).write_text(str(svm_file))
        return {'Logloss': 0.2}

      trainer.split_svm_ = split_svm
      trainer.train_chunk_ = train_chunk
      trainer.evaluate_model_sets_ = lambda model_file, validation_paths: {
        'Logloss': 0.1,
        'sets': validation_paths,
      }
      trainer.model_feature_indexes_ = lambda model_file: {
        int(pathlib.Path(model_file).stem.rsplit('-', 1)[1])
      }

      indexes = trainer.select_feature_indexes(
        source,
        100,
        10,
        1,
        selection_validation_sets=1,
        training_validation_sets=1,
        final_test_sets=1,
        fit_steps=3)

      self.assertEqual(chunks[:3], trained_chunks)
      self.assertEqual([None, None, None], initial_models)
      self.assertEqual({1, 2, 3}, indexes)

  def test_residual_feature_selection_uses_independent_baselines(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      trainer = CatBoostTrainer(features_dimension=4)
      baselines = []

      def train_chunk(
          svm_file,
          output_model,
          iterations,
          initial_model=None,
          baseline_file=None,
          merge_model=None,
      ):
        del svm_file, iterations, merge_model
        self.assertIsNone(initial_model)
        baselines.append(baseline_file)
        pathlib.Path(output_model).write_text('model')
        return {'Logloss': 0.2}

      trainer.train_chunk_ = train_chunk
      trainer.evaluate_model_sets_ = lambda model_file, validation_paths: {
        'Logloss': 0.1,
        'sets': validation_paths,
      }
      trainer.model_feature_indexes_ = lambda model_file: {
        int(pathlib.Path(model_file).stem.rsplit('-', 1)[1])
      }
      chunks = [
        (temp_path / 'chunk-1', temp_path / 'baseline-1'),
        (temp_path / 'chunk-2', temp_path / 'baseline-2'),
      ]
      validation = [(temp_path / 'validation', temp_path / 'baseline')]

      indexes = trainer.select_feature_indexes_from_chunks_(
        chunks,
        validation,
        fit_iterations=1,
        work_dir=temp_path,
        fit_steps=2)

      self.assertEqual([chunk[1] for chunk in chunks], baselines)
      self.assertEqual({1, 2}, indexes)

  def test_main_training_does_not_repeat_chunks(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      source = temp_path / 'source.libsvm'
      source.write_text('0 1:1\n')
      chunks = [
        temp_path / ('chunk-' + str(index))
        for index in range(3)
      ]
      validations = [
        temp_path / ('validation-' + str(index))
        for index in range(3)
      ]
      trainer = CatBoostTrainer(features_dimension=4)
      model_sequences = []

      trainer.split_svm_ = lambda *args: (chunks, validations)

      def fit_sequence(
          model_chunks,
          validation_paths,
          fit_iterations,
          patience,
          model_dir,
          description,
          fit_steps,
      ):
        del validation_paths, fit_iterations, patience, description
        self.assertEqual(8, fit_steps)
        model_sequences.append(list(model_chunks))
        model_file = model_dir / 'best-model.cbm'
        model_file.write_text('model')
        return model_file, 0.1, len(model_chunks), []

      class CatBoostClassifierStub:
        def load_model(self, model_file):
          self.model_file = model_file

      trainer.fit_sequence_ = fit_sequence
      trainer.evaluate_model_with_ctr_thresholds_ = lambda model_file, svm_file: {
        'Logloss': 0.1,
        'ctr_thresholds': [],
      }
      with unittest.mock.patch(
          'rtbserver_utils.CatBoostTrainer.CatBoostClassifier',
          CatBoostClassifierStub):
        trainer.train_filtered_by_chunks(
          source,
          100,
          10,
          1,
          selection_validation_sets=1,
          training_validation_sets=1,
          final_test_sets=1,
          fit_steps=8)

      self.assertEqual([chunks], model_sequences)


if __name__ == '__main__':
  unittest.main()
