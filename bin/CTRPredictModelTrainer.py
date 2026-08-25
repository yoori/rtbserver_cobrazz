#!/usr/bin/env python3.12

import argparse
import csv
import datetime
import json
import logging
import os
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

CAMPAIGN_CORRECTION_FEATURE_CONFIG = {
  'features_dimension': 14,
  'features': [
    ['campaign'],
    ['group'],
    ['ccid'],
    ['campaign', 'publisher'],
    ['campaign', 'tag'],
    ['campaign', 'sizeid'],
    ['campaign', 'wd'],
    ['campaign', 'hour'],
    ['campaign', 'device'],
    ['campaign', 'campaign_freq'],
    ['campaign', 'campaign_freq_log'],
    ['campaign', 'geoch'],
    ['campaign', 'userch'],
  ],
}


class FeatureStatistics:
  def __init__(self):
    self.total_impressions = 0
    self.total_clicks = 0
    self.features = {}

  def add_file(self, stats_file):
    with pathlib.Path(stats_file).open(newline='') as input_file:
      for line_number, row in enumerate(csv.reader(input_file), 1):
        if len(row) != 3:
          raise ValueError(
            'Invalid feature statistics row ' + str(line_number) + ': ' +
            repr(row))
        try:
          index, impressions, clicks = (int(value) for value in row)
        except ValueError as error:
          raise ValueError(
            'Invalid feature statistics value at row ' +
            str(line_number) + ': ' + repr(row)) from error
        if index == 0:
          self.total_impressions += impressions
          self.total_clicks += clicks
        else:
          feature_impressions, feature_clicks = self.features.get(
            index, (0, 0))
          self.features[index] = (
            feature_impressions + impressions,
            feature_clicks + clicks)

  def get(self, index):
    return self.features.get(index, (0, 0))


class InProgressModel:
  def __init__(self, model_root, train_start=None):
    if train_start is None:
      train_start = datetime.datetime.now(datetime.timezone.utc)
    self.train_start = train_start.replace(microsecond=0)
    self.train_start_text = (
      self.train_start.isoformat().replace('+00:00', 'Z'))
    self.model_id = self.train_start.strftime('%Y%m%d.%H%M%S')
    self.model_root = pathlib.Path(model_root)
    self.path = self.model_root / ('~' + self.model_id)

  def __enter__(self):
    self.model_root.mkdir(parents=True, exist_ok=True)
    result_path = self.model_root / self.model_id
    if result_path.exists():
      raise FileExistsError(
        "Model directory already exists: '" + str(result_path) + "'")
    self.path.mkdir()
    try:
      with (self.path / 'traits.json').open('w') as output:
        json.dump({
          'status': 'in_progress',
          'train_start': self.train_start_text,
          'pid': os.getpid(),
        }, output, indent=2)
        output.write('\n')
    except Exception:
      shutil.rmtree(self.path, ignore_errors=True)
      raise
    return self

  def __exit__(self, exception_type, exception_value, traceback):
    shutil.rmtree(self.path, ignore_errors=True)


def prepare_features_config(
    work_dir,
    feature_config=FEATURE_CONFIG,
    file_name='CTRGeneratorConfig.json',
):
  features_config_file = work_dir / file_name
  current_config = None
  if features_config_file.exists():
    with features_config_file.open() as input_file:
      current_config = json.load(input_file)
  if current_config != feature_config:
    with features_config_file.open('w') as output_file:
      json.dump(feature_config, output_file, separators=(',', ':'))
      output_file.write('\n')
  return features_config_file


def generate_libsvm(
    csv_file,
    svm_file,
    features_config_file,
    dictionary_file=None,
    feature_indexes_file=None,
    feature_stats_file=None,
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
  if feature_stats_file is not None:
    command.append('--feature-stats=' + str(feature_stats_file))
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
    feature_statistics=None,
):
  csv_iterator = iter(csv_chunks)
  svm_file = None
  dictionary_file = None
  feature_stats_file = None
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
      if feature_statistics is not None:
        feature_stats_file = work_dir / (
          file_prefix + '-' + str(chunk_index).zfill(3) + '.stats')
      generate_libsvm(
        csv_file,
        svm_file,
        features_config_file,
        dictionary_file,
        feature_indexes_file,
        feature_stats_file)
      if dictionary_file is not None:
        with dictionary_file.open('rb') as input_file:
          dictionary_lines.update(input_file)
        dictionary_file.unlink()
        dictionary_file = None
      if feature_stats_file is not None:
        feature_statistics.add_file(feature_stats_file)
        feature_stats_file.unlink()
        feature_stats_file = None

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
    if feature_stats_file is not None:
      try:
        feature_stats_file.unlink()
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
  result = prepare_component_validation_sets(
    validation_csv_files,
    selection_validation_files,
    work_dir,
    {
      'common': (features_config_file, feature_indexes_file),
    })
  return result['common']


def prepare_component_validation_sets(
    validation_csv_files,
    selection_validation_files,
    work_dir,
    component_configs,
):
  result = {
    name: ([], [])
    for name in component_configs
  }
  feature_stats_file = None
  try:
    for component_name, config in component_configs.items():
      features_config_file, feature_indexes_file = config
      component_files, component_statistics = result[component_name]
      for chunk_index, csv_file in enumerate(validation_csv_files):
        prefix = (
          'validation' if component_name == 'common' else
          'validation-' + component_name.replace('_', '-'))
        svm_file = work_dir / (
          prefix + '-' + str(chunk_index).zfill(3) + '.libsvm')
        feature_stats_file = work_dir / (
          prefix + '-' + str(chunk_index).zfill(3) + '.stats')
        generate_libsvm(
          csv_file,
          svm_file,
          features_config_file,
          feature_indexes_file=feature_indexes_file,
          feature_stats_file=feature_stats_file)
        statistics = FeatureStatistics()
        statistics.add_file(feature_stats_file)
        feature_stats_file.unlink()
        feature_stats_file = None
        component_files.append(svm_file)
        component_statistics.append(statistics)
  finally:
    if feature_stats_file is not None:
      try:
        feature_stats_file.unlink()
      except FileNotFoundError:
        pass
    for file_path in validation_csv_files + selection_validation_files:
      try:
        file_path.unlink()
      except FileNotFoundError:
        pass
  return result


def generate_common_baselines(
    trainer,
    common_model_file,
    validation_files,
    work_dir,
):
  baselines = []
  for index, validation_file in enumerate(validation_files):
    baseline_file = work_dir / (
      'validation-common-' + str(index).zfill(3) + '.baseline')
    trainer.predict_raw_(
      common_model_file,
      validation_file,
      baseline_file)
    baselines.append(baseline_file)
  return baselines


def stream_aligned_chunks(
    csv_chunks,
    work_dir,
    common_features_config_file,
    correction_features_config_file,
    common_feature_indexes_file,
    common_model_file,
    common_trainer,
    stable_dictionary_lines,
    correction_dictionary_lines,
    stable_statistics,
    correction_statistics,
):
  csv_iterator = iter(csv_chunks)
  current_files = []
  try:
    for chunk_index, (csv_file, row_count) in enumerate(csv_iterator):
      logger.debug(
        'Generating aligned LibSVM chunk %d with %d rows',
        chunk_index + 1,
        row_count)
      prefix = str(chunk_index).zfill(3)
      stable_svm = work_dir / ('stable-common-' + prefix + '.libsvm')
      correction_svm = work_dir / (
        'campaign-correction-' + prefix + '.libsvm')
      common_baseline = work_dir / ('common-' + prefix + '.baseline')
      stable_dictionary = work_dir / ('stable-common-' + prefix + '.features')
      correction_dictionary = work_dir / (
        'campaign-correction-' + prefix + '.features')
      stable_stats = work_dir / ('stable-common-' + prefix + '.stats')
      correction_stats = work_dir / (
        'campaign-correction-' + prefix + '.stats')
      current_files = [
        stable_svm,
        correction_svm,
        common_baseline,
        stable_dictionary,
        correction_dictionary,
        stable_stats,
        correction_stats,
      ]

      generate_libsvm(
        csv_file,
        stable_svm,
        common_features_config_file,
        stable_dictionary,
        common_feature_indexes_file,
        stable_stats)
      generate_libsvm(
        csv_file,
        correction_svm,
        correction_features_config_file,
        correction_dictionary,
        feature_stats_file=correction_stats)
      with stable_dictionary.open('rb') as input_file:
        stable_dictionary_lines.update(input_file)
      with correction_dictionary.open('rb') as input_file:
        correction_dictionary_lines.update(input_file)
      stable_statistics.add_file(stable_stats)
      correction_statistics.add_file(correction_stats)
      common_trainer.predict_raw_(
        common_model_file,
        stable_svm,
        common_baseline)

      try:
        yield stable_svm, correction_svm, common_baseline
      finally:
        for file_path in current_files:
          try:
            file_path.unlink()
          except FileNotFoundError:
            pass
        current_files = []
  finally:
    close = getattr(csv_iterator, 'close', None)
    if close is not None:
      close()
    for file_path in current_files:
      try:
        file_path.unlink()
      except FileNotFoundError:
        pass


def dataset_size(statistics):
  return {
    'rows': sum(item.total_impressions for item in statistics),
    'clicks': sum(item.total_clicks for item in statistics),
  }


def generate_model(config):
  with InProgressModel(config.model_root()) as in_progress_model:
    return generate_model_(config, in_progress_model)


def generate_model_(config, in_progress_model):
  workspace_root = pathlib.Path(config.workspace_root)
  work_dir = workspace_root / 'CTRPredictModelGenerator'
  output_dir = config.model_root()
  work_dir.mkdir(parents=True, exist_ok=True)
  features_config_file = prepare_features_config(work_dir)
  correction_features_config_file = prepare_features_config(
    work_dir,
    CAMPAIGN_CORRECTION_FEATURE_CONFIG,
    'CTRGeneratorCampaignCorrectionConfig.json')
  common_dictionary_file = work_dir / 'RImpressionTrain.common.features'
  correction_dictionary_file = (
    work_dir / 'RImpressionTrain.campaign-correction.features')
  stable_dictionary_file = work_dir / 'RImpressionTrain.stable-common.features'
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
  correction_trainer = CatBoostTrainer(
    features_config_file=correction_features_config_file,
    train_dir=work_dir / 'catboost_info' / 'campaign-correction')
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

    validation_components = prepare_component_validation_sets(
      validation_csv_files,
      selection_validation_files,
      cycle_dir,
      {
        'common': (features_config_file, feature_indexes_file),
        'campaign_correction': (correction_features_config_file, None),
      })
    validation_files, validation_statistics = validation_components['common']
    correction_validation_files, correction_validation_statistics = (
      validation_components['campaign_correction'])
    training_validation_begin = config.selection_validation_sets
    training_validation_end = (
      training_validation_begin + config.training_validation_sets)
    dictionary_lines = set()
    feature_statistics = FeatureStatistics()
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
      dictionary_lines,
      feature_statistics)
    common_model, common_logloss_history, common_ctr_thresholds = (
      trainer.train_from_chunks(
        training_svm_chunks,
        validation_files[training_validation_begin:training_validation_end],
        validation_files[training_validation_end:],
        fit_iterations=config.fit_iterations,
        patience=config.training_patience,
        work_dir=cycle_dir,
        fit_steps=config.training_fit_steps))
    common_dataset_sizes = {
      'train': dataset_size([feature_statistics]),
      'test': dataset_size(
        validation_statistics[
          training_validation_begin:training_validation_end]),
      'final_test': dataset_size(
        validation_statistics[training_validation_end:]),
    }

    with common_dictionary_file.open('wb') as output_file:
      for line in sorted(dictionary_lines):
        output_file.write(line)

    common_model_file = cycle_dir / 'common.cbm'
    common_model.save_model(str(common_model_file))
    common_baselines = generate_common_baselines(
      trainer,
      common_model_file,
      validation_files,
      cycle_dir)
    correction_validation_inputs = list(zip(
      correction_validation_files[
        training_validation_begin:training_validation_end],
      common_baselines[training_validation_begin:training_validation_end]))
    correction_final_inputs = list(zip(
      correction_validation_files[training_validation_end:],
      common_baselines[training_validation_end:]))

    stable_dictionary_lines = set()
    correction_dictionary_lines = set()
    stable_statistics = FeatureStatistics()
    correction_statistics = FeatureStatistics()
    aligned_csv_chunks = exporter.export_partitioned_chunks(
      cycle_dir,
      'aligned-training',
      training_rows,
      config.main_chunk_rows,
      config.training_fit_steps,
      date_from,
      date_to)
    aligned_chunks = stream_aligned_chunks(
      aligned_csv_chunks,
      cycle_dir,
      features_config_file,
      correction_features_config_file,
      feature_indexes_file,
      common_model_file,
      trainer,
      stable_dictionary_lines,
      correction_dictionary_lines,
      stable_statistics,
      correction_statistics)
    aligned_models = trainer.train_aligned_from_chunks(
      aligned_chunks,
      correction_trainer,
      correction_validation_inputs,
      validation_files[
        training_validation_begin:training_validation_end],
      correction_final_inputs,
      validation_files[training_validation_end:],
      fit_iterations=config.fit_iterations,
      patience=config.training_patience,
      work_dir=cycle_dir,
      fit_steps=config.training_fit_steps)

    with stable_dictionary_file.open('wb') as output_file:
      for line in sorted(stable_dictionary_lines):
        output_file.write(line)
    with correction_dictionary_file.open('wb') as output_file:
      for line in sorted(correction_dictionary_lines):
        output_file.write(line)

    stable_dataset_sizes = {
      'train': dataset_size([stable_statistics]),
      'test': dataset_size(
        validation_statistics[
          training_validation_begin:training_validation_end]),
      'final_test': dataset_size(
        validation_statistics[training_validation_end:]),
    }
    correction_dataset_sizes = {
      'train': dataset_size([correction_statistics]),
      'test': dataset_size(
        correction_validation_statistics[
          training_validation_begin:training_validation_end]),
      'final_test': dataset_size(
        correction_validation_statistics[training_validation_end:]),
    }
    train_end = (
      datetime.datetime.now(datetime.timezone.utc)
      .replace(microsecond=0)
      .isoformat()
      .replace('+00:00', 'Z'))
    result_dir = trainer.save_campaign_manager_model_bundle(
      output_dir,
      {
        'common': {
          'trainer': trainer,
          'model': common_model,
          'feature_dictionary_file': common_dictionary_file,
          'feature_statistics': feature_statistics,
          'logloss_history': common_logloss_history,
          'dataset_sizes': common_dataset_sizes,
          'ctr_thresholds': common_ctr_thresholds,
        },
        'campaign_correction': {
          'trainer': correction_trainer,
          'model': aligned_models['campaign_correction']['model'],
          'feature_dictionary_file': correction_dictionary_file,
          'feature_statistics': correction_statistics,
          'logloss_history': aligned_models[
            'campaign_correction']['logloss_history'],
          'dataset_sizes': correction_dataset_sizes,
          'ctr_thresholds': aligned_models[
            'campaign_correction']['ctr_thresholds'],
        },
        'stable_common': {
          'trainer': trainer,
          'model': aligned_models['stable_common']['model'],
          'feature_dictionary_file': stable_dictionary_file,
          'feature_statistics': stable_statistics,
          'logloss_history': aligned_models[
            'stable_common']['logloss_history'],
          'dataset_sizes': stable_dataset_sizes,
          'ctr_thresholds': aligned_models[
            'stable_common']['ctr_thresholds'],
        },
      },
      timestamp=in_progress_model.model_id,
      staging_dir=in_progress_model.path,
      algorithm_id=config.algorithm_id,
      feature_name_resolver=PostgresFeatureNameResolver(config.postgres_conn),
      train_start=in_progress_model.train_start_text,
      train_end=train_end)
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
