import argparse
import json
import pathlib

import numpy
from catboost import CatBoostClassifier, Pool


CTR_GOALS = numpy.arange(31, dtype=numpy.float64) / 1000


def ctr_threshold_statistics(predictions, labels):
  bucket_indexes = numpy.searchsorted(
    CTR_GOALS,
    predictions,
    side='left')
  bucket_count = len(CTR_GOALS) + 1
  impressions = numpy.bincount(
    bucket_indexes,
    minlength=bucket_count)
  clicks = numpy.bincount(
    bucket_indexes,
    weights=labels,
    minlength=bucket_count)
  predicted_ctr_sums = numpy.bincount(
    bucket_indexes,
    weights=predictions,
    minlength=bucket_count)
  impressions = numpy.cumsum(impressions[::-1])[::-1]
  clicks = numpy.cumsum(clicks[::-1])[::-1]
  predicted_ctr_sums = numpy.cumsum(predicted_ctr_sums[::-1])[::-1]

  return [
    {
      'ctr_goal': float(ctr_goal),
      'impressions': int(impressions[index + 1]),
      'clicks': int(round(clicks[index + 1])),
      'predicted_ctr_sum': float(predicted_ctr_sums[index + 1]),
    }
    for index, ctr_goal in enumerate(CTR_GOALS)
  ]


def load_baseline(baseline_file, row_count):
  baseline = numpy.atleast_1d(numpy.loadtxt(
    pathlib.Path(baseline_file).resolve(),
    dtype=numpy.float64))
  if baseline.shape != (row_count,):
    raise ValueError(
      'Baseline row count does not match evaluation pool: ' +
      str(baseline.shape[0]) + ' != ' + str(row_count))
  return baseline


def evaluate_model(
    model_file,
    svm_file,
    include_ctr_thresholds=False,
    baseline_file=None,
    raw_predictions_file=None,
    model_raw_predictions_file=None,
):
  model = CatBoostClassifier()
  model.load_model(str(pathlib.Path(model_file).resolve()))
  validation_pool = Pool(
    'libsvm://' + str(pathlib.Path(svm_file).resolve()))
  raw_predictions = numpy.asarray(
    model.predict(
      validation_pool,
      prediction_type='RawFormulaVal'),
    dtype=numpy.float64)
  if model_raw_predictions_file is not None:
    numpy.savetxt(
      pathlib.Path(model_raw_predictions_file).resolve(),
      raw_predictions,
      fmt='%.17g')
  if baseline_file is not None:
    raw_predictions += load_baseline(
      baseline_file,
      validation_pool.num_row())
  if raw_predictions_file is not None:
    numpy.savetxt(
      pathlib.Path(raw_predictions_file).resolve(),
      raw_predictions,
      fmt='%.17g')
  labels = numpy.asarray(
    validation_pool.get_label(),
    dtype=numpy.float64)
  logloss = numpy.mean(
    numpy.logaddexp(0, raw_predictions) - labels * raw_predictions)
  result = {
    'Logloss': float(logloss),
  }
  if include_ctr_thresholds:
    predictions = numpy.exp(-numpy.logaddexp(0, -raw_predictions))
    result['ctr_thresholds'] = ctr_threshold_statistics(predictions, labels)
  return result


def main():
  parser = argparse.ArgumentParser(description='Evaluate a CatBoost model.')
  parser.add_argument('--model-file', required=True)
  parser.add_argument('--svm-file', required=True)
  parser.add_argument('--ctr-thresholds', action='store_true')
  parser.add_argument('--baseline-file')
  parser.add_argument('--raw-predictions-file')
  parser.add_argument('--model-raw-predictions-file')
  args = parser.parse_args()
  print(json.dumps(evaluate_model(
    args.model_file,
    args.svm_file,
    args.ctr_thresholds,
    args.baseline_file,
    args.raw_predictions_file,
    args.model_raw_predictions_file)))


if __name__ == '__main__':
  main()
