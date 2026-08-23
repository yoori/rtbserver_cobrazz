import argparse
import json
import pathlib

import numpy
from catboost import CatBoostClassifier, Pool


def evaluate_model(model_file, svm_file):
  model = CatBoostClassifier()
  model.load_model(str(pathlib.Path(model_file).resolve()))
  validation_pool = Pool(
    'libsvm://' + str(pathlib.Path(svm_file).resolve()))
  raw_predictions = numpy.asarray(
    model.predict(
      validation_pool,
      prediction_type='RawFormulaVal'),
    dtype=numpy.float64)
  labels = numpy.asarray(
    validation_pool.get_label(),
    dtype=numpy.float64)
  logloss = numpy.mean(
    numpy.logaddexp(0, raw_predictions) - labels * raw_predictions)
  return {
    'Logloss': float(logloss),
  }


def main():
  parser = argparse.ArgumentParser(description='Evaluate a CatBoost model.')
  parser.add_argument('--model-file', required=True)
  parser.add_argument('--svm-file', required=True)
  args = parser.parse_args()
  print(json.dumps(evaluate_model(args.model_file, args.svm_file)))


if __name__ == '__main__':
  main()
