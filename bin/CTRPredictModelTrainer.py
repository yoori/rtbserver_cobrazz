#!/usr/bin/env python3.12

import argparse
import json
import logging
import pathlib
import shutil
import signal
import subprocess
import sys
import tempfile
import time

from rtbserver_utils.CatBoostTrainer import CatBoostTrainer
from rtbserver_utils.CTRPredictModelGeneratorConfig import load_config
from rtbserver_utils.PostgresFeatureNameResolver import PostgresFeatureNameResolver
from rtbserver_utils.RImpressionTrainExporter import RImpressionTrainExporter
from rtbserver_utils.SignalInterruptHandler import SignalInterruptHandler


logger = logging.getLogger(__name__)

FEATURE_CONFIG = {
  'features_dimension': 14,
  'features': [
    ['publisher'],
    ['tag'],
    ['sizeid'],
    ['wd'],
    ['hour'],
    ['device'],
    ['campaign'],
    ['group'],
    ['ccid'],
    ['campaign_freq'],
    ['campaign_freq_log'],
    ['geoch'],
    ['userch'],
  ],
}


def prepare_features_config(work_dir):
  features_config_file = work_dir / 'CTRGeneratorConfig.json'
  if not features_config_file.exists():
    with features_config_file.open('w') as output_file:
      json.dump(FEATURE_CONFIG, output_file, separators=(',', ':'))
      output_file.write('\n')
  return features_config_file


def generate_libsvm(
    csv_file,
    svm_file,
    features_config_file,
    dictionary_file=None,
    feature_indexes_file=None,
):
  command = [
    'CTRGenerator',
    'generate-svm',
    str(features_config_file),
    '--model=catboost',
  ]
  if dictionary_file is not None:
    command.append('--dictionary=' + str(dictionary_file))
  if feature_indexes_file is not None:
    command.append('--feature-indexes-file=' + str(feature_indexes_file))
  with csv_file.open() as input_file, svm_file.open('w') as output_file:
    subprocess.run(
      command,
      check=True,
      stdin=input_file,
      stdout=output_file)


def stream_libsvm_chunks(
    csv_chunks,
    work_dir,
    file_prefix,
    features_config_file,
    feature_indexes_file=None,
    dictionary_lines=None,
):
  csv_iterator = iter(csv_chunks)
  svm_file = None
  dictionary_file = None
  try:
    for chunk_index, (csv_file, row_count) in enumerate(csv_iterator):
      logger.debug(
        'Generating %s LibSVM chunk %d with %d rows',
        file_prefix,
        chunk_index + 1,
        row_count)
      svm_file = work_dir / (
        file_prefix + '-' + str(chunk_index).zfill(3) + '.libsvm')
      if dictionary_lines is not None:
        dictionary_file = work_dir / (
          file_prefix + '-' + str(chunk_index).zfill(3) + '.features')
      generate_libsvm(
        csv_file,
        svm_file,
        features_config_file,
        dictionary_file,
        feature_indexes_file)
      if dictionary_file is not None:
        with dictionary_file.open('rb') as input_file:
          dictionary_lines.update(input_file)
        dictionary_file.unlink()
        dictionary_file = None

      try:
        yield svm_file
      finally:
        try:
          svm_file.unlink()
        except FileNotFoundError:
          pass
        svm_file = None
  finally:
    close = getattr(csv_iterator, 'close', None)
    if close is not None:
      close()
    if svm_file is not None:
      try:
        svm_file.unlink()
      except FileNotFoundError:
        pass
    if dictionary_file is not None:
      try:
        dictionary_file.unlink()
      except FileNotFoundError:
        pass


def prepare_validation_sets(
    exporter,
    work_dir,
    features_config_file,
    date_from,
    date_to,
    validation_rows,
    validation_sets,
):
  validation_csv_files = []
  selection_validation_files = []
  csv_chunks = exporter.export_chunks(
    work_dir,
    'validation-source',
    validation_rows * validation_sets,
    validation_rows,
    date_from,
    date_to,
    exporter.validation_condition())
  for chunk_index, (csv_file, row_count) in enumerate(csv_chunks):
    if row_count != validation_rows:
      raise RuntimeError(
        'Validation chunk contains ' + str(row_count) +
        ' rows, expected ' + str(validation_rows))
    saved_csv_file = work_dir / (
      'validation-' + str(chunk_index).zfill(3) + '.csv')
    shutil.copyfile(csv_file, saved_csv_file)
    validation_csv_files.append(saved_csv_file)
    svm_file = work_dir / (
      'selection-validation-' + str(chunk_index).zfill(3) + '.libsvm')
    generate_libsvm(saved_csv_file, svm_file, features_config_file)
    selection_validation_files.append(svm_file)

  if len(validation_csv_files) != validation_sets:
    raise RuntimeError(
      'Created ' + str(len(validation_csv_files)) +
      ' validation sets, expected ' + str(validation_sets))
  return validation_csv_files, selection_validation_files


def filter_validation_sets(
    validation_csv_files,
    selection_validation_files,
    work_dir,
    features_config_file,
    feature_indexes_file,
):
  filtered_validation_files = []
  try:
    for chunk_index, csv_file in enumerate(validation_csv_files):
      svm_file = work_dir / (
        'validation-' + str(chunk_index).zfill(3) + '.libsvm')
      generate_libsvm(
        csv_file,
        svm_file,
        features_config_file,
        feature_indexes_file=feature_indexes_file)
      filtered_validation_files.append(svm_file)
  finally:
    for file_path in validation_csv_files + selection_validation_files:
      try:
        file_path.unlink()
      except FileNotFoundError:
        pass
  return filtered_validation_files


def generate_model(config):
  workspace_root = pathlib.Path(config.workspace_root)
  work_dir = workspace_root / 'CTRPredictModelGenerator'
  output_dir = config.model_root()
  work_dir.mkdir(parents=True, exist_ok=True)
  features_config_file = prepare_features_config(work_dir)
  dictionary_file = work_dir / 'RImpressionTrain.features'
  feature_indexes_file = work_dir / 'RImpressionTrain.feature-indexes'

  logger.debug('Loading data from ClickHouse')
  exporter = RImpressionTrainExporter(config.clickhouse_conn, logger)
  validation_sets = (
    config.selection_validation_sets +
    config.training_validation_sets +
    config.final_test_sets)
  selection_rows = config.selection_chunk_rows * config.selection_fit_steps
  training_rows = config.main_chunk_rows * config.training_fit_steps
  validation_rows_total = config.validation_set_rows * validation_sets
  source_rows = exporter.required_source_rows(
    max(selection_rows, training_rows),
    validation_rows_total)
  date_from, date_to = exporter.find_date_range(
    source_rows,
    config.data_delay)
  available_training_rows = exporter.count_rows(
    date_from,
    date_to,
    exporter.training_condition())
  available_validation_rows = exporter.count_rows(
    date_from,
    date_to,
    exporter.validation_condition())
  validation_rows = min(
    config.validation_set_rows,
    available_validation_rows // validation_sets)
  if validation_rows == 0:
    raise RuntimeError('Not enough rows to create validation sets')
  if available_training_rows == 0:
    raise RuntimeError('No rows are available for training')
  selection_rows = min(selection_rows, available_training_rows)
  training_rows = min(training_rows, available_training_rows)
  logger.info(
    'Using rows: selection=%d, training=%d, validation=%d x %d',
    selection_rows,
    training_rows,
    validation_rows,
    validation_sets)

  trainer = CatBoostTrainer(
    features_config_file=features_config_file,
    train_dir=work_dir / 'catboost_info')
  with tempfile.TemporaryDirectory(
      dir=str(work_dir),
      prefix='ctr-model-cycle.') as cycle_dir_name:
    cycle_dir = pathlib.Path(cycle_dir_name)
    validation_csv_files, selection_validation_files = prepare_validation_sets(
      exporter,
      cycle_dir,
      features_config_file,
      date_from,
      date_to,
      validation_rows,
      validation_sets)

    selection_csv_chunks = exporter.export_partitioned_chunks(
      cycle_dir,
      'selection',
      selection_rows,
      config.selection_chunk_rows,
      config.selection_fit_steps,
      date_from,
      date_to)
    selection_svm_chunks = stream_libsvm_chunks(
      selection_csv_chunks,
      cycle_dir,
      'selection',
      features_config_file)
    feature_indexes = trainer.select_feature_indexes_from_chunks(
      selection_svm_chunks,
      selection_validation_files[:config.selection_validation_sets],
      fit_iterations=config.fit_iterations,
      patience=config.selection_patience,
      work_dir=cycle_dir,
      fit_steps=config.selection_fit_steps)
    with feature_indexes_file.open('w') as output_file:
      for feature_index in sorted(feature_indexes):
        output_file.write(str(feature_index) + '\n')
    logger.info('Selected %d LibSVM feature indexes', len(feature_indexes))

    validation_files = filter_validation_sets(
      validation_csv_files,
      selection_validation_files,
      cycle_dir,
      features_config_file,
      feature_indexes_file)
    training_validation_begin = config.selection_validation_sets
    training_validation_end = (
      training_validation_begin + config.training_validation_sets)
    dictionary_lines = set()
    training_csv_chunks = exporter.export_partitioned_chunks(
      cycle_dir,
      'training',
      training_rows,
      config.main_chunk_rows,
      config.training_fit_steps,
      date_from,
      date_to)
    training_svm_chunks = stream_libsvm_chunks(
      training_csv_chunks,
      cycle_dir,
      'training',
      features_config_file,
      feature_indexes_file,
      dictionary_lines)
    model = trainer.train_from_chunks(
      training_svm_chunks,
      validation_files[training_validation_begin:training_validation_end],
      validation_files[training_validation_end:],
      fit_iterations=config.fit_iterations,
      patience=config.training_patience,
      work_dir=cycle_dir,
      fit_steps=config.training_fit_steps)

    with dictionary_file.open('wb') as output_file:
      for line in sorted(dictionary_lines):
        output_file.write(line)
    result_dir = trainer.save_campaign_manager_model(
      model,
      output_dir,
      algorithm_id=config.algorithm_id,
      feature_dictionary_file=dictionary_file,
      feature_name_resolver=PostgresFeatureNameResolver(config.postgres_conn))
    logger.info('Generated CampaignManager model in %s', result_dir)
    return result_dir


def wait_for_period(interrupter, period):
  deadline = time.monotonic() + period
  while not interrupter.interrupted():
    remaining = deadline - time.monotonic()
    if remaining <= 0:
      return
    time.sleep(min(remaining, 0.1))


def run_service(config, run_once):
  if run_once:
    generate_model(config)
    return

  with SignalInterruptHandler(
      [signal.SIGINT, signal.SIGTERM, signal.SIGUSR1, signal.SIGHUP],
      handler=None) as interrupter:
    while not interrupter.interrupted():
      try:
        logger.info('Starting CTR model generation')
        generate_model(config)
        logger.info('CTR model generation completed')
      except Exception:
        logger.exception('CTR model generation failed')
      wait_for_period(interrupter, config.generate_period)


def main():
  parser = argparse.ArgumentParser(description='CTR model trainer.')
  parser.add_argument('--config', required=True, help='JSON configuration file.')
  parser.add_argument('--run-once', action='store_true')
  args = parser.parse_args()
  run_service(load_config(args.config), args.run_once)


if __name__ == '__main__':
  logging.basicConfig(
    level='DEBUG',
    format='%(asctime)s Trainer[%(process)d] %(levelname)s %(message)s')
  try:
    main()
  except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError):
    logger.exception('CTR model trainer failed')
    sys.exit(1)
