import argparse
import json
import pathlib
import tempfile

from catboost import CatBoostClassifier


def used_feature_indexes(model_file):
  model = CatBoostClassifier()
  model.load_model(str(pathlib.Path(model_file).resolve()))

  with tempfile.NamedTemporaryFile(suffix='.json') as json_file:
    model.save_model(json_file.name, format='json')
    with open(json_file.name, 'r') as input_file:
      model_json = json.load(input_file)

  feature_indexes = {
    feature['feature_index']: feature['flat_feature_index']
    for feature in model_json.get('features_info', {}).get(
      'float_features', [])
  }
  result = set()
  for tree in model_json.get('oblivious_trees', []):
    for split in tree.get('splits', []):
      if split.get('split_type') != 'FloatFeature':
        continue
      feature_index = split['float_feature_index']
      try:
        flat_feature_index = feature_indexes[feature_index]
      except KeyError as error:
        raise ValueError(
          'CatBoost model does not describe float feature index ' +
          str(feature_index)) from error
      result.add(flat_feature_index + 1)
  return sorted(result)


def main():
  parser = argparse.ArgumentParser(
    description='Extract used 1-based LibSVM indexes from a CatBoost model.')
  parser.add_argument('--model-file', required=True)
  args = parser.parse_args()
  print(json.dumps(used_feature_indexes(args.model_file)))


if __name__ == '__main__':
  main()
