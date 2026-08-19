#!/usr/bin/env python3

import argparse
import csv
import json
import math
import os
import pathlib
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ElementTree


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]


def dictionary_options(args):
  result = []
  for option, value in (
      ('cc-to-ccg', args.cc_to_ccg),
      ('cc-to-campaign', args.cc_to_campaign),
      ('tag-to-publisher', args.tag_to_publisher),
  ):
    if value:
      result.append('--' + option + '=' + value)
  return result


def training_features_size(feature_config):
  root = ElementTree.parse(feature_config).getroot()
  model = next(
    (element for element in root if element.tag.rsplit('}', 1)[-1] == 'Model'),
    None)
  if model is None:
    raise RuntimeError("Model is missing in '" + feature_config + "'")
  dimension = int(model.attrib['features_dimension'])
  return 1 << dimension


def runtime_catboost_model(runtime_config_dir):
  config_file = pathlib.Path(runtime_config_dir) / 'config.json'
  with config_file.open() as input_file:
    config = json.load(input_file)

  models = [
    model
    for algorithm in config.get('algorithms', [])
    for model in algorithm.get('models', [])
    if model.get('method') == 'catboost'
  ]
  if len(models) != 1:
    raise RuntimeError(
      "Expected one CatBoost model in '" + str(config_file) +
      "', found " + str(len(models)))

  model = models[0]
  if 'features_size' not in model:
    raise RuntimeError(
      "features_size is missing for the CatBoost model in '" +
      str(config_file) + "'")

  model_file = pathlib.Path(runtime_config_dir) / model['file']
  if not model_file.is_file():
    raise RuntimeError("CatBoost model is missing: '" + str(model_file) + "'")
  return model_file, int(model['features_size'])


def input_rows(input_file):
  with open(input_file, newline='') as stream:
    header = stream.readline().rstrip('\r\n')
    stream.seek(0)
    reader = csv.DictReader(stream)
    rows = list(reader)
    if reader.fieldnames is None:
      raise RuntimeError("CSV header is missing in '" + input_file + "'")
    return header, rows


def run_generate_svm(args, output_file):
  command = [
    args.ctr_generator,
    'generate-svm',
    args.feature_config,
    '--model=catboost',
    *dictionary_options(args),
  ]
  with open(args.input) as input_file, open(output_file, 'w') as output:
    subprocess.run(command, stdin=input_file, stdout=output, check=True)


def run_python_predict(args, svm_file, model_file, features_size):
  predictor = pathlib.Path(__file__).with_name('CatBoostPredict.py')
  environment = os.environ.copy()
  library_root = SOURCE_ROOT / 'lib'
  python_path = environment.get('PYTHONPATH')
  environment['PYTHONPATH'] = (
    str(library_root) if not python_path else
    str(library_root) + os.pathsep + python_path)

  command = [
    args.python,
    str(predictor),
    '--svm',
    svm_file,
    '--model',
    str(model_file),
    '--features-size',
    str(features_size),
  ]
  result = subprocess.run(
    command,
    check=True,
    capture_output=True,
    text=True,
    env=environment)
  return [
    float(row['predicted_ctr'])
    for row in csv.DictReader(result.stdout.splitlines())
  ]


def run_runtime_predict(args, header):
  command = [
    args.ctr_generator,
    'generate-ctr',
    args.runtime_config_dir,
    header,
    *dictionary_options(args),
  ]
  with open(args.input) as input_file:
    next(input_file)
    input_data = input_file.read()
    result = subprocess.run(
      command,
      input=input_data,
      check=True,
      capture_output=True,
      text=True)
  return [float(line) for line in result.stdout.splitlines() if line]


def row_name(row, row_index):
  for field in ('request_id', 'Request ID', '#RequestID', '#Request ID'):
    if row.get(field):
      return row[field]
  return str(row_index)


def main():
  parser = argparse.ArgumentParser(
    description='Compare training and runtime CatBoost feature normalization.')
  parser.add_argument('--input', required=True, help='RImpressionTrain CSV file.')
  parser.add_argument(
    '--feature-config',
    required=True,
    help='CTRGenerator XML feature configuration.')
  parser.add_argument(
    '--runtime-config-dir',
    required=True,
    help='CTRProvider directory containing config.json and the CatBoost model.')
  parser.add_argument(
    '--ctr-generator',
    default='CTRGenerator',
    help='CTRGenerator executable.')
  parser.add_argument(
    '--python',
    default='python3.12',
    help='Python executable with CatBoost dependencies.')
  parser.add_argument('--cc-to-ccg')
  parser.add_argument('--cc-to-campaign')
  parser.add_argument('--tag-to-publisher')
  parser.add_argument('--tolerance', type=float, default=1e-7)
  args = parser.parse_args()

  header, rows = input_rows(args.input)
  model_file, runtime_features_size = runtime_catboost_model(
    args.runtime_config_dir)
  train_features_size = training_features_size(args.feature_config)
  if runtime_features_size != train_features_size:
    raise RuntimeError(
      'features_size mismatch: training=' + str(train_features_size) +
      ', runtime=' + str(runtime_features_size))

  with tempfile.TemporaryDirectory(prefix='catboost-feature-parity-') as temp_dir:
    svm_file = str(pathlib.Path(temp_dir) / 'RImpressionTrain.libsvm')
    run_generate_svm(args, svm_file)
    python_predictions = run_python_predict(
      args,
      svm_file,
      model_file,
      train_features_size)
    runtime_predictions = run_runtime_predict(args, header)

  expected_rows = len(rows)
  if len(python_predictions) != expected_rows:
    raise RuntimeError(
      'Python prediction count mismatch: input=' + str(expected_rows) +
      ', predictions=' + str(len(python_predictions)))
  if len(runtime_predictions) != expected_rows:
    raise RuntimeError(
      'Runtime prediction count mismatch: input=' + str(expected_rows) +
      ', predictions=' + str(len(runtime_predictions)))

  mismatches = []
  max_delta = 0.0
  for row_index, (python_ctr, runtime_ctr) in enumerate(zip(
      python_predictions,
      runtime_predictions,
  )):
    delta = abs(python_ctr - runtime_ctr)
    max_delta = max(max_delta, delta)
    if not math.isclose(
        python_ctr,
        runtime_ctr,
        rel_tol=0.0,
        abs_tol=args.tolerance,
    ):
      mismatches.append((row_index, python_ctr, runtime_ctr, delta))

  for row_index, python_ctr, runtime_ctr, delta in mismatches[:20]:
    print(
      'mismatch row=' + row_name(rows[row_index], row_index) +
      ' python=' + format(python_ctr, '.17g') +
      ' runtime=' + format(runtime_ctr, '.17g') +
      ' delta=' + format(delta, '.17g'),
      file=sys.stderr)

  print(
    'rows=' + str(expected_rows) +
    ' mismatches=' + str(len(mismatches)) +
    ' max_delta=' + format(max_delta, '.17g'))
  return 1 if mismatches else 0


if __name__ == '__main__':
  try:
    sys.exit(main())
  except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
    print(str(error), file=sys.stderr)
    sys.exit(1)
