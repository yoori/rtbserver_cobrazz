import argparse
import csv
import datetime
import json
import os
import pathlib
import re
import shutil
import sklearn.model_selection
import tempfile
from catboost import CatBoostClassifier, Pool

from rtbserver_utils.CatBoostFeatures import load_catboost_svm


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

  def __init__(self, features_dimension=None, features_config_file=None):
    self.features_config_file = features_config_file
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

  def train_on_pools(self, train_pool: Pool, test_pool: Pool):
    # Step 2: Initialize and train the CatBoost model
    model = CatBoostClassifier(
      iterations=100,  # Number of boosting iterations (trees)
      learning_rate=0.1, # Step size shrinkage to prevent overfitting
      depth=6,          # Depth of the trees
      loss_function='Logloss', # Loss function for binary classification
      verbose=0         # Suppress training output
    )

    print("To fit")
    model.fit(train_pool, verbose=True)
    return model

  def split_and_train(self, svm_file):
    full_svm, full_label = load_catboost_svm(svm_file, self.features_size)
    train_data_sm, test_data_sm, train_label_sm, test_label_sm = (
      sklearn.model_selection.train_test_split(
        full_svm,
        full_label,
        test_size=0.2,
        random_state=42))
    return self.train_prepared_(
      train_data_sm,
      test_data_sm,
      train_label_sm,
      test_label_sm)

  def train(self, train_svm_file, test_svm_file):
    train_data_sm, train_label_sm = load_catboost_svm(
      train_svm_file,
      self.features_size)
    test_data_sm, test_label_sm = load_catboost_svm(
      test_svm_file,
      self.features_size)
    return self.train_prepared_(
      train_data_sm,
      test_data_sm,
      train_label_sm,
      test_label_sm)

  def train_prepared_(
      self,
      train_data,
      test_data,
      train_label,
      test_label,
  ):
    print("Train set: " + str(train_data.shape))
    print("Test set: " + str(test_data.shape))

    categorical_features_indices = []
    train_pool = Pool(train_data, train_label, cat_features=categorical_features_indices)
    test_pool = Pool(test_data, test_label, cat_features=categorical_features_indices)
    return self.train_on_pools(train_pool, test_pool)

  def save_campaign_manager_model(
      self,
      model,
      output_dir,
      timestamp=None,
      algorithm_id='catboost',
      feature_dictionary_file=None,
  ):
    if self.features is None:
      raise ValueError(
        'features_config_file is required to generate a CampaignManager model')

    if timestamp is None:
      timestamp = datetime.datetime.now(datetime.timezone.utc).strftime(
        '%Y%m%d.%H%M%S')

    output_dir = pathlib.Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    result_dir = output_dir / timestamp
    if result_dir.exists():
      raise FileExistsError("Model directory already exists: '" +
                            str(result_dir) + "'")

    staging_dir = pathlib.Path(tempfile.mkdtemp(
      prefix='.' + output_dir.name + '.' + timestamp + '.',
      dir=str(output_dir.parent)))
    try:
      model.save_model(str(staging_dir / 'model.cbm'))
      features, features_importance = self.model_traits_(
        model,
        feature_dictionary_file)
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
        traits.append({score: feature_name})

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
      json.dump(
        {'features_importance': features_importance},
        output,
        indent=2)
      output.write('\n')


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
