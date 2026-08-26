#!/usr/bin/env python3.12

import argparse
import contextlib
import csv
import datetime
import json
import logging
import math
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
    ['ssp_tag_id'],
    ['ssp_ctr'],
    ['ssp_viewability'],
    ['ssp_vtr'],
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
    ['ssp_tag_id'],
    ['ssp_ctr'],
    ['ssp_viewability'],
    ['ssp_vtr'],
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
    ['ssp_tag_id'],
    ['ssp_ctr'],
    ['ssp_viewability'],
    ['ssp_vtr'],
  ],
}

SSP_CTR_FEATURE_CONFIG = {
  'features_dimension': 14,
  'features': [
    ['publisher'],
    ['tag'],
    ['etag'],
    ['url'],
    ['sizeid'],
    ['wd'],
    ['hour'],
    ['device'],
    ['colo'],
    ['geoch'],
    ['userch'],
    ['ssp_tag_id'],
    ['ssp_viewability'],
    ['ssp_vtr'],
  ],
}


def train_step(step_id, title):
  return {
    'id': step_id,
    'title': title,
    'started': None,
    'ended': None,
  }


def indexed_train_steps(prefix, title, count, actions):
  result = []
  for index in range(1, count + 1):
    suffix = str(index).zfill(3)
    progress = str(index) + '/' + str(count)
    for action_id, action_title in actions:
      result.append(train_step(
        prefix + '_' + action_id + '_' + suffix,
        title + ': ' + action_title + ' ' + progress))
  return result


def prepare_train_steps(config):
  return [
    train_step('prepare_feature_configs', 'Prepare feature configurations'),
    train_step('find_date_range', 'Determine source date range'),
    train_step('select_campaigns', 'Select eligible campaigns'),
    train_step('resolve_campaign_names', 'Resolve campaign names'),
    train_step('count_available_rows', 'Count available rows'),
    *indexed_train_steps(
      'selection_validation',
      'Feature selection validation dataset',
      config.selection_validation_sets,
      (
        ('export', 'export'),
        ('libsvm', 'build LibSVM'),
      )),
    *indexed_train_steps(
      'feature_selection',
      'Feature selection',
      config.selection_fit_steps,
      (
        ('export', 'export dataset'),
        ('libsvm', 'build LibSVM'),
        ('fit', 'fit'),
      )),
    train_step('save_feature_indexes', 'Save selected feature indexes'),
    train_step(
      'release_selection_validation',
      'Release feature selection validation files'),
  ]


def common_train_steps(config):
  validation_sets = (
    config.training_validation_sets +
    config.final_test_sets)
  return [
    *indexed_train_steps(
      'common_validation',
      'Common validation dataset',
      validation_sets,
      (
        ('export', 'export'),
        ('libsvm', 'build LibSVM'),
      )),
    *indexed_train_steps(
      'training',
      'Common training',
      config.training_fit_steps,
      (
        ('export', 'export dataset'),
        ('libsvm', 'build LibSVM'),
        ('fit', 'fit and validate'),
      )),
    train_step('save_feature_dictionary', 'Save feature dictionary'),
    train_step('save_model', 'Save common model'),
    train_step('finalize_metrics', 'Finalize common metrics'),
  ]


def ssp_ctr_train_steps(
    config,
    selection_fit_steps=None,
    training_fit_steps=None,
):
  if selection_fit_steps is None:
    selection_fit_steps = config.selection_fit_steps
  if training_fit_steps is None:
    training_fit_steps = config.training_fit_steps
  validation_sets = config.training_validation_sets + config.final_test_sets
  return [
    *indexed_train_steps(
      'ssp_selection_validation',
      'SSP CTR feature selection validation dataset',
      config.selection_validation_sets,
      (
        ('export', 'export'),
        ('libsvm', 'build LibSVM'),
      )),
    *indexed_train_steps(
      'ssp_feature_selection',
      'SSP CTR feature selection',
      selection_fit_steps,
      (
        ('export', 'export dataset'),
        ('libsvm', 'build LibSVM'),
        ('fit', 'fit independent model'),
      )),
    train_step('save_feature_indexes', 'Save selected feature indexes'),
    train_step(
      'release_selection_validation',
      'Release SSP CTR feature selection validation files'),
    *indexed_train_steps(
      'ssp_validation',
      'SSP CTR validation dataset',
      validation_sets,
      (
        ('export', 'export'),
        ('libsvm', 'build LibSVM'),
      )),
    *indexed_train_steps(
      'ssp_training',
      'Common SSP CTR training',
      training_fit_steps,
      (
        ('export', 'export dataset'),
        ('libsvm', 'build LibSVM'),
        ('fit', 'fit and validate'),
      )),
    train_step('save_feature_dictionary', 'Save feature dictionary'),
    train_step('finalize_metrics', 'Finalize Common SSP CTR metrics'),
    train_step('release_validation', 'Release SSP CTR validation files'),
  ]


def aligned_train_steps(config, title):
  validation_sets = config.training_validation_sets + config.final_test_sets
  return [
    *indexed_train_steps(
      'aligned_validation',
      'Aligned validation dataset',
      validation_sets,
      (
        ('export', 'export shared rows'),
        ('denoise_libsvm', 'build denoise LibSVM'),
        ('common_baseline', 'calculate common baseline'),
      )),
    *indexed_train_steps(
      'aligned_training',
      title,
      config.training_fit_steps,
      (
        ('export', 'export dataset'),
        ('inputs', 'build aligned inputs'),
        ('fit', 'fit and validate'),
      )),
    train_step('save_feature_dictionary', 'Save feature dictionary'),
    train_step('save_model', 'Save model'),
    train_step('finalize_metrics', 'Finalize metrics'),
    train_step('release_validation', 'Release aligned validation files'),
  ]


def campaign_fit_steps(available_rows, chunk_rows, max_steps):
  if available_rows <= 0:
    raise ValueError('available_rows must be positive')
  if chunk_rows <= 0:
    raise ValueError('chunk_rows must be positive')
  if max_steps <= 0:
    raise ValueError('max_steps must be positive')
  return min(max_steps, max(1, math.ceil(available_rows / chunk_rows)))


def scaled_fit_iterations(fit_iterations, configured_steps, actual_steps):
  if fit_iterations <= 0:
    raise ValueError('fit_iterations must be positive')
  if configured_steps <= 0:
    raise ValueError('configured_steps must be positive')
  if actual_steps <= 0:
    raise ValueError('actual_steps must be positive')
  return math.ceil(fit_iterations * configured_steps / actual_steps)


def final_model_properties(logloss_history, **extra_properties):
  if not logloss_history:
    raise ValueError('Final model logloss history is empty')
  final_metrics = min(
    logloss_history,
    key=lambda item: (float(item['test']), int(item['step'])))
  properties = [
    {'train_logloss': float(final_metrics['train'])},
    {'val_logloss': float(final_metrics['test'])},
  ]
  properties.extend(
    {name: float(value)}
    for name, value in extra_properties.items())
  return properties


def campaign_train_steps(
    config,
    selection_fit_steps=None,
    training_fit_steps=None,
):
  if selection_fit_steps is None:
    selection_fit_steps = config.selection_fit_steps
  if training_fit_steps is None:
    training_fit_steps = config.training_fit_steps
  validation_sets = config.training_validation_sets + config.final_test_sets
  return [
    *indexed_train_steps(
      'campaign_selection_validation',
      'Campaign feature selection validation dataset',
      config.selection_validation_sets,
      (
        ('export', 'export'),
        ('inputs', 'build LibSVM and stable baseline'),
      )),
    *indexed_train_steps(
      'campaign_feature_selection',
      'Campaign feature selection',
      selection_fit_steps,
      (
        ('export', 'export dataset'),
        ('inputs', 'build LibSVM and stable baseline'),
        ('fit', 'fit independent model'),
      )),
    train_step('save_feature_indexes', 'Save selected feature indexes'),
    train_step(
      'release_selection_validation',
      'Release campaign feature selection validation files'),
    *indexed_train_steps(
      'campaign_validation',
      'Campaign validation dataset',
      validation_sets,
      (
        ('export', 'export'),
        ('inputs', 'build LibSVM and stable baseline'),
      )),
    *indexed_train_steps(
      'campaign_training',
      'Campaign residual training',
      training_fit_steps,
      (
        ('export', 'export dataset'),
        ('inputs', 'build LibSVM and stable baseline'),
        ('fit', 'fit and validate'),
      )),
    train_step('save_feature_dictionary', 'Save feature dictionary'),
    train_step('finalize_metrics', 'Finalize residual metrics and weight'),
  ]


def fit_step_callbacks(progress, section, prefix):
  def start(step):
    progress.start_train_step(
      section,
      prefix + '_fit_' + str(step).zfill(3))

  def end(step):
    progress.end_train_step(
      section,
      prefix + '_fit_' + str(step).zfill(3))

  return start, end


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


class RowCounter:
  def __init__(self):
    self.rows = 0

  def add(self, rows):
    self.rows += int(rows)


class InProgressModel:
  def __init__(self, model_root, train_start=None, prepare_steps=None):
    if train_start is None:
      train_start = datetime.datetime.now(datetime.timezone.utc)
    self.train_start = train_start.replace(microsecond=0)
    self.train_start_text = (
      self.train_start.isoformat().replace('+00:00', 'Z'))
    self.model_id = self.train_start.strftime('%Y%m%d.%H%M%S')
    self.model_root = pathlib.Path(model_root)
    self.path = self.model_root / ('~' + self.model_id)
    self.traits = None
    self.prepare_steps = [dict(step) for step in (prepare_steps or [])]

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
        'prepare': {
          'status': 'planned',
          'train_steps': self.prepare_steps,
        },
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

  def complete_model(self, model_name, **traits):
    train_end = utc_now_text()
    traits.update(status='completed', train_end=train_end)
    self.update_models_((model_name,), **traits)
    return train_end

  def skip_model(self, model_name, reason):
    self.update_models_(
      (model_name,),
      status='skipped',
      train_end=utc_now_text(),
      skip_reason=reason)

  def model_traits(self, model_name):
    for model in self.traits.get('models', []):
      if model.get('name') == model_name:
        return model
    raise KeyError('Unknown in-progress model: ' + model_name)

  def complete_prepare(self):
    prepare = self.traits['prepare']
    prepare['status'] = 'completed'
    prepare['train_end'] = utc_now_text()
    self.write_traits_()

  @contextlib.contextmanager
  def train_step(self, section, step_id):
    self.start_train_step(section, step_id)
    try:
      yield
    except BaseException:
      raise
    else:
      self.end_train_step(section, step_id)

  def start_train_step(self, section, step_id):
    timestamp = utc_now_text()
    for target in self.step_targets_(section):
      step = self.find_step_(target, step_id)
      if step.get('started') is None:
        step['started'] = timestamp
      step['ended'] = None
      if target.get('status') == 'planned':
        target['status'] = 'training'
        target['train_start'] = timestamp
    self.write_traits_()
    return timestamp

  def end_train_step(self, section, step_id):
    timestamp = utc_now_text()
    for target in self.step_targets_(section):
      step = self.find_step_(target, step_id)
      if step.get('started') is None:
        raise RuntimeError("Training step '" + step_id + "' was not started")
      step['ended'] = timestamp
    self.write_traits_()
    return timestamp

  def cancel_train_step(self, section, step_id):
    for target in self.step_targets_(section):
      step = self.find_step_(target, step_id)
      step['started'] = None
      step['ended'] = None
    self.write_traits_()

  def step_targets_(self, section):
    if section == 'prepare':
      return [self.traits['prepare']]
    model_names = (section,) if isinstance(section, str) else section
    return [self.model_traits(name) for name in model_names]

  @staticmethod
  def find_step_(target, step_id):
    for step in target.get('train_steps', []):
      if step.get('id') == step_id:
        return step
    raise KeyError("Unknown training step: '" + step_id + "'")

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
    CatBoostTrainer.write_json_(temporary_file, self.traits)
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
    prepare = self.traits.get('prepare')
    if isinstance(prepare, dict) and prepare.get('status') == 'training':
      prepare['status'] = 'interrupted'
      prepare['train_end'] = train_end
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
    progress=None,
    progress_section=None,
    progress_prefix=None,
    row_counter=None,
):
  csv_iterator = iter(csv_chunks)
  svm_file = None
  dictionary_file = None
  feature_stats_file = None
  try:
    chunk_index = 0
    while True:
      step_number = str(chunk_index + 1).zfill(3)
      export_step = (
        progress_prefix + '_export_' + step_number
        if progress_prefix else None)
      try:
        if progress is not None:
          with progress.train_step(progress_section, export_step):
            csv_file, row_count = next(csv_iterator)
        else:
          csv_file, row_count = next(csv_iterator)
      except StopIteration:
        if progress is not None:
          progress.cancel_train_step(progress_section, export_step)
        break
      logger.debug(
        'Generating %s LibSVM chunk %d with %d rows',
        file_prefix,
        chunk_index + 1,
        row_count)
      if row_counter is not None:
        row_counter.add(row_count)
      svm_file = work_dir / (
        file_prefix + '-' + str(chunk_index).zfill(3) + '.libsvm')
      if dictionary_lines is not None:
        dictionary_file = work_dir / (
          file_prefix + '-' + str(chunk_index).zfill(3) + '.features')
      if feature_statistics is not None:
        feature_stats_file = work_dir / (
          file_prefix + '-' + str(chunk_index).zfill(3) + '.stats')
      libsvm_step = (
        progress_prefix + '_libsvm_' + step_number
        if progress_prefix else None)
      context = (
        progress.train_step(progress_section, libsvm_step)
        if progress is not None else contextlib.nullcontext())
      with context:
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
      chunk_index += 1
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


def remove_files(file_paths):
  for file_path in file_paths:
    try:
      pathlib.Path(file_path).unlink()
    except FileNotFoundError:
      pass


def prepare_validation_libsvm_sets(
    exporter,
    work_dir,
    file_prefix,
    features_config_file,
    date_from,
    date_to,
    validation_rows,
    validation_sets,
    validation_offset_rows=0,
    feature_indexes_file=None,
    collect_statistics=False,
    progress=None,
    progress_section=None,
    progress_prefix=None,
    condition=None,
    label='click',
):
  validation_files = []
  validation_statistics = []
  feature_stats_file = None
  completed = False
  validation_condition = exporter.validation_condition()
  if condition is not None:
    validation_condition = (
      '(' + validation_condition + ') AND (' + condition + ')')
  csv_chunks = exporter.export_chunks(
    work_dir,
    file_prefix + '-source',
    validation_rows * validation_sets,
    validation_rows,
    date_from,
    date_to,
    validation_condition,
    offset_rows=validation_offset_rows,
    label=label)
  csv_iterator = iter(csv_chunks)
  try:
    for chunk_index in range(validation_sets):
      step_number = str(chunk_index + 1).zfill(3)
      try:
        context = (
          progress.train_step(
            progress_section,
            progress_prefix + '_export_' + step_number)
          if progress is not None else contextlib.nullcontext())
        with context:
          csv_file, row_count = next(csv_iterator)
      except StopIteration:
        break
      if row_count != validation_rows:
        raise RuntimeError(
          'Validation chunk contains ' + str(row_count) +
          ' rows, expected ' + str(validation_rows))

      svm_file = work_dir / (
        file_prefix + '-' + str(chunk_index).zfill(3) + '.libsvm')
      if collect_statistics:
        feature_stats_file = work_dir / (
          file_prefix + '-' + str(chunk_index).zfill(3) + '.stats')
      context = (
        progress.train_step(
          progress_section,
          progress_prefix + '_libsvm_' + step_number)
        if progress is not None else contextlib.nullcontext())
      with context:
        generate_libsvm(
          csv_file,
          svm_file,
          features_config_file,
          feature_indexes_file=feature_indexes_file,
          feature_stats_file=feature_stats_file)
      validation_files.append(svm_file)
      if feature_stats_file is not None:
        statistics = FeatureStatistics()
        statistics.add_file(feature_stats_file)
        feature_stats_file.unlink()
        feature_stats_file = None
        validation_statistics.append(statistics)

    if len(validation_files) != validation_sets:
      raise RuntimeError(
        'Created ' + str(len(validation_files)) +
        ' validation sets, expected ' + str(validation_sets))
    completed = True
    return validation_files, validation_statistics
  finally:
    close = getattr(csv_iterator, 'close', None)
    if close is not None:
      close()
    if feature_stats_file is not None:
      remove_files([feature_stats_file])
    if not completed:
      remove_files(validation_files)


def prepare_denoise_validation_sets(
    exporter,
    work_dir,
    correction_features_config_file,
    common_model_file,
    common_trainer,
    common_validation_files,
    date_from,
    date_to,
    validation_rows,
    validation_sets,
    validation_offset_rows,
    progress=None,
):
  if len(common_validation_files) != validation_sets:
    raise ValueError(
      'Common and denoise validation set counts must match')

  correction_files = []
  baseline_files = []
  correction_statistics = []
  owned_files = []
  feature_stats_file = None
  completed = False
  csv_chunks = exporter.export_chunks(
    work_dir,
    'aligned-validation-source',
    validation_rows * validation_sets,
    validation_rows,
    date_from,
    date_to,
    exporter.validation_condition(),
    offset_rows=validation_offset_rows)
  csv_iterator = iter(csv_chunks)
  progress_section = ('common_denoise', 'common_stable')
  try:
    for index in range(validation_sets):
      step_number = str(index + 1).zfill(3)
      try:
        context = (
          progress.train_step(
            progress_section,
            'aligned_validation_export_' + step_number)
          if progress is not None else contextlib.nullcontext())
        with context:
          csv_file, row_count = next(csv_iterator)
      except StopIteration:
        break
      if row_count != validation_rows:
        raise RuntimeError(
          'Aligned validation chunk contains ' + str(row_count) +
          ' rows, expected ' + str(validation_rows))

      prefix = 'aligned-validation-' + str(index).zfill(3)
      correction_file = work_dir / (prefix + '-denoise.libsvm')
      baseline_file = work_dir / (prefix + '-common.baseline')
      feature_stats_file = work_dir / (prefix + '-denoise.stats')
      owned_files.extend((
        correction_file,
        baseline_file,
        feature_stats_file,
      ))
      context = (
        progress.train_step(
          progress_section,
          'aligned_validation_denoise_libsvm_' + step_number)
        if progress is not None else contextlib.nullcontext())
      with context:
        generate_libsvm(
          csv_file,
          correction_file,
          correction_features_config_file,
          feature_stats_file=feature_stats_file)
      statistics = FeatureStatistics()
      statistics.add_file(feature_stats_file)
      feature_stats_file.unlink()
      feature_stats_file = None

      context = (
        progress.train_step(
          progress_section,
          'aligned_validation_common_baseline_' + step_number)
        if progress is not None else contextlib.nullcontext())
      with context:
        common_trainer.predict_raw_(
          common_model_file,
          common_validation_files[index],
          baseline_file)
      correction_files.append(correction_file)
      baseline_files.append(baseline_file)
      correction_statistics.append(statistics)

    if len(correction_files) != validation_sets:
      raise RuntimeError(
        'Created ' + str(len(correction_files)) +
        ' aligned validation sets, expected ' + str(validation_sets))
    completed = True
    return list(zip(correction_files, baseline_files)), correction_statistics
  finally:
    close = getattr(csv_iterator, 'close', None)
    if close is not None:
      close()
    if feature_stats_file is not None:
      remove_files([feature_stats_file])
    if not completed:
      remove_files(owned_files)


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
    progress=None,
):
  csv_iterator = iter(csv_chunks)
  current_files = []
  try:
    chunk_index = 0
    while True:
      step_number = str(chunk_index + 1).zfill(3)
      try:
        context = (
          progress.train_step(
            ('common_denoise', 'common_stable'),
            'aligned_training_export_' + step_number)
          if progress is not None else contextlib.nullcontext())
        with context:
          csv_file, row_count = next(csv_iterator)
      except StopIteration:
        if progress is not None:
          progress.cancel_train_step(
            ('common_denoise', 'common_stable'),
            'aligned_training_export_' + step_number)
        break
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

      context = (
        progress.train_step(
          ('common_denoise', 'common_stable'),
          'aligned_training_inputs_' + step_number)
        if progress is not None else contextlib.nullcontext())
      with context:
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
      chunk_index += 1
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
    progress=None,
    campaign_feature_indexes_file=None,
    validation_offset_rows=0,
    progress_prefix='campaign_validation',
):
  svm_files = []
  baseline_files = []
  statistics = []
  owned_files = []
  completed = False
  condition = (
    '(' + exporter.validation_condition() + ') AND (' +
    exporter.campaign_condition(campaign_id) + ')')
  csv_chunks = exporter.export_chunks(
    work_dir,
    'campaign-' + str(campaign_id) + '-' + progress_prefix + '-source',
    validation_rows * validation_sets,
    validation_rows,
    date_from,
    date_to,
    condition,
    offset_rows=validation_offset_rows)
  try:
    csv_iterator = iter(csv_chunks)
    for index in range(validation_sets):
      step_number = str(index + 1).zfill(3)
      try:
        context = (
          progress.train_step(
            'campaign_' + str(campaign_id),
            progress_prefix + '_export_' + step_number)
          if progress is not None else contextlib.nullcontext())
        with context:
          csv_file, row_count = next(csv_iterator)
      except StopIteration:
        break
      if row_count != validation_rows:
        raise RuntimeError(
          'Campaign validation chunk contains ' + str(row_count) +
          ' rows, expected ' + str(validation_rows))
      prefix = (
        'campaign-' + str(campaign_id) + '-' + progress_prefix + '-' +
        str(index).zfill(3))
      stable_svm = work_dir / (prefix + '-stable.libsvm')
      campaign_svm = work_dir / (prefix + '.libsvm')
      baseline_file = work_dir / (prefix + '.baseline')
      stats_file = work_dir / (prefix + '.stats')
      owned_files.extend((
        stable_svm,
        campaign_svm,
        baseline_file,
        stats_file,
      ))
      context = (
        progress.train_step(
          'campaign_' + str(campaign_id),
          progress_prefix + '_inputs_' + step_number)
        if progress is not None else contextlib.nullcontext())
      with context:
        generate_libsvm(
          csv_file,
          stable_svm,
          stable_features_config_file,
          feature_indexes_file=stable_feature_indexes_file)
        generate_libsvm(
          csv_file,
          campaign_svm,
          campaign_features_config_file,
          feature_indexes_file=campaign_feature_indexes_file,
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
    if len(svm_files) != validation_sets:
      raise RuntimeError(
        'Created ' + str(len(svm_files)) +
        ' campaign validation sets, expected ' + str(validation_sets))
    completed = True
    return list(zip(svm_files, baseline_files)), statistics
  finally:
    csv_chunks.close()
    if not completed:
      remove_files(owned_files)


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
    progress=None,
    campaign_feature_indexes_file=None,
    progress_prefix='campaign_training',
):
  csv_iterator = iter(csv_chunks)
  current_files = []
  try:
    index = 0
    while True:
      step_number = str(index + 1).zfill(3)
      try:
        context = (
          progress.train_step(
            'campaign_' + str(campaign_id),
            progress_prefix + '_export_' + step_number)
          if progress is not None else contextlib.nullcontext())
        with context:
          csv_file, row_count = next(csv_iterator)
      except StopIteration:
        if progress is not None:
          progress.cancel_train_step(
            'campaign_' + str(campaign_id),
            progress_prefix + '_export_' + step_number)
        break
      logger.debug(
        'Generating campaign %d LibSVM chunk %d with %d rows',
        campaign_id,
        index + 1,
        row_count)
      prefix = (
        'campaign-' + str(campaign_id) + '-' + progress_prefix + '-' +
        str(index).zfill(3))
      stable_svm = work_dir / (prefix + '-stable.libsvm')
      campaign_svm = work_dir / (prefix + '.libsvm')
      baseline_file = work_dir / (prefix + '.baseline')
      dictionary_file = (
        work_dir / (prefix + '.features')
        if dictionary_lines is not None else None)
      stats_file = (
        work_dir / (prefix + '.stats')
        if feature_statistics is not None else None)
      current_files = [
        stable_svm,
        campaign_svm,
        baseline_file,
      ]
      if dictionary_file is not None:
        current_files.append(dictionary_file)
      if stats_file is not None:
        current_files.append(stats_file)
      context = (
        progress.train_step(
          'campaign_' + str(campaign_id),
          progress_prefix + '_inputs_' + step_number)
        if progress is not None else contextlib.nullcontext())
      with context:
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
          feature_indexes_file=campaign_feature_indexes_file,
          feature_stats_file=stats_file)
        if dictionary_file is not None:
          with dictionary_file.open('rb') as input_file:
            dictionary_lines.update(input_file)
        if stats_file is not None:
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
      index += 1
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
  with InProgressModel(
      config.model_root(),
      prepare_steps=prepare_train_steps(config)) as in_progress_model:
    return generate_model_(config, in_progress_model)


def generate_model_(config, in_progress_model):
  workspace_root = pathlib.Path(config.workspace_root)
  work_dir = workspace_root / 'CTRPredictModelGenerator'
  output_dir = config.model_root()
  work_dir.mkdir(parents=True, exist_ok=True)
  with in_progress_model.train_step('prepare', 'prepare_feature_configs'):
    features_config_file = prepare_features_config(work_dir)
    correction_features_config_file = prepare_features_config(
      work_dir,
      CAMPAIGN_CORRECTION_FEATURE_CONFIG,
      'CTRGeneratorCampaignCorrectionConfig.json')
    campaign_features_config_file = prepare_features_config(
      work_dir,
      CAMPAIGN_MODEL_FEATURE_CONFIG,
      'CTRGeneratorCampaignModelConfig.json')
    ssp_ctr_features_config_file = prepare_features_config(
      work_dir,
      SSP_CTR_FEATURE_CONFIG,
      'CTRGeneratorCommonSSPCTRConfig.json')
  common_dictionary_file = work_dir / 'RImpressionTrain.common.features'
  correction_dictionary_file = (
    work_dir / 'RImpressionTrain.campaign-correction.features')
  stable_dictionary_file = work_dir / 'RImpressionTrain.stable-common.features'
  feature_indexes_file = work_dir / 'RImpressionTrain.feature-indexes'
  ssp_ctr_dictionary_file = work_dir / 'RImpressionTrain.ssp-ctr.features'
  ssp_ctr_feature_indexes_file = (
    work_dir / 'RImpressionTrain.ssp-ctr.feature-indexes')

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
  with in_progress_model.train_step('prepare', 'find_date_range'):
    date_from, date_to = exporter.find_date_range(
      source_rows,
      config.data_delay)
  with in_progress_model.train_step('prepare', 'select_campaigns'):
    eligible_campaign_candidates = exporter.eligible_campaigns(
      date_from,
      date_to,
      config.campaign_model_activity_period,
      config.min_campaign_model_imps)
  campaign_validation_sets = (
    config.selection_validation_sets +
    config.training_validation_sets +
    config.final_test_sets)
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
  with in_progress_model.train_step('prepare', 'resolve_campaign_names'):
    feature_name_resolver = PostgresFeatureNameResolver(config.postgres_conn)
    campaign_names = feature_name_resolver.resolve_campaign_names(
      campaign_id for campaign_id, _, _ in eligible_campaigns)
  ssp_ctr_condition = exporter.ssp_ctr_condition()
  with in_progress_model.train_step('prepare', 'count_available_rows'):
    available_training_rows = exporter.count_rows(
      date_from,
      date_to,
      exporter.training_condition())
    available_validation_rows = exporter.count_rows(
      date_from,
      date_to,
      exporter.validation_condition())
    available_ssp_ctr_training_rows = exporter.count_rows(
      date_from,
      date_to,
      '(' + exporter.training_condition() + ') AND (' +
      ssp_ctr_condition + ')')
    available_ssp_ctr_validation_rows = exporter.count_rows(
      date_from,
      date_to,
      '(' + exporter.validation_condition() + ') AND (' +
      ssp_ctr_condition + ')')
  if available_ssp_ctr_training_rows == 0:
    raise RuntimeError('No SSP CTR rows are available for training')
  ssp_ctr_selection_fit_steps = campaign_fit_steps(
    available_ssp_ctr_training_rows,
    config.selection_chunk_rows,
    config.selection_fit_steps)
  ssp_ctr_training_fit_steps = campaign_fit_steps(
    available_ssp_ctr_training_rows,
    config.main_chunk_rows,
    config.training_fit_steps)
  in_progress_model.publish_model_plan(
    [
      {
        'name': 'common',
        'kind': 'common',
        'runtime': False,
        'status': 'planned',
        'train_steps': common_train_steps(config),
      },
      {
        'name': 'common_denoise',
        'kind': 'denoise_residual',
        'runtime': False,
        'status': 'planned',
        'train_steps': aligned_train_steps(
          config, 'Common denoise training'),
      },
      {
        'name': 'common_stable',
        'kind': 'common_stable',
        'runtime': True,
        'status': 'planned',
        'train_steps': aligned_train_steps(
          config, 'Stable common training'),
      },
      {
        'name': 'common_ssp_ctr',
        'kind': 'common_ssp_ctr',
        'runtime': False,
        'status': 'planned',
        'eligible_training_impressions': available_ssp_ctr_training_rows,
        'validation_impressions': available_ssp_ctr_validation_rows,
        'train_steps': ssp_ctr_train_steps(
          config,
          ssp_ctr_selection_fit_steps,
          ssp_ctr_training_fit_steps),
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
          'train_steps': campaign_train_steps(
            config,
            campaign_fit_steps(
              training_impressions,
              config.selection_chunk_rows,
              config.selection_fit_steps),
            campaign_fit_steps(
              training_impressions,
              config.main_chunk_rows,
              config.training_fit_steps)),
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
    min_campaign_model_imps=config.min_campaign_model_imps,
    ssp_ctr_training_rows=available_ssp_ctr_training_rows,
    ssp_ctr_validation_rows=available_ssp_ctr_validation_rows)
  validation_rows = min(
    config.validation_set_rows,
    available_validation_rows // validation_sets)
  if validation_rows == 0:
    raise RuntimeError('Not enough rows to create validation sets')
  if available_training_rows == 0:
    raise RuntimeError('No rows are available for training')
  ssp_ctr_validation_rows = min(
    config.validation_set_rows,
    available_ssp_ctr_validation_rows // validation_sets)
  if ssp_ctr_validation_rows == 0:
    raise RuntimeError('Not enough SSP CTR rows to create validation sets')
  selection_rows = min(selection_rows, available_training_rows)
  training_rows = min(training_rows, available_training_rows)
  ssp_ctr_selection_rows = min(
    available_ssp_ctr_training_rows,
    config.selection_chunk_rows * ssp_ctr_selection_fit_steps)
  ssp_ctr_training_rows = min(
    available_ssp_ctr_training_rows,
    config.main_chunk_rows * ssp_ctr_training_fit_steps)
  logger.info(
    'Using rows: selection=%d, training=%d, validation=%d x %d',
    selection_rows,
    training_rows,
    validation_rows,
    validation_sets)
  logger.info(
    'Using SSP CTR rows: selection=%d, training=%d, validation=%d x %d',
    ssp_ctr_selection_rows,
    ssp_ctr_training_rows,
    ssp_ctr_validation_rows,
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
  ssp_ctr_trainer = CatBoostTrainer(
    features_config_file=ssp_ctr_features_config_file,
    train_dir=work_dir / 'catboost_info' / 'common-ssp-ctr',
    loss_function='CrossEntropy',
    include_ctr_thresholds=False)
  with tempfile.TemporaryDirectory(
      dir=str(work_dir),
      prefix='ctr-model-cycle.') as cycle_dir_name:
    cycle_dir = pathlib.Path(cycle_dir_name)
    selection_validation_files, _ = prepare_validation_libsvm_sets(
      exporter,
      cycle_dir,
      'selection-validation',
      features_config_file,
      date_from,
      date_to,
      validation_rows,
      config.selection_validation_sets,
      progress=in_progress_model,
      progress_section='prepare',
      progress_prefix='selection_validation')

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
      features_config_file,
      progress=in_progress_model,
      progress_section='prepare',
      progress_prefix='feature_selection')
    selection_fit_start, selection_fit_end = fit_step_callbacks(
      in_progress_model,
      'prepare',
      'feature_selection')
    feature_indexes = trainer.select_feature_indexes_from_chunks(
      selection_svm_chunks,
      selection_validation_files,
      fit_iterations=config.fit_iterations,
      work_dir=cycle_dir,
      fit_steps=config.selection_fit_steps,
      on_fit_start=selection_fit_start,
      on_fit_end=selection_fit_end)
    with in_progress_model.train_step('prepare', 'save_feature_indexes'):
      with feature_indexes_file.open('w') as output_file:
        for feature_index in sorted(feature_indexes):
          output_file.write(str(feature_index) + '\n')
    logger.info('Selected %d LibSVM feature indexes', len(feature_indexes))
    with in_progress_model.train_step(
        'prepare', 'release_selection_validation'):
      remove_files(selection_validation_files)
    in_progress_model.complete_prepare()

    core_validation_sets = (
      config.training_validation_sets + config.final_test_sets)
    core_validation_offset_rows = (
      config.selection_validation_sets * validation_rows)
    common_train_start = in_progress_model.start_models('common')
    common_validation_files, common_validation_statistics = (
      prepare_validation_libsvm_sets(
        exporter,
        cycle_dir,
        'common-validation',
        features_config_file,
        date_from,
        date_to,
        validation_rows,
        core_validation_sets,
        validation_offset_rows=core_validation_offset_rows,
        feature_indexes_file=feature_indexes_file,
        collect_statistics=True,
        progress=in_progress_model,
        progress_section='common',
        progress_prefix='common_validation'))
    training_validation_end = config.training_validation_sets
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
      feature_statistics,
      progress=in_progress_model,
      progress_section='common',
      progress_prefix='training')
    common_fit_start, common_fit_end = fit_step_callbacks(
      in_progress_model,
      'common',
      'training')
    common_model, common_logloss_history, common_ctr_thresholds = (
      trainer.train_from_chunks(
        training_svm_chunks,
        common_validation_files[:training_validation_end],
        common_validation_files[training_validation_end:],
        fit_iterations=config.fit_iterations,
        patience=config.training_patience,
        work_dir=cycle_dir,
        fit_steps=config.training_fit_steps,
        on_fit_start=common_fit_start,
        on_fit_end=common_fit_end))
    common_dataset_sizes = {
      'train': dataset_size([feature_statistics]),
      'test': dataset_size(
        common_validation_statistics[:training_validation_end]),
      'final_test': dataset_size(
        common_validation_statistics[training_validation_end:]),
    }

    with in_progress_model.train_step('common', 'save_feature_dictionary'):
      with common_dictionary_file.open('wb') as output_file:
        for line in sorted(dictionary_lines):
          output_file.write(line)

    common_model_file = cycle_dir / 'common.cbm'
    with in_progress_model.train_step('common', 'save_model'):
      common_model.save_model(str(common_model_file))
    with in_progress_model.train_step('common', 'finalize_metrics'):
      common_dataset_sizes = dict(common_dataset_sizes)
      common_model_description = trainer.describe_model(
        common_model,
        common_dictionary_file,
        feature_statistics,
        feature_name_resolver)
      common_properties = final_model_properties(common_logloss_history)
    common_train_end = in_progress_model.complete_model(
      'common',
      file='common.cbm',
      **common_model_description,
      logloss_history=common_logloss_history,
      properties=common_properties,
      dataset_sizes=common_dataset_sizes,
      ctr_thresholds=common_ctr_thresholds)

    stable_dictionary_lines = set()
    correction_dictionary_lines = set()
    stable_statistics = FeatureStatistics()
    correction_statistics = FeatureStatistics()
    aligned_train_start = in_progress_model.start_models(
      'common_denoise',
      'common_stable')
    correction_validation_inputs, correction_validation_statistics = (
      prepare_denoise_validation_sets(
        exporter,
        cycle_dir,
        correction_features_config_file,
        common_model_file,
        trainer,
        common_validation_files,
        date_from,
        date_to,
        validation_rows,
        core_validation_sets,
        core_validation_offset_rows,
        in_progress_model))
    correction_training_inputs = correction_validation_inputs[
      :training_validation_end]
    correction_final_inputs = correction_validation_inputs[
      training_validation_end:]
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
      correction_statistics,
      in_progress_model)
    aligned_fit_start, aligned_fit_end = fit_step_callbacks(
      in_progress_model,
      ('common_denoise', 'common_stable'),
      'aligned_training')
    aligned_models = trainer.train_aligned_from_chunks(
      aligned_chunks,
      correction_trainer,
      correction_training_inputs,
      common_validation_files[:training_validation_end],
      correction_final_inputs,
      common_validation_files[training_validation_end:],
      fit_iterations=config.fit_iterations,
      patience=config.training_patience,
      work_dir=cycle_dir,
      fit_steps=config.training_fit_steps,
      on_fit_start=aligned_fit_start,
      on_fit_end=aligned_fit_end)

    with in_progress_model.train_step(
        ('common_denoise', 'common_stable'),
        'save_feature_dictionary'):
      with stable_dictionary_file.open('wb') as output_file:
        for line in sorted(stable_dictionary_lines):
          output_file.write(line)
      with correction_dictionary_file.open('wb') as output_file:
        for line in sorted(correction_dictionary_lines):
          output_file.write(line)

    stable_dataset_sizes = {
      'train': dataset_size([stable_statistics]),
      'test': dataset_size(
        common_validation_statistics[:training_validation_end]),
      'final_test': dataset_size(
        common_validation_statistics[training_validation_end:]),
    }
    correction_dataset_sizes = {
      'train': dataset_size([correction_statistics]),
      'test': dataset_size(
        correction_validation_statistics[:training_validation_end]),
      'final_test': dataset_size(
        correction_validation_statistics[training_validation_end:]),
    }

    stable_model_file = cycle_dir / 'common-stable.cbm'
    with in_progress_model.train_step(
        ('common_denoise', 'common_stable'),
        'save_model'):
      aligned_models['stable_common']['model'].save_model(
        str(stable_model_file))
    with in_progress_model.train_step(
        ('common_denoise', 'common_stable'),
        'finalize_metrics'):
      stable_dataset_sizes = dict(stable_dataset_sizes)
      correction_dataset_sizes = dict(correction_dataset_sizes)
      correction_model_description = correction_trainer.describe_model(
        aligned_models['campaign_correction']['model'],
        correction_dictionary_file,
        correction_statistics,
        feature_name_resolver)
      stable_model_description = trainer.describe_model(
        aligned_models['stable_common']['model'],
        stable_dictionary_file,
        stable_statistics,
        feature_name_resolver)
      correction_properties = final_model_properties(
        aligned_models['campaign_correction']['logloss_history'])
      stable_properties = final_model_properties(
        aligned_models['stable_common']['logloss_history'])
    with in_progress_model.train_step(
        ('common_denoise', 'common_stable'),
        'release_validation'):
      remove_training_inputs(correction_validation_inputs)
      correction_validation_inputs = []
      remove_files(common_validation_files)
      common_validation_files = []
    correction_train_end = in_progress_model.complete_model(
      'common_denoise',
      file='common_denoise.cbm',
      **correction_model_description,
      logloss_history=aligned_models[
        'campaign_correction']['logloss_history'],
      properties=correction_properties,
      dataset_sizes=correction_dataset_sizes,
      ctr_thresholds=aligned_models[
        'campaign_correction']['ctr_thresholds'])
    stable_train_end = in_progress_model.complete_model(
      'common_stable',
      file='model.cbm',
      **stable_model_description,
      logloss_history=aligned_models['stable_common']['logloss_history'],
      properties=stable_properties,
      dataset_sizes=stable_dataset_sizes,
      ctr_thresholds=aligned_models['stable_common']['ctr_thresholds'])

    ssp_ctr_train_start = in_progress_model.start_models('common_ssp_ctr')
    ssp_ctr_selection_validation_files, _ = prepare_validation_libsvm_sets(
      exporter,
      cycle_dir,
      'ssp-ctr-selection-validation',
      ssp_ctr_features_config_file,
      date_from,
      date_to,
      ssp_ctr_validation_rows,
      config.selection_validation_sets,
      progress=in_progress_model,
      progress_section='common_ssp_ctr',
      progress_prefix='ssp_selection_validation',
      condition=ssp_ctr_condition,
      label='ssp_ctr')
    ssp_ctr_selection_csv_chunks = exporter.export_partitioned_chunks(
      cycle_dir,
      'ssp-ctr-selection',
      ssp_ctr_selection_rows,
      config.selection_chunk_rows,
      ssp_ctr_selection_fit_steps,
      date_from,
      date_to,
      ssp_ctr_condition,
      label='ssp_ctr')
    ssp_ctr_selection_svm_chunks = stream_libsvm_chunks(
      ssp_ctr_selection_csv_chunks,
      cycle_dir,
      'ssp-ctr-selection',
      ssp_ctr_features_config_file,
      progress=in_progress_model,
      progress_section='common_ssp_ctr',
      progress_prefix='ssp_feature_selection')
    ssp_ctr_selection_fit_start, ssp_ctr_selection_fit_end = (
      fit_step_callbacks(
        in_progress_model,
        'common_ssp_ctr',
        'ssp_feature_selection'))
    ssp_ctr_selection_fit_iterations = scaled_fit_iterations(
      config.fit_iterations,
      config.selection_fit_steps,
      ssp_ctr_selection_fit_steps)
    ssp_ctr_feature_indexes = ssp_ctr_trainer.select_feature_indexes_from_chunks(
      ssp_ctr_selection_svm_chunks,
      ssp_ctr_selection_validation_files,
      fit_iterations=ssp_ctr_selection_fit_iterations,
      work_dir=cycle_dir,
      fit_steps=ssp_ctr_selection_fit_steps,
      on_fit_start=ssp_ctr_selection_fit_start,
      on_fit_end=ssp_ctr_selection_fit_end)
    with in_progress_model.train_step(
        'common_ssp_ctr', 'save_feature_indexes'):
      with ssp_ctr_feature_indexes_file.open('w') as output_file:
        for feature_index in sorted(ssp_ctr_feature_indexes):
          output_file.write(str(feature_index) + '\n')
    with in_progress_model.train_step(
        'common_ssp_ctr', 'release_selection_validation'):
      remove_files(ssp_ctr_selection_validation_files)
      ssp_ctr_selection_validation_files = []

    ssp_ctr_core_validation_sets = (
      config.training_validation_sets + config.final_test_sets)
    ssp_ctr_core_validation_offset_rows = (
      config.selection_validation_sets * ssp_ctr_validation_rows)
    ssp_ctr_validation_files, _ = prepare_validation_libsvm_sets(
      exporter,
      cycle_dir,
      'ssp-ctr-validation',
      ssp_ctr_features_config_file,
      date_from,
      date_to,
      ssp_ctr_validation_rows,
      ssp_ctr_core_validation_sets,
      validation_offset_rows=ssp_ctr_core_validation_offset_rows,
      feature_indexes_file=ssp_ctr_feature_indexes_file,
      progress=in_progress_model,
      progress_section='common_ssp_ctr',
      progress_prefix='ssp_validation',
      condition=ssp_ctr_condition,
      label='ssp_ctr')
    ssp_ctr_dictionary_lines = set()
    ssp_ctr_training_counter = RowCounter()
    ssp_ctr_training_csv_chunks = exporter.export_partitioned_chunks(
      cycle_dir,
      'ssp-ctr-training',
      ssp_ctr_training_rows,
      config.main_chunk_rows,
      ssp_ctr_training_fit_steps,
      date_from,
      date_to,
      ssp_ctr_condition,
      label='ssp_ctr')
    ssp_ctr_training_svm_chunks = stream_libsvm_chunks(
      ssp_ctr_training_csv_chunks,
      cycle_dir,
      'ssp-ctr-training',
      ssp_ctr_features_config_file,
      ssp_ctr_feature_indexes_file,
      ssp_ctr_dictionary_lines,
      progress=in_progress_model,
      progress_section='common_ssp_ctr',
      progress_prefix='ssp_training',
      row_counter=ssp_ctr_training_counter)
    ssp_ctr_fit_start, ssp_ctr_fit_end = fit_step_callbacks(
      in_progress_model,
      'common_ssp_ctr',
      'ssp_training')
    ssp_ctr_training_fit_iterations = scaled_fit_iterations(
      config.fit_iterations,
      config.training_fit_steps,
      ssp_ctr_training_fit_steps)
    (
      ssp_ctr_model,
      ssp_ctr_logloss_history,
      ssp_ctr_thresholds,
    ) = ssp_ctr_trainer.train_from_chunks(
      ssp_ctr_training_svm_chunks,
      ssp_ctr_validation_files[:config.training_validation_sets],
      ssp_ctr_validation_files[config.training_validation_sets:],
      fit_iterations=ssp_ctr_training_fit_iterations,
      patience=config.training_patience,
      work_dir=cycle_dir,
      fit_steps=ssp_ctr_training_fit_steps,
      on_fit_start=ssp_ctr_fit_start,
      on_fit_end=ssp_ctr_fit_end)
    ssp_ctr_dataset_sizes = {
      'train': {'rows': ssp_ctr_training_counter.rows},
      'test': {
        'rows': ssp_ctr_validation_rows * config.training_validation_sets,
      },
      'final_test': {
        'rows': ssp_ctr_validation_rows * config.final_test_sets,
      },
    }
    with in_progress_model.train_step(
        'common_ssp_ctr', 'save_feature_dictionary'):
      with ssp_ctr_dictionary_file.open('wb') as output_file:
        for line in sorted(ssp_ctr_dictionary_lines):
          output_file.write(line)
    with in_progress_model.train_step(
        'common_ssp_ctr', 'finalize_metrics'):
      ssp_ctr_event_logloss = exporter.ssp_ctr_logloss(
        date_from,
        date_to,
        ssp_ctr_validation_rows * config.final_test_sets,
        exporter.validation_condition(),
        (
          config.selection_validation_sets +
          config.training_validation_sets
        ) * ssp_ctr_validation_rows)
      ssp_ctr_model_description = ssp_ctr_trainer.describe_model(
        ssp_ctr_model,
        ssp_ctr_dictionary_file,
        feature_name_resolver=feature_name_resolver)
      ssp_ctr_properties = final_model_properties(
        ssp_ctr_logloss_history,
        ssp_ctr_logloss=ssp_ctr_event_logloss)
    with in_progress_model.train_step(
        'common_ssp_ctr', 'release_validation'):
      remove_files(ssp_ctr_validation_files)
      ssp_ctr_validation_files = []
    ssp_ctr_train_end = in_progress_model.complete_model(
      'common_ssp_ctr',
      file='common_ssp_ctr.cbm',
      **ssp_ctr_model_description,
      logloss_history=ssp_ctr_logloss_history,
      properties=ssp_ctr_properties,
      dataset_sizes=ssp_ctr_dataset_sizes,
      ctr_thresholds=ssp_ctr_thresholds,
      selection_rows=ssp_ctr_selection_rows,
      selection_models=ssp_ctr_selection_fit_steps,
      selection_model_iterations=ssp_ctr_selection_fit_iterations,
      selected_feature_indexes=len(ssp_ctr_feature_indexes),
      training_rows_limit=ssp_ctr_training_rows,
      training_fit_steps=ssp_ctr_training_fit_steps,
      training_fit_iterations=ssp_ctr_training_fit_iterations)

    campaign_model_entries = []
    for (
        campaign_id,
        eligible_impressions,
        available_campaign_validation_rows,
    ) in eligible_campaigns:
      campaign_condition = exporter.campaign_condition(campaign_id)
      campaign_selection_fit_steps = campaign_fit_steps(
        eligible_impressions,
        config.selection_chunk_rows,
        config.selection_fit_steps)
      campaign_training_fit_steps = campaign_fit_steps(
        eligible_impressions,
        config.main_chunk_rows,
        config.training_fit_steps)
      campaign_selection_fit_iterations = scaled_fit_iterations(
        config.fit_iterations,
        config.selection_fit_steps,
        campaign_selection_fit_steps)
      campaign_training_fit_iterations = scaled_fit_iterations(
        config.fit_iterations,
        config.training_fit_steps,
        campaign_training_fit_steps)
      campaign_selection_rows = min(
        eligible_impressions,
        config.selection_chunk_rows * campaign_selection_fit_steps)
      campaign_training_rows = min(
        eligible_impressions,
        config.main_chunk_rows * campaign_training_fit_steps)
      campaign_validation_rows = min(
        config.validation_set_rows,
        available_campaign_validation_rows // campaign_validation_sets)

      logger.info(
        'Training campaign model %d (%s): selection=%d rows / %d models '
        'x %d iterations, train=%d rows / %d chunks x %d iterations, '
        'validation=%d x %d',
        campaign_id,
        campaign_names.get(campaign_id),
        campaign_selection_rows,
        campaign_selection_fit_steps,
        campaign_selection_fit_iterations,
        campaign_training_rows,
        campaign_training_fit_steps,
        campaign_training_fit_iterations,
        campaign_validation_rows,
        campaign_validation_sets)
      campaign_model_name = 'campaign_' + str(campaign_id)
      campaign_train_start = in_progress_model.start_models(
        campaign_model_name)
      campaign_selection_validation_inputs = []
      campaign_validation_inputs = []
      try:
        campaign_selection_validation_inputs, _ = (
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
            config.selection_validation_sets,
            in_progress_model,
            progress_prefix='campaign_selection_validation'))
        campaign_selection_csv_chunks = exporter.export_partitioned_chunks(
          cycle_dir,
          'campaign-' + str(campaign_id) + '-feature-selection',
          campaign_selection_rows,
          config.selection_chunk_rows,
          campaign_selection_fit_steps,
          date_from,
          date_to,
          campaign_condition)
        campaign_selection_chunks = stream_campaign_chunks(
          campaign_selection_csv_chunks,
          cycle_dir,
          campaign_id,
          features_config_file,
          campaign_features_config_file,
          feature_indexes_file,
          stable_model_file,
          trainer,
          None,
          None,
          in_progress_model,
          progress_prefix='campaign_feature_selection')
        selection_fit_start, selection_fit_end = fit_step_callbacks(
          in_progress_model,
          campaign_model_name,
          'campaign_feature_selection')
        campaign_feature_indexes = (
          campaign_model_trainer.select_feature_indexes_from_chunks(
            campaign_selection_chunks,
            campaign_selection_validation_inputs,
            fit_iterations=campaign_selection_fit_iterations,
            work_dir=cycle_dir,
            fit_steps=campaign_selection_fit_steps,
            on_fit_start=selection_fit_start,
            on_fit_end=selection_fit_end))
        campaign_feature_indexes_file = cycle_dir / (
          'campaign-' + str(campaign_id) + '.feature-indexes')
        with in_progress_model.train_step(
            campaign_model_name,
            'save_feature_indexes'):
          with campaign_feature_indexes_file.open('w') as output_file:
            for feature_index in sorted(campaign_feature_indexes):
              output_file.write(str(feature_index) + '\n')
        logger.info(
          'Campaign %d selected %d LibSVM feature indexes from %d models',
          campaign_id,
          len(campaign_feature_indexes),
          campaign_selection_fit_steps)
        with in_progress_model.train_step(
            campaign_model_name,
            'release_selection_validation'):
          remove_training_inputs(campaign_selection_validation_inputs)
          campaign_selection_validation_inputs = []

        final_campaign_validation_sets = (
          config.training_validation_sets + config.final_test_sets)
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
            final_campaign_validation_sets,
            in_progress_model,
            campaign_feature_indexes_file=campaign_feature_indexes_file,
            validation_offset_rows=(
              config.selection_validation_sets * campaign_validation_rows)))
        campaign_dictionary_lines = set()
        campaign_statistics = FeatureStatistics()
        campaign_csv_chunks = exporter.export_partitioned_chunks(
          cycle_dir,
          'campaign-' + str(campaign_id) + '-training',
          campaign_training_rows,
          config.main_chunk_rows,
          campaign_training_fit_steps,
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
          campaign_statistics,
          in_progress_model,
          campaign_feature_indexes_file=campaign_feature_indexes_file)
        campaign_training_validation_end = config.training_validation_sets
        campaign_fit_start, campaign_fit_end = fit_step_callbacks(
          in_progress_model,
          campaign_model_name,
          'campaign_training')
        campaign_result = campaign_model_trainer.train_residual_from_chunks(
          campaign_chunks,
          campaign_validation_inputs[
            :campaign_training_validation_end],
          campaign_validation_inputs[
            campaign_training_validation_end:],
          fit_iterations=campaign_training_fit_iterations,
          patience=config.training_patience,
          work_dir=cycle_dir,
          fit_steps=campaign_training_fit_steps,
          on_fit_start=campaign_fit_start,
          on_fit_end=campaign_fit_end)

        campaign_dictionary_file = cycle_dir / (
          'campaign-' + str(campaign_id) + '.features')
        with in_progress_model.train_step(
            campaign_model_name,
            'save_feature_dictionary'):
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
        with in_progress_model.train_step(
            campaign_model_name,
            'finalize_metrics'):
          campaign_weight = campaign_result['weight']
          campaign_model_description = campaign_model_trainer.describe_model(
            campaign_result['model'],
            campaign_dictionary_file,
            campaign_statistics,
            feature_name_resolver)
          campaign_properties = final_model_properties(
            campaign_result['logloss_history'])
        campaign_train_end = in_progress_model.complete_model(
          campaign_model_name,
          file=campaign_model_name + '.cbm',
          **campaign_model_description,
          logloss_history=campaign_result['logloss_history'],
          properties=campaign_properties,
          dataset_sizes=campaign_dataset_sizes,
          ctr_thresholds=campaign_result['ctr_thresholds'],
          runtime=campaign_weight > 0,
          baseline_model='common_stable',
          selection_rows=campaign_selection_rows,
          selection_models=campaign_selection_fit_steps,
          selection_model_iterations=campaign_selection_fit_iterations,
          selected_feature_indexes=len(campaign_feature_indexes),
          training_rows_limit=campaign_training_rows,
          training_fit_steps=campaign_training_fit_steps,
          training_fit_iterations=campaign_training_fit_iterations,
          weight=campaign_weight,
          base_logloss=campaign_result['base_logloss'],
          combined_logloss=campaign_result['combined_logloss'])
        campaign_model_entries.append({
          'name': campaign_model_name,
          'trainer': campaign_model_trainer,
          'model': campaign_result['model'],
          'feature_dictionary_file': campaign_dictionary_file,
          'feature_statistics': campaign_statistics,
          'model_description': campaign_model_description,
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
            'selection_rows': campaign_selection_rows,
            'selection_models': campaign_selection_fit_steps,
            'selection_model_iterations': campaign_selection_fit_iterations,
            'selected_feature_indexes': len(campaign_feature_indexes),
            'training_rows_limit': campaign_training_rows,
            'training_fit_steps': campaign_training_fit_steps,
            'training_fit_iterations': campaign_training_fit_iterations,
            'weight': campaign_weight,
            'base_logloss': campaign_result['base_logloss'],
            'combined_logloss': campaign_result['combined_logloss'],
            'properties': campaign_properties,
            'status': 'completed',
            'train_start': campaign_train_start,
            'train_end': campaign_train_end,
            'train_steps': in_progress_model.model_traits(
              campaign_model_name)['train_steps'],
          },
        })
      finally:
        remove_training_inputs(campaign_selection_validation_inputs)
        remove_training_inputs(campaign_validation_inputs)

    model_entries = [
      {
        'name': 'common',
        'trainer': trainer,
        'model': common_model,
        'feature_dictionary_file': common_dictionary_file,
        'feature_statistics': feature_statistics,
        'model_description': common_model_description,
        'logloss_history': common_logloss_history,
        'dataset_sizes': common_dataset_sizes,
        'ctr_thresholds': common_ctr_thresholds,
        'traits': {
          'kind': 'common',
          'runtime': False,
          'metrics_prediction': 'sigmoid(common)',
          'properties': common_properties,
          'status': 'completed',
          'train_start': common_train_start,
          'train_end': common_train_end,
          'train_steps': in_progress_model.model_traits('common')[
            'train_steps'],
        },
      },
      {
        'name': 'common_denoise',
        'trainer': correction_trainer,
        'model': aligned_models['campaign_correction']['model'],
        'feature_dictionary_file': correction_dictionary_file,
        'feature_statistics': correction_statistics,
        'model_description': correction_model_description,
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
          'properties': correction_properties,
          'status': 'completed',
          'train_start': aligned_train_start,
          'train_end': correction_train_end,
          'train_steps': in_progress_model.model_traits('common_denoise')[
            'train_steps'],
        },
      },
      {
        'name': 'common_stable',
        'trainer': trainer,
        'model': aligned_models['stable_common']['model'],
        'feature_dictionary_file': stable_dictionary_file,
        'feature_statistics': stable_statistics,
        'model_description': stable_model_description,
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
          'properties': stable_properties,
          'status': 'completed',
          'train_start': aligned_train_start,
          'train_end': stable_train_end,
          'train_steps': in_progress_model.model_traits('common_stable')[
            'train_steps'],
        },
      },
      {
        'name': 'common_ssp_ctr',
        'trainer': ssp_ctr_trainer,
        'model': ssp_ctr_model,
        'feature_dictionary_file': ssp_ctr_dictionary_file,
        'model_description': ssp_ctr_model_description,
        'logloss_history': ssp_ctr_logloss_history,
        'dataset_sizes': ssp_ctr_dataset_sizes,
        'ctr_thresholds': ssp_ctr_thresholds,
        'traits': {
          'kind': 'common_ssp_ctr',
          'runtime': False,
          'metrics_prediction': 'sigmoid(common_ssp_ctr)',
          'target': 'ssp_ctr',
          'loss_function': 'CrossEntropy',
          'properties': ssp_ctr_properties,
          'selection_rows': ssp_ctr_selection_rows,
          'selection_models': ssp_ctr_selection_fit_steps,
          'selection_model_iterations': ssp_ctr_selection_fit_iterations,
          'selected_feature_indexes': len(ssp_ctr_feature_indexes),
          'training_rows_limit': ssp_ctr_training_rows,
          'training_fit_steps': ssp_ctr_training_fit_steps,
          'training_fit_iterations': ssp_ctr_training_fit_iterations,
          'status': 'completed',
          'train_start': ssp_ctr_train_start,
          'train_end': ssp_ctr_train_end,
          'train_steps': in_progress_model.model_traits('common_ssp_ctr')[
            'train_steps'],
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
      algorithm_id=in_progress_model.model_id,
      feature_name_resolver=feature_name_resolver,
      prepare=in_progress_model.traits['prepare'],
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
