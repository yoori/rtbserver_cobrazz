#!/usr/bin/env python3.12

import argparse
import bisect
import csv
import logging
import math
import pathlib
import signal
import subprocess
import sys
import tempfile
import time

from CTRPredictModelTrainer import (
  InProgressModel,
  RowCounter,
  campaign_fit_steps,
  deduplicate_feature_indexes,
  fit_step_callbacks,
  final_model_properties,
  indexed_train_steps,
  prepare_features_config,
  prepare_validation_libsvm_sets,
  remove_files,
  repeat_partitioned_chunks,
  repeated_step_callbacks,
  scaled_fit_iterations,
  select_uncorrelated_feature_indexes,
  stream_libsvm_chunks,
  train_step,
  utc_now_text,
)
from rtbserver_utils.CatBoostTrainer import CatBoostTrainer
from rtbserver_utils.CTRModelRepository import CTRModelRepository, ModelNotFound
from rtbserver_utils.CTRPredictModelGeneratorConfig import load_config
from rtbserver_utils.PostgresFeatureNameResolver import PostgresFeatureNameResolver
from rtbserver_utils.RImpressionTrainExporter import RImpressionTrainExporter
from rtbserver_utils.SignalInterruptHandler import SignalInterruptHandler


logger = logging.getLogger(__name__)

RESEARCH_SUFFIX = '.SSP-CTR-CHECK'

CTR_THRESHOLD_GOALS = tuple(
  index / 1000.0 for index in range(31))

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


def research_prepare_steps():
  return [
    train_step('prepare_feature_config', 'Prepare feature configuration'),
    train_step('find_date_range', 'Determine source date range'),
    train_step('fit_row_counts', 'Fit row counts to available source data'),
    train_step('count_available_rows', 'Count available SSP CTR rows'),
  ]


def ssp_ctr_threshold_statistics(csv_file):
  bucket_count = len(CTR_THRESHOLD_GOALS) + 1
  impressions = [0] * bucket_count
  clicks = [0.0] * bucket_count
  predicted_ctr_sums = [0.0] * bucket_count
  total_rows = 0
  total_clicks = 0.0
  with pathlib.Path(csv_file).open(newline='') as input_file:
    reader = csv.DictReader(input_file)
    if reader.fieldnames is None:
      raise RuntimeError('SSP CTR export returned no CSV header')
    for field in ('label', 'SSP_CTR'):
      if field not in reader.fieldnames:
        raise RuntimeError("SSP CTR export is missing '" + field + "' column")
    for row in reader:
      ssp_ctr_text = row.get('SSP_CTR')
      if ssp_ctr_text is None or not ssp_ctr_text.strip():
        continue
      try:
        predicted_ctr = float(ssp_ctr_text)
        label = float(row['label'])
      except (KeyError, TypeError, ValueError):
        raise RuntimeError(
          'SSP CTR export contains an invalid label or SSP_CTR') from None
      if not math.isfinite(predicted_ctr) or not math.isfinite(label):
        raise RuntimeError(
          'SSP CTR export contains a non-finite label or SSP_CTR')
      bucket = bisect.bisect_right(CTR_THRESHOLD_GOALS, predicted_ctr)
      impressions[bucket] += 1
      clicks[bucket] += label
      predicted_ctr_sums[bucket] += predicted_ctr
      total_rows += 1
      total_clicks += label

  for index in range(bucket_count - 2, -1, -1):
    impressions[index] += impressions[index + 1]
    clicks[index] += clicks[index + 1]
    predicted_ctr_sums[index] += predicted_ctr_sums[index + 1]
  return {
    'rows': total_rows,
    'clicks': int(round(total_clicks)),
    'ctr_thresholds': [
      {
        'ctr_goal': ctr_goal,
        'impressions': impressions[index + 1],
        'clicks': int(round(clicks[index + 1])),
        'predicted_ctr_sum': predicted_ctr_sums[index + 1],
        'total_impressions': total_rows,
      }
      for index, ctr_goal in enumerate(CTR_THRESHOLD_GOALS)
    ],
  }


def add_ctr_thresholds(aggregate, chunk):
  if aggregate is None:
    aggregate = [
      {
        'ctr_goal': item['ctr_goal'],
        'impressions': 0,
        'clicks': 0,
        'predicted_ctr_sum': 0.0,
        'total_impressions': 0,
      }
      for item in chunk
    ]
  if len(aggregate) != len(chunk):
    raise ValueError('CTR threshold count mismatch')
  for aggregate_item, chunk_item in zip(aggregate, chunk):
    if aggregate_item['ctr_goal'] != chunk_item['ctr_goal']:
      raise ValueError('CTR threshold mismatch')
    aggregate_item['impressions'] += chunk_item['impressions']
    aggregate_item['clicks'] += chunk_item['clicks']
    aggregate_item['predicted_ctr_sum'] += chunk_item['predicted_ctr_sum']
    aggregate_item['total_impressions'] += chunk_item['total_impressions']
  return aggregate


def finalize_ctr_thresholds(aggregate):
  return [
    {
      'ctr_goal': item['ctr_goal'],
      'impressions': item['impressions'],
      'clicks': item['clicks'],
      'share': (
        item['impressions'] * 100 / item['total_impressions']
        if item['total_impressions'] else None),
      'actual_ctr': (
        item['clicks'] / item['impressions']
        if item['impressions'] else None),
      'average_predicted_ctr': (
        item['predicted_ctr_sum'] / item['impressions']
        if item['impressions'] else None),
    }
    for item in aggregate
  ]


def ssp_ctr_train_steps(config, selection_fit_steps, training_fit_steps):
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
        ('thresholds', 'calculate SSP CTR thresholds'),
        ('libsvm', 'build LibSVM'),
        ('fit', 'fit independent model'),
      )),
    train_step(
      'prune_correlated_feature_indexes',
      'Remove highly correlated feature indexes'),
    train_step(
      'deduplicate_feature_indexes',
      'Remove fully duplicated feature indexes'),
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
    train_step('save_model', 'Save Common SSP CTR model'),
    train_step('finalize_metrics', 'Finalize Common SSP CTR metrics'),
    train_step('release_validation', 'Release SSP CTR validation files'),
  ]


def latest_production_model_id(config):
  repository = CTRModelRepository(config.model_root())
  try:
    return repository.latest_model_id()
  except ModelNotFound:
    return None


def generate_model(config):
  parent_model_id = latest_production_model_id(config)
  root_traits = {
    'model_type': 'research',
    'research_type': 'common_ssp_ctr',
  }
  if parent_model_id is not None:
    root_traits['parent_model_id'] = parent_model_id
  with InProgressModel(
      config.research_model_root(),
      prepare_steps=research_prepare_steps(),
      model_suffix=RESEARCH_SUFFIX,
      root_traits=root_traits) as in_progress_model:
    return generate_model_(config, in_progress_model, parent_model_id)


def generate_model_(config, in_progress_model, parent_model_id=None):
  workspace_root = pathlib.Path(config.workspace_root)
  work_dir = workspace_root / 'CTRResearchModelGenerator'
  output_dir = config.research_model_root()
  work_dir.mkdir(parents=True, exist_ok=True)
  with in_progress_model.train_step('prepare', 'prepare_feature_config'):
    features_config_file = prepare_features_config(
      work_dir,
      SSP_CTR_FEATURE_CONFIG,
      'CTRGeneratorCommonSSPCTRConfig.json')

  logger.debug('Loading SSP CTR research data from ClickHouse')
  exporter = RImpressionTrainExporter(
    config.clickhouse_conn,
    logger=logger,
    user_navigation_sampling=config.user_navigation_sampling)
  validation_sets = (
    config.selection_validation_sets +
    config.training_validation_sets +
    config.final_test_sets)
  selection_rows_limit = (
    config.selection_chunk_rows * config.selection_fit_steps)
  training_rows_limit = config.main_chunk_rows * config.training_fit_steps
  validation_rows_total = config.validation_set_rows * validation_sets
  source_rows = exporter.required_source_rows(
    max(selection_rows_limit, training_rows_limit),
    validation_rows_total)
  with in_progress_model.train_step('prepare', 'find_date_range'):
    date_from, date_to = exporter.find_date_range(
      source_rows,
      config.data_delay)
  ssp_ctr_condition = exporter.ssp_ctr_condition()
  with in_progress_model.train_step('prepare', 'fit_row_counts'):
    available_source_rows = exporter.count_rows(
      date_from,
      date_to,
      ssp_ctr_condition)
  _, validation_window_rows = exporter.fit_row_counts(
    max(selection_rows_limit, training_rows_limit),
    validation_rows_total,
    available_source_rows)
  validation_window_rows -= validation_window_rows % validation_sets
  if validation_window_rows == 0:
    raise RuntimeError('Not enough rows to create SSP CTR validation sets')

  validation_condition = (
    '(' + exporter.validation_condition() + ') AND (' +
    ssp_ctr_condition + ')')
  validation_cutoff = exporter.ordered_slice_min_timestamp(
    date_from,
    date_to,
    validation_window_rows,
    validation_condition)
  training_time_condition = "timestamp < '" + validation_cutoff + "'"
  validation_time_condition = "timestamp >= '" + validation_cutoff + "'"
  with in_progress_model.train_step('prepare', 'count_available_rows'):
    available_training_rows = exporter.count_rows(
      date_from,
      date_to,
      '(' + exporter.training_condition() + ') AND (' +
      ssp_ctr_condition + ') AND (' + training_time_condition + ')')
    available_validation_rows = exporter.count_rows(
      date_from,
      date_to,
      validation_condition + ' AND (' + validation_time_condition + ')')
  if available_training_rows == 0:
    raise RuntimeError('No SSP CTR rows are available for training')

  selection_fit_steps = campaign_fit_steps(
    available_training_rows,
    config.selection_chunk_rows,
    config.selection_fit_steps)
  training_source_steps = campaign_fit_steps(
    available_training_rows,
    config.main_chunk_rows,
    config.training_fit_steps)
  training_fit_steps = config.training_fit_steps
  selection_fit_iterations = scaled_fit_iterations(
    config.fit_iterations,
    config.selection_fit_steps,
    selection_fit_steps)
  validation_rows = min(
    config.validation_set_rows,
    available_validation_rows // validation_sets)
  if validation_rows == 0:
    raise RuntimeError('Not enough SSP CTR rows to create validation sets')
  selection_rows = min(
    available_training_rows,
    config.selection_chunk_rows * selection_fit_steps)
  training_rows = min(
    available_training_rows,
    config.main_chunk_rows * training_source_steps)

  in_progress_model.publish_model_plan([{
    'name': 'common_ssp_ctr',
    'kind': 'common_ssp_ctr',
    'runtime': False,
    'status': 'planned',
    'eligible_training_impressions': available_training_rows,
    'validation_impressions': available_validation_rows,
    'train_steps': ssp_ctr_train_steps(
      config,
      selection_fit_steps,
      training_fit_steps),
  }], date_from=date_from, date_to=date_to, parent_model_id=parent_model_id)
  in_progress_model.complete_prepare(
    date_from=date_from,
    date_to=date_to,
    training_cutoff=validation_cutoff,
    dataset_sizes={
      'train': {'rows': available_training_rows},
      'validation': {'rows': available_validation_rows},
    })
  logger.info(
    'Using SSP CTR rows: selection=%d, training=%d, validation=%d x %d',
    selection_rows,
    training_rows,
    validation_rows,
    validation_sets)

  trainer = CatBoostTrainer(
    features_config_file=features_config_file,
    train_dir=work_dir / 'catboost_info',
    loss_function='CrossEntropy',
    include_ctr_thresholds=False)
  dictionary_file = work_dir / 'RImpressionTrain.ssp-ctr.features'
  feature_indexes_file = work_dir / 'RImpressionTrain.ssp-ctr.feature-indexes'
  validation_files = []
  with tempfile.TemporaryDirectory(
      dir=str(work_dir),
      prefix='ssp-ctr-research.') as cycle_dir_name:
    cycle_dir = pathlib.Path(cycle_dir_name)
    selection_validation_files, _ = prepare_validation_libsvm_sets(
      exporter,
      cycle_dir,
      'ssp-ctr-selection-validation',
      features_config_file,
      validation_cutoff,
      date_to,
      validation_rows,
      config.selection_validation_sets,
      validation_offset_rows=(
        (config.training_validation_sets + config.final_test_sets) *
        validation_rows),
      progress=in_progress_model,
      progress_section='common_ssp_ctr',
      progress_prefix='ssp_selection_validation',
      condition=ssp_ctr_condition,
      label='ssp_ctr',
      workers=config.ctr_generator_workers)

    selection_thresholds = None
    selection_threshold_rows = 0
    selection_threshold_clicks = 0

    def collect_selection_thresholds(csv_file, unused_row_count):
      nonlocal selection_thresholds
      nonlocal selection_threshold_rows
      nonlocal selection_threshold_clicks
      del unused_row_count
      result = ssp_ctr_threshold_statistics(csv_file)
      selection_threshold_rows += result['rows']
      selection_threshold_clicks += result['clicks']
      selection_thresholds = add_ctr_thresholds(
        selection_thresholds,
        result['ctr_thresholds'])
      in_progress_model.update_prepare(
        ctr_thresholds=finalize_ctr_thresholds(selection_thresholds),
        dataset_sizes={
          'ssp_ctr': {
            'rows': selection_threshold_rows,
            'clicks': selection_threshold_clicks,
          },
        })

    def selection_chunks(selection_step):
      csv_chunks = repeat_partitioned_chunks(
        exporter,
        cycle_dir,
        'ssp-ctr-selection',
        selection_rows,
        config.selection_chunk_rows,
        selection_fit_steps,
        date_from,
        date_to,
        '(' + ssp_ctr_condition + ') AND (' + training_time_condition + ')',
        label='ssp_ctr',
        order='ASC')
      return stream_libsvm_chunks(
        csv_chunks,
        cycle_dir,
        'ssp-ctr-selection',
        features_config_file,
        progress=in_progress_model,
        progress_section='common_ssp_ctr',
        progress_prefix='ssp_feature_selection',
        csv_chunk_callback=(
          collect_selection_thresholds if selection_step == 1 else None),
        workers=config.ctr_generator_workers)

    in_progress_model.start_models('common_ssp_ctr')
    selection_fit_start, selection_fit_end = fit_step_callbacks(
      in_progress_model,
      'common_ssp_ctr',
      'ssp_feature_selection')
    correlation_start, correlation_end = repeated_step_callbacks(
      in_progress_model,
      'common_ssp_ctr',
      'prune_correlated_feature_indexes')
    feature_indexes = select_uncorrelated_feature_indexes(
      trainer,
      selection_chunks,
      selection_validation_files,
      selection_fit_iterations,
      cycle_dir,
      selection_fit_steps,
      config.max_feature_selection_steps,
      config.feature_correlation_threshold,
      selection_fit_start,
      selection_fit_end,
      correlation_start,
      correlation_end)
    with in_progress_model.train_step(
        'common_ssp_ctr', 'deduplicate_feature_indexes'):
      feature_indexes = deduplicate_feature_indexes(
        selection_validation_files,
        feature_indexes,
        cycle_dir,
        dropped_features_file=cycle_dir / 'ssp-ctr.feature-indexes.dropped')
    with in_progress_model.train_step(
        'common_ssp_ctr', 'save_feature_indexes'):
      with feature_indexes_file.open('w') as output_file:
        for feature_index in sorted(feature_indexes):
          output_file.write(str(feature_index) + '\n')
    with in_progress_model.train_step(
        'common_ssp_ctr', 'release_selection_validation'):
      remove_files(selection_validation_files)

    core_validation_sets = (
      config.training_validation_sets + config.final_test_sets)
    validation_files, _ = prepare_validation_libsvm_sets(
      exporter,
      cycle_dir,
      'ssp-ctr-validation',
      features_config_file,
      validation_cutoff,
      date_to,
      validation_rows,
      core_validation_sets,
      feature_indexes_file=feature_indexes_file,
      progress=in_progress_model,
      progress_section='common_ssp_ctr',
      progress_prefix='ssp_validation',
      condition=ssp_ctr_condition,
      label='ssp_ctr',
      workers=config.ctr_generator_workers)
    dictionary_lines = set()
    training_counter = RowCounter()
    training_csv_chunks = repeat_partitioned_chunks(
      exporter,
      cycle_dir,
      'ssp-ctr-training',
      training_rows,
      config.main_chunk_rows,
      training_source_steps,
      date_from,
      date_to,
      '(' + ssp_ctr_condition + ') AND (' + training_time_condition + ')',
      label='ssp_ctr',
      order='ASC')
    training_svm_chunks = stream_libsvm_chunks(
      training_csv_chunks,
      cycle_dir,
      'ssp-ctr-training',
      features_config_file,
      feature_indexes_file,
      dictionary_lines,
      progress=in_progress_model,
      progress_section='common_ssp_ctr',
      progress_prefix='ssp_training',
      row_counter=training_counter,
      workers=config.ctr_generator_workers)
    fit_start, fit_end = fit_step_callbacks(
      in_progress_model,
      'common_ssp_ctr',
      'ssp_training')
    model, logloss_history, ctr_thresholds = trainer.train_from_chunks(
      training_svm_chunks,
      validation_files[config.final_test_sets:],
      validation_files[:config.final_test_sets],
      fit_iterations=config.fit_iterations,
      patience=config.training_patience,
      work_dir=cycle_dir,
      fit_steps=training_fit_steps,
      on_fit_start=fit_start,
      on_fit_end=fit_end)
    dataset_sizes = {
      'train': {'rows': training_counter.rows},
      'test': {
        'rows': validation_rows * config.training_validation_sets,
      },
      'final_test': {
        'rows': validation_rows * config.final_test_sets,
      },
    }
    with in_progress_model.train_step(
        'common_ssp_ctr', 'save_feature_dictionary'):
      with dictionary_file.open('wb') as output_file:
        for line in sorted(dictionary_lines):
          output_file.write(line)
    with in_progress_model.train_step('common_ssp_ctr', 'save_model'):
      model.save_model(str(cycle_dir / 'common-ssp-ctr.cbm'))
    with in_progress_model.train_step(
        'common_ssp_ctr', 'finalize_metrics'):
      event_logloss = exporter.ssp_ctr_logloss(
        validation_cutoff,
        date_to,
        validation_rows * config.final_test_sets,
        exporter.validation_condition(),
        0)
      prediction_statistics = (
        CatBoostTrainer.collect_feature_prediction_statistics(
          cycle_dir / 'common-ssp-ctr.cbm',
          validation_files[:config.final_test_sets]))
      model_description = trainer.describe_model(
        model,
        dictionary_file,
        feature_name_resolver=PostgresFeatureNameResolver(
          config.postgres_conn),
        feature_prediction_statistics=prediction_statistics)
      properties = final_model_properties(
        logloss_history,
        ssp_ctr_logloss=event_logloss)
    with in_progress_model.train_step(
        'common_ssp_ctr', 'release_validation'):
      remove_files(validation_files)
      validation_files = []
    train_end = in_progress_model.complete_model(
      'common_ssp_ctr',
      file='model.cbm',
      metrics_prediction='sigmoid(common_ssp_ctr)',
      target='ssp_ctr',
      loss_function='CrossEntropy',
      **model_description,
      logloss_history=logloss_history,
      properties=properties,
      dataset_sizes=dataset_sizes,
      ctr_thresholds=ctr_thresholds,
      selected_feature_indexes=len(feature_indexes),
      selection_rows=selection_rows,
      selection_models=selection_fit_steps,
      selection_model_iterations=selection_fit_iterations,
      training_rows_limit=training_rows,
      training_fit_steps=training_fit_steps,
      training_fit_iterations=config.fit_iterations)

    extra_traits = {
      'traits_version': 2,
      'status': 'published',
      'model_type': 'research',
      'research_type': 'common_ssp_ctr',
      'kind': 'common_ssp_ctr',
      'name': 'common_ssp_ctr',
      'target': 'ssp_ctr',
      'loss_function': 'CrossEntropy',
      'runtime': False,
      'training_pipeline': {
        'published_model': 'common_ssp_ctr',
      },
      'models': list(in_progress_model.traits.get('models', [])),
      'prepare': dict(in_progress_model.traits['prepare']),
      'properties': properties,
      'selection_rows': selection_rows,
      'selection_models': selection_fit_steps,
      'selection_model_iterations': selection_fit_iterations,
      'selected_feature_indexes': len(feature_indexes),
      'training_rows_limit': training_rows,
      'training_fit_steps': training_fit_steps,
      'training_fit_iterations': config.fit_iterations,
      'train_steps': in_progress_model.model_traits(
        'common_ssp_ctr')['train_steps'],
      'date_from': date_from,
      'date_to': date_to,
      'training_cutoff': validation_cutoff,
    }
    if parent_model_id is not None:
      extra_traits['parent_model_id'] = parent_model_id
    result_dir = trainer.save_campaign_manager_model(
      model,
      output_dir,
      timestamp=in_progress_model.model_id,
      staging_dir=in_progress_model.path,
      algorithm_id='ssp_ctr_check',
      feature_dictionary_file=dictionary_file,
      logloss_history=logloss_history,
      dataset_sizes=dataset_sizes,
      ctr_thresholds=ctr_thresholds,
      train_start=in_progress_model.train_start_text,
      train_end=train_end,
      extra_traits=extra_traits,
      model_description=model_description)
    logger.info('Generated SSP CTR research model in %s', result_dir)
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
        logger.info('Starting CTR research model generation')
        generate_model(config)
        logger.info('CTR research model generation completed')
      except Exception:
        logger.exception('CTR research model generation failed')
      wait_for_period(interrupter, config.generate_period)


def main():
  parser = argparse.ArgumentParser(description='CTR research model trainer.')
  parser.add_argument('--config', required=True, help='JSON configuration file.')
  parser.add_argument('--run-once', action='store_true')
  args = parser.parse_args()
  run_service(load_config(args.config), args.run_once)


if __name__ == '__main__':
  logging.basicConfig(
    level='DEBUG',
    format='%(asctime)s ResearchTrainer[%(process)d] %(levelname)s %(message)s')
  try:
    main()
  except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError):
    logger.exception('CTR research model trainer failed')
    sys.exit(1)
