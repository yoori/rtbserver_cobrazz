import argparse
import json
import pathlib

import numpy
from catboost import CatBoostClassifier, Pool, sum_models


def train_chunk(
    svm_file,
    output_model,
    iterations,
    initial_model=None,
    train_dir=None,
    metrics_file=None,
    baseline_file=None,
    merge_model=None,
):
  if baseline_file is not None and initial_model is not None:
    raise ValueError(
      'CatBoost does not support baseline together with initial_model; '
      'use merge_model for residual continuation')
  train_pool = Pool('libsvm://' + str(pathlib.Path(svm_file).resolve()))
  if baseline_file is not None:
    baseline = numpy.atleast_1d(numpy.loadtxt(
      pathlib.Path(baseline_file).resolve(),
      dtype=numpy.float64))
    if baseline.shape != (train_pool.num_row(),):
      raise ValueError(
        'Baseline row count does not match training pool: ' +
        str(baseline.shape[0]) + ' != ' + str(train_pool.num_row()))
    train_pool.set_baseline(baseline)
  model = CatBoostClassifier(
    iterations=iterations,
    learning_rate=0.1,
    depth=6,
    loss_function='Logloss',
    verbose=0,
    train_dir=train_dir,
    use_best_model=False)
  model.fit(
    train_pool,
    init_model=initial_model,
    verbose=True)
  learn_metrics = model.get_evals_result().get('learn', {})
  logloss = learn_metrics.get('Logloss', [])
  if not logloss:
    raise RuntimeError('CatBoost did not return train Logloss')
  metrics = {'Logloss': float(logloss[-1])}
  if merge_model is not None:
    previous_model = CatBoostClassifier()
    previous_model.load_model(str(pathlib.Path(merge_model).resolve()))
    model = sum_models([previous_model, model])
  model.save_model(str(pathlib.Path(output_model).resolve()))
  if metrics_file is not None:
    with pathlib.Path(metrics_file).open('w') as output:
      json.dump(metrics, output)
      output.write('\n')
  return metrics


def main():
  parser = argparse.ArgumentParser(description='Train one CatBoost chunk.')
  parser.add_argument('--svm-file', required=True)
  parser.add_argument('--output-model', required=True)
  parser.add_argument('--iterations', required=True, type=int)
  parser.add_argument('--initial-model')
  parser.add_argument('--train-dir')
  parser.add_argument('--metrics-file', required=True)
  parser.add_argument('--baseline-file')
  parser.add_argument('--merge-model')
  args = parser.parse_args()
  train_chunk(
    args.svm_file,
    args.output_model,
    args.iterations,
    args.initial_model,
    args.train_dir,
    args.metrics_file,
    args.baseline_file,
    args.merge_model)


if __name__ == '__main__':
  main()
