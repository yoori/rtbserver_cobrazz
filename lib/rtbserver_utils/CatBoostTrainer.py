import argparse
import csv
import datetime
import decimal
import itertools
import json
import math
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
from catboost import CatBoostClassifier, Pool


class CatBoostTrainer(object):
  DICTIONARY_FEATURE_NAMES = {
    'campaignfreq': 'campaign_freq',
    'campaignfreqlog': 'campaign_freq_log',
    'ccg': 'group',
    'channel': 'userch',
    'contcat': 'crcatcont',
    'geochannel': 'geoch',
    'size': 'sizeid',
    'tagviewability': 'viewability',
    'tagvisibility': 'visibility',
    'viscat': 'crcatvis',
  }

  features_size: int = None

  def __init__(
      self,
      features_dimension=None,
      features_config_file=None,
      train_dir=None,
  ):
    self.features_config_file = features_config_file
    self.train_dir = train_dir
    self.features = None
    if features_config_file is not None:
      config_dimension, self.features = self.read_features_config_(
        features_config_file)
      if (features_dimension is not None and
          features_dimension != config_dimension):
        raise ValueError(
          'Feature dimension mismatch: argument=' +
          str(features_dimension) + ', config=' + str(config_dimension))
      features_dimension = config_dimension

    if features_dimension is None:
      features_dimension = 24
    self.features_size = 1 << features_dimension

  def train_on_pools(
      self,
      train_pool: Pool,
      test_pool: Pool = None,
      iterations=100,
      initial_model=None,
  ):
    # Step 2: Initialize and train the CatBoost model
    model = CatBoostClassifier(
      iterations=iterations,
      learning_rate=0.1, # Step size shrinkage to prevent overfitting
      depth=6,          # Depth of the trees
      loss_function='Logloss', # Loss function for binary classification
      verbose=0,        # Suppress training output
      train_dir=self.train_dir,
      use_best_model=False,
    )

    print("To fit")
    model.fit(
      train_pool,
      eval_set=test_pool,
      init_model=initial_model,
      verbose=True)
    return model

  def select_feature_indexes(
      self,
      svm_file,
      total_rows,
      chunk_rows,
      validation_rows,
      selection_validation_sets=3,
      training_validation_sets=3,
      final_test_sets=3,
      fit_steps=10,
      fit_iterations=10,
      patience=3,
  ):
    validation_sets = (
      selection_validation_sets +
      training_validation_sets +
      final_test_sets)
    with tempfile.TemporaryDirectory(
        dir=str(pathlib.Path(svm_file).resolve().parent),
        prefix='catboost-feature-selection.') as temp_dir:
      chunks, validations = self.split_svm_(
        svm_file,
        total_rows,
        chunk_rows,
        validation_rows,
        validation_sets,
        pathlib.Path(temp_dir),
        chunk_rows * fit_steps)
      return self.select_feature_indexes_from_chunks_(
        chunks,
        validations[:selection_validation_sets],
        fit_iterations,
        patience,
        pathlib.Path(temp_dir),
        fit_steps)

  def select_feature_indexes_from_chunks(
      self,
      chunks,
      validation_paths,
      fit_iterations=10,
      patience=3,
      work_dir=None,
      fit_steps=None,
  ):
    temp_parent = None if work_dir is None else str(pathlib.Path(work_dir))
    with tempfile.TemporaryDirectory(
        dir=temp_parent,
        prefix='catboost-feature-selection.') as temp_dir:
      return self.select_feature_indexes_from_chunks_(
        chunks,
        validation_paths,
        fit_iterations,
        patience,
        pathlib.Path(temp_dir),
        fit_steps)

  def select_feature_indexes_from_chunks_(
      self,
      chunks,
      validation_paths,
      fit_iterations,
      patience,
      work_dir,
      fit_steps,
  ):
    model_dir = work_dir / 'selection-model'
    model_dir.mkdir()
    _, best_logloss, trained_steps = self.fit_sequence_(
      chunks,
      validation_paths,
      fit_iterations,
      patience,
      model_dir,
      'Feature selection',
      fit_steps)
    selected_indexes = self.model_feature_indexes_(model_dir / 'model.cbm')
    print(
      'Feature selection: chunks=' + str(trained_steps) +
      ', Logloss=' + str(best_logloss) +
      ', selected_indexes=' + str(len(selected_indexes)),
      flush=True)

    if not selected_indexes:
      raise RuntimeError('Feature selection produced no feature indexes')
    return selected_indexes

  def train_filtered_by_chunks(
      self,
      svm_file,
      total_rows,
      chunk_rows,
      validation_rows,
      selection_validation_sets=3,
      training_validation_sets=3,
      final_test_sets=3,
      fit_iterations=10,
      patience=5,
      fit_steps=30,
  ):
    validation_sets = (
      selection_validation_sets +
      training_validation_sets +
      final_test_sets)
    with tempfile.TemporaryDirectory(
        dir=str(pathlib.Path(svm_file).resolve().parent),
        prefix='catboost-main-training.') as temp_dir:
      chunks, validations = self.split_svm_(
        svm_file,
        total_rows,
        chunk_rows,
        validation_rows,
        validation_sets,
        pathlib.Path(temp_dir),
        chunk_rows * fit_steps)
      training_validation_begin = selection_validation_sets
      training_validation_end = (
        training_validation_begin + training_validation_sets)
      return self.train_from_chunks_(
        chunks,
        validations[training_validation_begin:training_validation_end],
        validations[training_validation_end:],
        fit_iterations,
        patience,
        pathlib.Path(temp_dir),
        fit_steps)

  def train_from_chunks(
      self,
      chunks,
      validation_paths,
      final_test_paths,
      fit_iterations=10,
      patience=5,
      work_dir=None,
      fit_steps=None,
  ):
    temp_parent = None if work_dir is None else str(pathlib.Path(work_dir))
    with tempfile.TemporaryDirectory(
        dir=temp_parent,
        prefix='catboost-main-training.') as temp_dir:
      return self.train_from_chunks_(
        chunks,
        validation_paths,
        final_test_paths,
        fit_iterations,
        patience,
        pathlib.Path(temp_dir),
        fit_steps)

  def train_from_chunks_(
      self,
      chunks,
      validation_paths,
      final_test_paths,
      fit_iterations,
      patience,
      work_dir,
      fit_steps,
  ):
    model_dir = work_dir / 'main-model'
    model_dir.mkdir()
    best_model, best_logloss, trained_steps = self.fit_sequence_(
      chunks,
      validation_paths,
      fit_iterations,
      patience,
      model_dir,
      'Main training',
      fit_steps)
    final_metrics = [
      self.evaluate_model_(best_model, final_test)
      for final_test in final_test_paths
    ]
    print(
      'Best main validation Logloss: ' + str(best_logloss) +
      ', trained chunks: ' + str(trained_steps),
      flush=True)
    print(
      'Final test metrics: ' +
      json.dumps(final_metrics, sort_keys=True),
      flush=True)

    model = CatBoostClassifier()
    model.load_model(str(best_model))
    return model

  def split_svm_(
      self,
      svm_file,
      total_rows,
      chunk_rows,
      validation_rows,
      validation_sets,
      output_dir,
      max_train_rows=None,
  ):
    if total_rows <= 1:
      raise ValueError('total_rows must be greater than 1')
    if chunk_rows <= 0:
      raise ValueError('chunk_rows must be positive')
    if validation_rows <= 0:
      raise ValueError('validation_rows must be positive')
    if validation_sets <= 0:
      raise ValueError('validation_sets must be positive')

    holdout_rows = validation_rows * validation_sets
    if holdout_rows >= total_rows:
      raise ValueError(
        'Validation and final test rows must be less than total_rows')
    train_rows = total_rows - holdout_rows
    if max_train_rows is not None:
      if max_train_rows <= 0:
        raise ValueError('max_train_rows must be positive')
      train_rows = min(train_rows, max_train_rows)
    chunk_count = math.ceil(train_rows / chunk_rows)
    chunk_paths = [
      output_dir / ('train-' + str(index) + '.libsvm')
      for index in range(chunk_count)
    ]
    validation_paths = [
      output_dir / ('validation-' + str(index) + '.libsvm')
      for index in range(validation_sets)
    ]
    chunk_files = [path.open('w') for path in chunk_paths]
    validation_files = [path.open('w') for path in validation_paths]
    chunk_sizes = [0] * chunk_count
    validation_sizes = [0] * validation_sets

    try:
      holdout_index = 0
      train_index = 0
      actual_rows = 0
      with pathlib.Path(svm_file).resolve().open() as input_file:
        for row_index, line in enumerate(input_file):
          actual_rows = row_index + 1
          next_holdout_count = (
            (row_index + 1) * holdout_rows // total_rows)
          if next_holdout_count > holdout_index:
            validation_index = holdout_index % validation_sets
            self.write_svm_line_(
              validation_files[validation_index],
              line,
              validation_sizes[validation_index] == 0)
            validation_sizes[validation_index] += 1
            holdout_index += 1
          else:
            if train_index >= train_rows:
              continue
            chunk_index = min(train_index // chunk_rows, chunk_count - 1)
            self.write_svm_line_(
              chunk_files[chunk_index],
              line,
              chunk_sizes[chunk_index] == 0)
            chunk_sizes[chunk_index] += 1
            train_index += 1
    finally:
      for file in chunk_files:
        file.close()
      for file in validation_files:
        file.close()

    if actual_rows != total_rows:
      raise ValueError('Actual SVM row count does not match total_rows')
    if any(size != validation_rows for size in validation_sizes):
      raise RuntimeError('Failed to create validation sets of requested size')
    return chunk_paths, validation_paths

  def fit_sequence_(
      self,
      chunk_paths,
      validation_paths,
      fit_iterations,
      patience,
      model_dir,
      description,
      fit_steps=None,
  ):
    if not validation_paths:
      raise ValueError('At least one validation set is required')
    if fit_iterations <= 0:
      raise ValueError('fit_iterations must be positive')
    if patience <= 0:
      raise ValueError('patience must be positive')
    if fit_steps is not None and fit_steps <= 0:
      raise ValueError('fit_steps must be positive')

    model_path = model_dir / 'model.cbm'
    next_model_path = model_dir / 'next-model.cbm'
    best_model_path = model_dir / 'best-model.cbm'
    best_logloss = None
    non_improving_steps = 0
    trained_steps = 0

    chunk_iterator = iter(chunk_paths)
    try:
      chunks_to_fit = chunk_iterator
      if fit_steps is not None:
        chunks_to_fit = itertools.islice(chunk_iterator, fit_steps)
      for step, chunk_path in enumerate(chunks_to_fit, 1):
        total = '' if fit_steps is None else '/' + str(fit_steps)
        print(
          description + ': fit ' + str(step) + total,
          flush=True)
        self.train_chunk_(
          chunk_path,
          next_model_path,
          fit_iterations,
          model_path if model_path.exists() else None)
        os.replace(next_model_path, model_path)
        trained_steps = step
        metrics = self.evaluate_model_sets_(model_path, validation_paths)
        logloss = metrics['Logloss']
        print(
          description + ': validation=' +
          json.dumps(metrics, sort_keys=True),
          flush=True)

        if best_logloss is None or logloss < best_logloss:
          best_logloss = logloss
          non_improving_steps = 0
          shutil.copyfile(model_path, best_model_path)
        else:
          non_improving_steps += 1
          if non_improving_steps >= patience:
            print(
              description + ': early stop after ' + str(step) + ' fits',
              flush=True)
            break
    finally:
      close = getattr(chunk_iterator, 'close', None)
      if close is not None:
        close()

    if trained_steps == 0:
      raise ValueError('At least one training chunk is required')
    return best_model_path, best_logloss, trained_steps

  def evaluate_model_sets_(self, model_file, svm_files):
    metrics = [
      self.evaluate_model_(model_file, svm_file)
      for svm_file in svm_files
    ]
    return {
      'Logloss': sum(metric['Logloss'] for metric in metrics) / len(metrics),
      'sets': metrics,
    }

  @staticmethod
  def model_feature_indexes_(model_file):
    result = subprocess.run(
      [
        sys.executable,
        '-m',
        'rtbserver_utils.CatBoostModelFeatures',
        '--model-file',
        str(model_file),
      ],
      check=True,
      capture_output=True,
      text=True)
    return set(json.loads(result.stdout))

  def write_svm_line_(self, output_file, line, add_size_sentinel):
    feature_prefix = ' ' + str(self.features_size) + ':'
    if add_size_sentinel and feature_prefix not in line:
      output_file.write(
        line.rstrip('\r\n') + feature_prefix + '0\n')
    else:
      output_file.write(line)

  def train_chunk_(
      self,
      svm_file,
      output_model,
      iterations,
      initial_model=None,
  ):
    command = [
      sys.executable,
      '-m',
      'rtbserver_utils.CatBoostTrainChunk',
      '--svm-file',
      str(svm_file),
      '--output-model',
      str(output_model),
      '--iterations',
      str(iterations),
    ]
    if initial_model is not None:
      command.extend(['--initial-model', str(initial_model)])
    if self.train_dir is not None:
      command.extend(['--train-dir', str(self.train_dir)])
    subprocess.run(command, check=True)

  @staticmethod
  def evaluate_model_(model_file, svm_file):
    result = subprocess.run(
      [
        sys.executable,
        '-m',
        'rtbserver_utils.CatBoostModelEvaluator',
        '--model-file',
        str(model_file),
        '--svm-file',
        str(svm_file),
      ],
      check=True,
      capture_output=True,
      text=True)
    return json.loads(result.stdout)

  def train(self, train_svm_file, test_svm_file=None):
    train_pool = self.load_pool_(train_svm_file)
    print("Train set: (" + str(train_pool.num_row()) + ", " +
          str(train_pool.num_col()) + ")")
    test_pool = None
    if test_svm_file is not None:
      test_pool = self.load_pool_(test_svm_file)
      print("Test set: (" + str(test_pool.num_row()) + ", " +
            str(test_pool.num_col()) + ")")
    return self.train_on_pools(train_pool, test_pool)

  @staticmethod
  def load_pool_(svm_file):
    svm_path = pathlib.Path(svm_file).resolve()
    return Pool('libsvm://' + str(svm_path))

  def save_campaign_manager_model(
      self,
      model,
      output_dir,
      timestamp=None,
      algorithm_id='catboost',
      feature_dictionary_file=None,
      feature_name_resolver=None,
  ):
    if self.features is None:
      raise ValueError(
        'features_config_file is required to generate a CampaignManager model')

    if timestamp is None:
      timestamp = datetime.datetime.now(datetime.timezone.utc).strftime(
        '%Y%m%d.%H%M%S')

    output_dir = pathlib.Path(output_dir)
    result_dir = output_dir / timestamp
    if result_dir.exists():
      raise FileExistsError("Model directory already exists: '" +
                            str(result_dir) + "'")

    output_dir.mkdir(parents=True, exist_ok=True)
    staging_dir = pathlib.Path(tempfile.mkdtemp(
      prefix='~' + timestamp + '.',
      dir=str(output_dir)))
    try:
      model.save_model(str(staging_dir / 'model.cbm'))
      features, features_importance = self.model_traits_(
        model,
        feature_dictionary_file)
      if feature_name_resolver is not None:
        feature_names = feature_name_resolver.resolve(
          [item['feature'] for item in features_importance])
        for item in features_importance:
          name = feature_names.get(item['feature'])
          if name is not None:
            item['name'] = name
      self.write_campaign_manager_config_(
        staging_dir / 'config.json',
        algorithm_id,
        self.features_size,
        features)
      self.write_model_traits_(
        staging_dir / 'traits.json',
        features_importance)
      os.rename(staging_dir, result_dir)
    except Exception:
      shutil.rmtree(staging_dir, ignore_errors=True)
      raise

    return result_dir

  def model_traits_(self, model, feature_dictionary_file):
    feature_importance = [
      (index, float(score))
      for index, score in enumerate(model.get_feature_importance())
      if score > 0
    ]
    feature_importance.sort(key=lambda item: (-item[1], item[0]))

    if feature_dictionary_file is None:
      return self.features, []

    dictionary = self.read_feature_dictionary_(feature_dictionary_file)
    configured_features = {
      frozenset(feature): feature
      for feature in self.features
    }
    used_feature_signatures = set()
    traits = []

    for index, score in feature_importance:
      feature_names = dictionary.get(index + 1)
      if not feature_names:
        raise ValueError(
          'Feature dictionary does not contain used SVM index ' +
          str(index + 1))

      for feature_name in feature_names:
        signature = self.feature_signature_(feature_name)
        if signature not in configured_features:
          raise ValueError(
            "Feature '" + feature_name +
            "' from dictionary is absent in feature configuration")
        used_feature_signatures.add(signature)
        traits.append({
          'score': score,
          'feature': feature_name,
        })

    features = [
      feature
      for feature in self.features
      if frozenset(feature) in used_feature_signatures
    ]
    return features, traits

  @classmethod
  def feature_signature_(cls, feature_name):
    dictionary_names = re.findall(r'(?:^|,)([a-z_]+):', feature_name)
    if not dictionary_names:
      raise ValueError(
        "Can't extract feature domains from dictionary value '" +
        feature_name + "'")
    return frozenset(
      cls.DICTIONARY_FEATURE_NAMES.get(name, name)
      for name in dictionary_names)

  @staticmethod
  def read_feature_dictionary_(feature_dictionary_file):
    dictionary = {}
    with pathlib.Path(feature_dictionary_file).open(newline='') as input_file:
      for line_number, row in enumerate(csv.reader(input_file), 1):
        if not row:
          continue
        if len(row) != 2:
          raise ValueError(
            'Invalid feature dictionary row ' + str(line_number) + ': ' +
            repr(row))
        try:
          index = int(row[0])
        except ValueError as error:
          raise ValueError(
            'Invalid feature dictionary index at row ' +
            str(line_number) + ": '" + row[0] + "'") from error
        dictionary.setdefault(index, [])
        if row[1] not in dictionary[index]:
          dictionary[index].append(row[1])
    return dictionary

  @staticmethod
  def read_features_config_(features_config_file):
    with pathlib.Path(features_config_file).open() as input_file:
      config = json.load(input_file)

    dimension = config.get('features_dimension')
    if (
        not isinstance(dimension, int) or
        isinstance(dimension, bool) or
        dimension <= 0):
      raise ValueError(
        "Invalid features_dimension in feature config '" +
        str(features_config_file) + "'")

    features = config.get('features')
    if (
        not isinstance(features, list) or
        not features or
        any(
          not isinstance(feature, list) or
          not feature or
          any(not isinstance(name, str) or not name for name in feature)
          for feature in features)):
      raise ValueError(
        "Invalid features in feature config '" +
        str(features_config_file) + "'")

    return dimension, features

  @staticmethod
  def write_campaign_manager_config_(
      config_file,
      algorithm_id,
      features_size,
      features,
  ):
    config = {
      'version': 2,
      'default_weight': 0,
      'algorithms': [
        {
          'id': algorithm_id,
          'weight': 1,
          'threshold': 0,
          'models': [
            {
              'method': 'catboost',
              'features_size': features_size,
              'weight': 1.0,
              'features': features,
              'file': 'model.cbm',
            },
          ],
        },
      ],
    }
    with config_file.open('w') as output:
      json.dump(config, output, indent=2)
      output.write('\n')

  @staticmethod
  def write_model_traits_(traits_file, features_importance):
    with traits_file.open('w') as output:
      output.write('{\n  "features_importance": [')
      if not features_importance:
        output.write(']\n}\n')
        return
      for index, item in enumerate(features_importance):
        output.write(',\n' if index else '\n')
        output.write('    {\n')
        output.write(
          '      "score": ' +
          format(decimal.Decimal(repr(item['score'])), 'f') + ',\n')
        output.write(
          '      "feature": ' +
          json.dumps(item['feature'], ensure_ascii=False))
        if 'name' in item:
          output.write(',\n      "name": ' + json.dumps(
            item['name'], ensure_ascii=False))
        output.write('\n    }')
      output.write('\n  ]\n}\n')


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Train catboost model.')
  parser.add_argument('--train-file', help='Train libsvm.')
  parser.add_argument('--test-file', help='Test libsvm.')
  parser.add_argument('--features-config-file', required=True)
  parser.add_argument('--feature-dictionary-file')
  parser.add_argument('--algorithm-id', default='catboost')
  parser.add_argument('--output-dir', required=True)
  args = parser.parse_args()

  trainer = CatBoostTrainer(features_config_file=args.features_config_file)
  model = trainer.train(args.train_file, args.test_file)
  trainer.save_campaign_manager_model(
    model,
    args.output_dir,
    algorithm_id=args.algorithm_id,
    feature_dictionary_file=args.feature_dictionary_file)
