#!/usr/bin/env python3.12

import argparse
import contextlib
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


def campaign_train_steps(config):
  validation_sets = config.training_validation_sets + config.final_test_sets
  return [
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
      config.training_fit_steps,
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
):
  validation_files = []
  validation_statistics = []
  feature_stats_file = None
  completed = False
  csv_chunks = exporter.export_chunks(
    work_dir,
    file_prefix + '-source',
    validation_rows * validation_sets,
    validation_rows,
    date_from,
    date_to,
    exporter.validation_condition(),
    offset_rows=validation_offset_rows)
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
    csv_iterator = iter(csv_chunks)
    for index in range(validation_sets):
      step_number = str(index + 1).zfill(3)
      try:
        context = (
          progress.train_step(
            'campaign_' + str(campaign_id),
            'campaign_validation_export_' + step_number)
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
        'campaign-' + str(campaign_id) + '-validation-' +
        str(index).zfill(3))
      stable_svm = work_dir / (prefix + '-stable.libsvm')
      campaign_svm = work_dir / (prefix + '.libsvm')
      baseline_file = work_dir / (prefix + '.baseline')
      stats_file = work_dir / (prefix + '.stats')
      context = (
        progress.train_step(
          'campaign_' + str(campaign_id),
          'campaign_validation_inputs_' + step_number)
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
    progress=None,
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
            'campaign_training_export_' + step_number)
          if progress is not None else contextlib.nullcontext())
        with context:
          csv_file, row_count = next(csv_iterator)
      except StopIteration:
        if progress is not None:
          progress.cancel_train_step(
            'campaign_' + str(campaign_id),
            'campaign_training_export_' + step_number)
        break
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
      context = (
        progress.train_step(
          'campaign_' + str(campaign_id),
          'campaign_training_inputs_' + step_number)
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
  with in_progress_model.train_step('prepare', 'resolve_campaign_names'):
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
      *[
        {
          'name': 'campaign_' + str(campaign_id),
          'kind': 'campaign',
          'runtime': True,
          'status': 'planned',
          'db_campaign_id': campaign_id,
          'runtime_campaign_group_id': campaign_id,
          'campaign_name': campaign_names.get(campaign_id),
          'train_steps': campaign_train_steps(config),
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
  with in_progress_model.train_step('prepare', 'count_available_rows'):
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
      patience=config.selection_patience,
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
    common_train_end = in_progress_model.complete_models('common')

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
    with in_progress_model.train_step(
        ('common_denoise', 'common_stable'),
        'release_validation'):
      remove_training_inputs(correction_validation_inputs)
      correction_validation_inputs = []
      remove_files(common_validation_files)
      common_validation_files = []
    aligned_train_end = in_progress_model.complete_models(
      'common_denoise',
      'common_stable')
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
            campaign_validation_sets,
            in_progress_model))
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
          campaign_statistics,
          in_progress_model)
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
          fit_iterations=config.fit_iterations,
          patience=config.training_patience,
          work_dir=cycle_dir,
          fit_steps=config.training_fit_steps,
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
            'train_steps': in_progress_model.model_traits(
              campaign_model_name)['train_steps'],
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
          'train_steps': in_progress_model.model_traits('common_stable')[
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
      algorithm_id=config.algorithm_id,
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
