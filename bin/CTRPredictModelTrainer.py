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


def utc_now_text():
  return (
    datetime.datetime.now(datetime.timezone.utc)
    .replace(microsecond=0)
    .isoformat()
    .replace('+00:00', 'Z'))


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

CAMPAIGN_MODEL_FEATURE_CONFIG = {
  'features_dimension': 14,
  'features': [
    ['publisher'],
    ['tag'],
    ['sizeid'],
    ['wd'],
    ['hour'],
    ['device'],
    ['group'],
    ['ccid'],
    ['campaign_freq'],
    ['campaign_freq_log'],
    ['geoch'],
    ['userch'],
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
    self.traits = None

  def __enter__(self):
    self.model_root.mkdir(parents=True, exist_ok=True)
    result_path = self.model_root / self.model_id
    if result_path.exists():
      raise FileExistsError(
        "Model directory already exists: '" + str(result_path) + "'")
    self.path.mkdir()
    try:
      self.traits = {
        'status': 'in_progress',
        'train_start': self.train_start_text,
        'pid': os.getpid(),
      }
      self.write_traits_()
    except Exception:
      shutil.rmtree(self.path, ignore_errors=True)
      raise
    return self

  def publish_model_plan(self, models, **plan_traits):
    self.traits['models'] = [dict(model) for model in models]
    campaign_models = [
      model
      for model in models
      if model.get('kind') == 'campaign'
    ]
    self.traits['model_plan'] = {
      'models': len(models),
      'campaign_models': len(campaign_models),
      **plan_traits,
    }
    self.write_traits_()

  def start_models(self, *model_names):
    train_start = utc_now_text()
    self.update_models_(
      model_names,
      status='training',
      train_start=train_start)
    return train_start

  def complete_models(self, *model_names):
    train_end = utc_now_text()
    self.update_models_(
      model_names,
      status='completed',
      train_end=train_end)
    return train_end

  def skip_model(self, model_name, reason):
    self.update_models_(
      (model_name,),
      status='skipped',
      train_end=utc_now_text(),
      skip_reason=reason)

  def update_models_(self, model_names, **values):
    model_names = set(model_names)
    found_names = set()
    for model in self.traits.get('models', []):
      if model.get('name') in model_names:
        model.update(values)
        found_names.add(model['name'])
    missing_names = model_names - found_names
    if missing_names:
      raise KeyError(
        'Unknown in-progress models: ' + ', '.join(sorted(missing_names)))
    self.write_traits_()

  def write_traits_(self):
    temporary_file = self.path / '.traits.json.tmp'
    with temporary_file.open('w') as output:
      json.dump(self.traits, output, indent=2)
      output.write('\n')
    os.replace(temporary_file, self.path / 'traits.json')

  def __exit__(self, exception_type, exception_value, traceback):
    if exception_type is None:
      shutil.rmtree(self.path, ignore_errors=True)
      return
    if not self.path.is_dir():
      return

    train_end = utc_now_text()
    self.traits['status'] = 'interrupted'
    self.traits['train_end'] = train_end
    self.traits['interruption_reason'] = exception_type.__name__
    for model in self.traits.get('models', []):
      if model.get('status') == 'training':
        model['status'] = 'interrupted'
        model['train_end'] = train_end
    try:
      self.write_traits_()
    except Exception:
      logger.exception(
        'Failed to persist interrupted model status in %s',
        self.path)


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


def prepare_campaign_validation_sets(
    exporter,
    work_dir,
    campaign_id,
    stable_features_config_file,
    campaign_features_config_file,
    stable_feature_indexes_file,
    stable_model_file,
    stable_trainer,
    date_from,
    date_to,
    validation_rows,
    validation_sets,
):
  svm_files = []
  baseline_files = []
  statistics = []
  condition = (
    '(' + exporter.validation_condition() + ') AND (' +
    exporter.campaign_condition(campaign_id) + ')')
  csv_chunks = exporter.export_chunks(
    work_dir,
    'campaign-' + str(campaign_id) + '-validation-source',
    validation_rows * validation_sets,
    validation_rows,
    date_from,
    date_to,
    condition)
  try:
    for index, (csv_file, row_count) in enumerate(csv_chunks):
      if row_count != validation_rows:
        raise RuntimeError(
          'Campaign validation chunk contains ' + str(row_count) +
          ' rows, expected ' + str(validation_rows))
      prefix = (
        'campaign-' + str(campaign_id) + '-validation-' +
        str(index).zfill(3))
      stable_svm = work_dir / (prefix + '-stable.libsvm')
      campaign_svm = work_dir / (prefix + '.libsvm')
      baseline_file = work_dir / (prefix + '.baseline')
      stats_file = work_dir / (prefix + '.stats')
      generate_libsvm(
        csv_file,
        stable_svm,
        stable_features_config_file,
        feature_indexes_file=stable_feature_indexes_file)
      generate_libsvm(
        csv_file,
        campaign_svm,
        campaign_features_config_file,
        feature_stats_file=stats_file)
      current_statistics = FeatureStatistics()
      current_statistics.add_file(stats_file)
      stable_trainer.predict_raw_(
        stable_model_file,
        stable_svm,
        baseline_file)
      stable_svm.unlink()
      stats_file.unlink()
      svm_files.append(campaign_svm)
      baseline_files.append(baseline_file)
      statistics.append(current_statistics)
  finally:
    csv_chunks.close()
  if len(svm_files) != validation_sets:
    raise RuntimeError(
      'Created ' + str(len(svm_files)) +
      ' campaign validation sets, expected ' + str(validation_sets))
  return list(zip(svm_files, baseline_files)), statistics


def stream_campaign_chunks(
    csv_chunks,
    work_dir,
    campaign_id,
    stable_features_config_file,
    campaign_features_config_file,
    stable_feature_indexes_file,
    stable_model_file,
    stable_trainer,
    dictionary_lines,
    feature_statistics,
):
  csv_iterator = iter(csv_chunks)
  current_files = []
  try:
    for index, (csv_file, row_count) in enumerate(csv_iterator):
      logger.debug(
        'Generating campaign %d LibSVM chunk %d with %d rows',
        campaign_id,
        index + 1,
        row_count)
      prefix = 'campaign-' + str(campaign_id) + '-' + str(index).zfill(3)
      stable_svm = work_dir / (prefix + '-stable.libsvm')
      campaign_svm = work_dir / (prefix + '.libsvm')
      baseline_file = work_dir / (prefix + '.baseline')
      dictionary_file = work_dir / (prefix + '.features')
      stats_file = work_dir / (prefix + '.stats')
      current_files = [
        stable_svm,
        campaign_svm,
        baseline_file,
        dictionary_file,
        stats_file,
      ]
      generate_libsvm(
        csv_file,
        stable_svm,
        stable_features_config_file,
        feature_indexes_file=stable_feature_indexes_file)
      generate_libsvm(
        csv_file,
        campaign_svm,
        campaign_features_config_file,
        dictionary_file=dictionary_file,
        feature_stats_file=stats_file)
      with dictionary_file.open('rb') as input_file:
        dictionary_lines.update(input_file)
      feature_statistics.add_file(stats_file)
      stable_trainer.predict_raw_(
        stable_model_file,
        stable_svm,
        baseline_file)
      try:
        yield campaign_svm, baseline_file
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


def remove_training_inputs(inputs):
  for svm_file, baseline_file in inputs:
    for file_path in (svm_file, baseline_file):
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
  campaign_features_config_file = prepare_features_config(
    work_dir,
    CAMPAIGN_MODEL_FEATURE_CONFIG,
    'CTRGeneratorCampaignModelConfig.json')
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
  eligible_campaign_candidates = exporter.eligible_campaigns(
    date_from,
    date_to,
    config.campaign_model_activity_period,
    config.min_campaign_model_imps)
  campaign_validation_sets = (
    config.training_validation_sets + config.final_test_sets)
  eligible_campaigns = [
    campaign
    for campaign in eligible_campaign_candidates
    if campaign[2] >= campaign_validation_sets
  ]
  logger.info(
    'Selected %d of %d active campaigns with more than %d training '
    'impressions and enough validation rows',
    len(eligible_campaigns),
    len(eligible_campaign_candidates),
    config.min_campaign_model_imps)
  feature_name_resolver = PostgresFeatureNameResolver(config.postgres_conn)
  campaign_names = feature_name_resolver.resolve_campaign_names(
    campaign_id for campaign_id, _, _ in eligible_campaigns)
  in_progress_model.publish_model_plan(
    [
      {
        'name': 'common',
        'kind': 'common',
        'runtime': False,
        'status': 'planned',
      },
      {
        'name': 'common_denoise',
        'kind': 'denoise_residual',
        'runtime': False,
        'status': 'planned',
      },
      {
        'name': 'common_stable',
        'kind': 'common_stable',
        'runtime': True,
        'status': 'planned',
      },
      *[
        {
          'name': 'campaign_' + str(campaign_id),
          'kind': 'campaign',
          'runtime': True,
          'status': 'planned',
          'db_campaign_id': campaign_id,
          'runtime_campaign_group_id': campaign_id,
          'campaign_name': campaign_names.get(campaign_id),
          'eligible_training_impressions': training_impressions,
          'validation_impressions': validation_impressions,
        }
        for campaign_id, training_impressions, validation_impressions
        in eligible_campaigns
      ],
    ],
    eligible_campaigns=len(eligible_campaign_candidates),
    date_from=date_from,
    date_to=date_to,
    campaign_model_activity_period=config.campaign_model_activity_period,
    min_campaign_model_imps=config.min_campaign_model_imps)
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
  campaign_model_trainer = CatBoostTrainer(
    features_config_file=campaign_features_config_file,
    train_dir=work_dir / 'catboost_info' / 'campaign-model')
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
    common_train_start = in_progress_model.start_models('common')
    common_model, common_logloss_history, common_ctr_thresholds = (
      trainer.train_from_chunks(
        training_svm_chunks,
        validation_files[training_validation_begin:training_validation_end],
        validation_files[training_validation_end:],
        fit_iterations=config.fit_iterations,
        patience=config.training_patience,
        work_dir=cycle_dir,
        fit_steps=config.training_fit_steps))
    common_train_end = in_progress_model.complete_models('common')
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
    aligned_train_start = in_progress_model.start_models(
      'common_denoise',
      'common_stable')
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
    aligned_train_end = in_progress_model.complete_models(
      'common_denoise',
      'common_stable')

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

    stable_model_file = cycle_dir / 'common-stable.cbm'
    aligned_models['stable_common']['model'].save_model(
      str(stable_model_file))
    campaign_model_entries = []
    for (
        campaign_id,
        eligible_impressions,
        available_campaign_validation_rows,
    ) in eligible_campaigns:
      campaign_condition = exporter.campaign_condition(campaign_id)
      campaign_validation_rows = min(
        config.validation_set_rows,
        available_campaign_validation_rows // campaign_validation_sets)

      logger.info(
        'Training campaign model %d (%s): train=%d, validation=%d x %d',
        campaign_id,
        campaign_names.get(campaign_id),
        eligible_impressions,
        campaign_validation_rows,
        campaign_validation_sets)
      campaign_model_name = 'campaign_' + str(campaign_id)
      campaign_train_start = in_progress_model.start_models(
        campaign_model_name)
      campaign_validation_inputs = []
      try:
        campaign_validation_inputs, campaign_validation_statistics = (
          prepare_campaign_validation_sets(
            exporter,
            cycle_dir,
            campaign_id,
            features_config_file,
            campaign_features_config_file,
            feature_indexes_file,
            stable_model_file,
            trainer,
            date_from,
            date_to,
            campaign_validation_rows,
            campaign_validation_sets))
        campaign_dictionary_lines = set()
        campaign_statistics = FeatureStatistics()
        campaign_csv_chunks = exporter.export_partitioned_chunks(
          cycle_dir,
          'campaign-' + str(campaign_id) + '-training',
          eligible_impressions,
          config.main_chunk_rows,
          config.training_fit_steps,
          date_from,
          date_to,
          campaign_condition)
        campaign_chunks = stream_campaign_chunks(
          campaign_csv_chunks,
          cycle_dir,
          campaign_id,
          features_config_file,
          campaign_features_config_file,
          feature_indexes_file,
          stable_model_file,
          trainer,
          campaign_dictionary_lines,
          campaign_statistics)
        campaign_training_validation_end = config.training_validation_sets
        campaign_result = campaign_model_trainer.train_residual_from_chunks(
          campaign_chunks,
          campaign_validation_inputs[
            :campaign_training_validation_end],
          campaign_validation_inputs[
            campaign_training_validation_end:],
          fit_iterations=config.fit_iterations,
          patience=config.training_patience,
          work_dir=cycle_dir,
          fit_steps=config.training_fit_steps)

        campaign_dictionary_file = cycle_dir / (
          'campaign-' + str(campaign_id) + '.features')
        with campaign_dictionary_file.open('wb') as output_file:
          for line in sorted(campaign_dictionary_lines):
            output_file.write(line)
        campaign_dataset_sizes = {
          'train': dataset_size([campaign_statistics]),
          'test': dataset_size(
            campaign_validation_statistics[
              :campaign_training_validation_end]),
          'final_test': dataset_size(
            campaign_validation_statistics[
              campaign_training_validation_end:]),
        }
        campaign_weight = campaign_result['weight']
        campaign_train_end = in_progress_model.complete_models(
          campaign_model_name)
        campaign_model_entries.append({
          'name': campaign_model_name,
          'trainer': campaign_model_trainer,
          'model': campaign_result['model'],
          'feature_dictionary_file': campaign_dictionary_file,
          'feature_statistics': campaign_statistics,
          'logloss_history': campaign_result['logloss_history'],
          'dataset_sizes': campaign_dataset_sizes,
          'ctr_thresholds': campaign_result['ctr_thresholds'],
          'traits': {
            'kind': 'campaign',
            'runtime': campaign_weight > 0,
            'baseline_model': 'common_stable',
            'db_campaign_id': campaign_id,
            'runtime_campaign_group_id': campaign_id,
            'campaign_name': campaign_names.get(campaign_id),
            'eligible_training_impressions': eligible_impressions,
            'weight': campaign_weight,
            'base_logloss': campaign_result['base_logloss'],
            'combined_logloss': campaign_result['combined_logloss'],
            'status': 'completed',
            'train_start': campaign_train_start,
            'train_end': campaign_train_end,
          },
        })
      finally:
        remove_training_inputs(campaign_validation_inputs)

    model_entries = [
      {
        'name': 'common',
        'trainer': trainer,
        'model': common_model,
        'feature_dictionary_file': common_dictionary_file,
        'feature_statistics': feature_statistics,
        'logloss_history': common_logloss_history,
        'dataset_sizes': common_dataset_sizes,
        'ctr_thresholds': common_ctr_thresholds,
        'traits': {
          'kind': 'common',
          'runtime': False,
          'metrics_prediction': 'sigmoid(common)',
          'status': 'completed',
          'train_start': common_train_start,
          'train_end': common_train_end,
        },
      },
      {
        'name': 'common_denoise',
        'trainer': correction_trainer,
        'model': aligned_models['campaign_correction']['model'],
        'feature_dictionary_file': correction_dictionary_file,
        'feature_statistics': correction_statistics,
        'logloss_history': aligned_models[
          'campaign_correction']['logloss_history'],
        'dataset_sizes': correction_dataset_sizes,
        'ctr_thresholds': aligned_models[
          'campaign_correction']['ctr_thresholds'],
        'traits': {
          'kind': 'denoise_residual',
          'runtime': False,
          'baseline_model': 'common',
          'metrics_prediction': 'sigmoid(common + common_denoise)',
          'oof_strategy': 'expanding_window_hash_partitions',
          'status': 'completed',
          'train_start': aligned_train_start,
          'train_end': aligned_train_end,
        },
      },
      {
        'name': 'common_stable',
        'trainer': trainer,
        'model': aligned_models['stable_common']['model'],
        'feature_dictionary_file': stable_dictionary_file,
        'feature_statistics': stable_statistics,
        'logloss_history': aligned_models[
          'stable_common']['logloss_history'],
        'dataset_sizes': stable_dataset_sizes,
        'ctr_thresholds': aligned_models[
          'stable_common']['ctr_thresholds'],
        'traits': {
          'kind': 'common_stable',
          'runtime': True,
          'subtracted_model': 'common_denoise',
          'metrics_prediction': 'sigmoid(common_stable)',
          'status': 'completed',
          'train_start': aligned_train_start,
          'train_end': aligned_train_end,
        },
      },
      *campaign_model_entries,
    ]
    train_end = utc_now_text()
    result_dir = trainer.save_campaign_manager_model_bundle(
      output_dir,
      model_entries,
      timestamp=in_progress_model.model_id,
      staging_dir=in_progress_model.path,
      algorithm_id=config.algorithm_id,
      feature_name_resolver=feature_name_resolver,
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
