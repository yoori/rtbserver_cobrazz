#!/usr/bin/env python3.12

import json
import pathlib
import sys
import tempfile
import unittest
import unittest.mock


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


class FeatureNameResolverStub:
  def resolve(self, features):
    return {
      feature: 'Account/' + feature
      for feature in features
      if feature != 'campaignfreqlog:3'
    }


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
      self.assertEqual([], list(output_dir.glob('~*')))

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

  def test_fit_sequence_stops_after_consecutive_non_improving_steps(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      trainer = CatBoostTrainer(features_dimension=4)
      losses = iter([1.0, 0.9, 0.91, 0.92, 0.93, 0.94, 0.95, 0.8])
      trained_chunks = []

      def train_chunk(chunk, output_model, iterations, initial_model):
        trained_chunks.append(chunk)
        pathlib.Path(output_model).write_text(str(len(trained_chunks)))

      def evaluate_model_sets(model_file, validation_files):
        return {'Logloss': next(losses), 'sets': []}

      trainer.train_chunk_ = train_chunk
      trainer.evaluate_model_sets_ = evaluate_model_sets
      chunks = [temp_path / ('chunk-' + str(index)) for index in range(8)]
      best_model, best_logloss, trained_steps = trainer.fit_sequence_(
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

  def test_feature_selection_changes_chunk_on_every_fit(self):
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
      model_sequences = []

      def split_svm(*args):
        self.assertEqual(30, args[-1])
        return chunks[:3], validations

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
        self.assertEqual(3, fit_steps)
        steps = 2
        model_sequences.append(list(model_chunks))
        trained_chunks.extend(model_chunks[:steps])
        model_file = model_dir / 'model.cbm'
        model_file.write_text('model')
        return model_file, 0.1, steps

      trainer.split_svm_ = split_svm
      trainer.fit_sequence_ = fit_sequence
      trainer.model_feature_indexes_ = lambda model_file: {1}

      indexes = trainer.select_feature_indexes(
        source,
        100,
        10,
        1,
        selection_validation_sets=1,
        training_validation_sets=1,
        final_test_sets=1,
        fit_steps=3)

      self.assertEqual(chunks[:2], trained_chunks)
      self.assertEqual([chunks[:3]], model_sequences)
      self.assertEqual({1}, indexes)

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
        return model_file, 0.1, len(model_chunks)

      class CatBoostClassifierStub:
        def load_model(self, model_file):
          self.model_file = model_file

      trainer.fit_sequence_ = fit_sequence
      trainer.evaluate_model_ = lambda model_file, svm_file: {
        'Logloss': 0.1
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
