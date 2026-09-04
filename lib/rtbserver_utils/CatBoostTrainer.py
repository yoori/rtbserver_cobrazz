import argparse
import csv
import datetime
import decimal
import itertools
import json
import math
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import numpy
from catboost import CatBoostClassifier, Pool

from rtbserver_utils.CTRModelTraits import traits_with_sections


class CatBoostTrainer(object):
  DICTIONARY_FEATURE_NAMES = {
    'campaignfreq': 'campaign_freq',
    'campaignfreqlog': 'campaign_freq_log',
    'ccg': 'group',
    'channel': 'userch',
    'contcat': 'crcatcont',
    'geochannel': 'geoch',
    'size': 'sizeid',
    'tagviewability': 'viewability',
    'tagvisibility': 'visibility',
    'viscat': 'crcatvis',
  }

  features_size: int = None

  class FeaturePredictionStatistics:
    def __init__(self):
      self.total_rows = 0
      self.total_predicted_ctr = 0.0
      self.features = {}

    def add(self, svm_file, predictions):
      row_count = 0
      with pathlib.Path(svm_file).open() as input_file:
        for line, prediction in zip(input_file, predictions):
          value = float(prediction)
          self.total_predicted_ctr += value
          indexes = set()
          for token in line.split()[1:]:
            index_text = token.split(':', 1)[0]
            index = int(index_text)
            if index <= 0 or index in indexes:
              continue
            indexes.add(index)
            rows, predicted_sum = self.features.get(index, (0, 0.0))
            self.features[index] = (rows + 1, predicted_sum + value)
          row_count += 1
          self.total_rows += 1
      if row_count != len(predictions):
        raise ValueError(
          'Prediction row count does not match SVM file: ' +
          str(len(predictions)) + ' != ' + str(row_count))

    def get(self, index):
      return self.features.get(index, (0, 0.0))

  def __init__(
      self,
      features_dimension=None,
      features_config_file=None,
      train_dir=None,
      loss_function='Logloss',
      include_ctr_thresholds=True,
  ):
    self.features_config_file = features_config_file
    self.train_dir = train_dir
    self.loss_function = loss_function
    self.include_ctr_thresholds = include_ctr_thresholds
    self.peak_rss_bytes = 0
    self.features = None
    if features_config_file is not None:
      config_dimension, self.features = self.read_features_config_(
        features_config_file)
      if (features_dimension is not None and
          features_dimension != config_dimension):
        raise ValueError(
          'Feature dimension mismatch: argument=' +
          str(features_dimension) + ', config=' + str(config_dimension))
      features_dimension = config_dimension

    if features_dimension is None:
      features_dimension = 24
    self.features_size = 1 << features_dimension

  def train_on_pools(
      self,
      train_pool: Pool,
      test_pool: Pool = None,
      iterations=100,
      initial_model=None,
  ):
    # Step 2: Initialize and train the CatBoost model
    model = CatBoostClassifier(
      iterations=iterations,
      learning_rate=0.1, # Step size shrinkage to prevent overfitting
      depth=6,          # Depth of the trees
      loss_function=self.loss_function,
      verbose=0,        # Suppress training output
      train_dir=self.train_dir,
      use_best_model=False,
    )

    print("To fit")
    model.fit(
      train_pool,
      eval_set=test_pool,
      init_model=initial_model,
      verbose=True)
    return model

  def select_feature_indexes(
      self,
      svm_file,
      total_rows,
      chunk_rows,
      validation_rows,
      selection_validation_sets=3,
      training_validation_sets=3,
      final_test_sets=3,
      fit_steps=10,
      fit_iterations=10,
  ):
    validation_sets = (
      selection_validation_sets +
      training_validation_sets +
      final_test_sets)
    with tempfile.TemporaryDirectory(
        dir=str(pathlib.Path(svm_file).resolve().parent),
        prefix='catboost-feature-selection.') as temp_dir:
      chunks, validations = self.split_svm_(
        svm_file,
        total_rows,
        chunk_rows,
        validation_rows,
        validation_sets,
        pathlib.Path(temp_dir),
        chunk_rows * fit_steps)
      return self.select_feature_indexes_from_chunks_(
        chunks,
        validations[:selection_validation_sets],
        fit_iterations,
        pathlib.Path(temp_dir),
        fit_steps)

  def select_feature_indexes_from_chunks(
      self,
      chunks,
      validation_paths,
      fit_iterations=10,
      work_dir=None,
      fit_steps=None,
      on_fit_start=None,
      on_fit_end=None,
      ignored_feature_indexes=None,
  ):
    temp_parent = None if work_dir is None else str(pathlib.Path(work_dir))
    with tempfile.TemporaryDirectory(
        dir=temp_parent,
        prefix='catboost-feature-selection.') as temp_dir:
      return self.select_feature_indexes_from_chunks_(
        chunks,
        validation_paths,
        fit_iterations,
        pathlib.Path(temp_dir),
        fit_steps,
        on_fit_start,
        on_fit_end,
        ignored_feature_indexes)

  def select_feature_indexes_from_chunks_(
      self,
      chunks,
      validation_paths,
      fit_iterations,
      work_dir,
      fit_steps,
      on_fit_start=None,
      on_fit_end=None,
      ignored_feature_indexes=None,
  ):
    if not validation_paths:
      raise ValueError('At least one feature selection validation set is required')
    if fit_iterations <= 0:
      raise ValueError('fit_iterations must be positive')
    if fit_steps is not None and fit_steps <= 0:
      raise ValueError('fit_steps must be positive')
    ignored_feature_indexes = {
      int(index)
      for index in (ignored_feature_indexes or ())
    }
    if any(index <= 0 for index in ignored_feature_indexes):
      raise ValueError('Ignored LibSVM feature indexes must be positive')

    # Selection models are intentionally independent.  A growing boosting
    # model tends to keep using the feature representation found on its first
    # chunks.  Starting from an empty model on every sample exposes
    # alternative useful feature indexes; their union is a conservative
    # whitelist for the final, larger training Pool.
    model_dir = work_dir / 'selection-model'
    model_dir.mkdir()
    selected_indexes = set()
    trained_steps = 0
    chunk_iterator = iter(chunks)
    try:
      chunks_to_fit = chunk_iterator
      if fit_steps is not None:
        chunks_to_fit = itertools.islice(chunk_iterator, fit_steps)
      for step, chunk in enumerate(chunks_to_fit, 1):
        if on_fit_start is not None:
          on_fit_start(step)
        if isinstance(chunk, (tuple, list)):
          svm_file, baseline_file = chunk
        else:
          svm_file = chunk
          baseline_file = None
        model_path = model_dir / (
          'model-' + str(step).zfill(3) + '.cbm')
        train_options = {'baseline_file': baseline_file}
        if ignored_feature_indexes:
          train_options['ignored_feature_indexes'] = ignored_feature_indexes
        train_metrics = self.train_chunk_(
          svm_file,
          model_path,
          fit_iterations,
          **train_options)
        validation_metrics = self.evaluate_model_sets_(
          model_path,
          validation_paths)
        model_indexes = self.model_feature_indexes_(model_path)
        selected_indexes.update(model_indexes)
        trained_steps = step
        print(
          'Feature selection: model=' + str(step) +
          ', train=' + str(train_metrics['Logloss']) +
          ', validation=' + str(validation_metrics['Logloss']) +
          ', model_indexes=' + str(len(model_indexes)) +
          ', selected_indexes=' + str(len(selected_indexes)),
          flush=True)
        if on_fit_end is not None:
          on_fit_end(step)
    finally:
      close = getattr(chunk_iterator, 'close', None)
      if close is not None:
        close()

    if trained_steps == 0:
      raise ValueError('At least one feature selection chunk is required')

    if not selected_indexes:
      raise RuntimeError('Feature selection produced no feature indexes')
    return selected_indexes

  def train_filtered_by_chunks(
      self,
      svm_file,
      total_rows,
      chunk_rows,
      validation_rows,
      selection_validation_sets=3,
      training_validation_sets=3,
      final_test_sets=3,
      fit_iterations=10,
      patience=5,
      fit_steps=30,
  ):
    validation_sets = (
      selection_validation_sets +
      training_validation_sets +
      final_test_sets)
    with tempfile.TemporaryDirectory(
        dir=str(pathlib.Path(svm_file).resolve().parent),
        prefix='catboost-main-training.') as temp_dir:
      chunks, validations = self.split_svm_(
        svm_file,
        total_rows,
        chunk_rows,
        validation_rows,
        validation_sets,
        pathlib.Path(temp_dir),
        chunk_rows * fit_steps)
      training_validation_begin = selection_validation_sets
      training_validation_end = (
        training_validation_begin + training_validation_sets)
      return self.train_from_chunks_(
        chunks,
        validations[training_validation_begin:training_validation_end],
        validations[training_validation_end:],
        fit_iterations,
        patience,
        pathlib.Path(temp_dir),
        fit_steps)

  def train_from_chunks(
      self,
      chunks,
      validation_paths,
      final_test_paths,
      fit_iterations=10,
      patience=5,
      work_dir=None,
      fit_steps=None,
      on_fit_start=None,
      on_fit_end=None,
  ):
    temp_parent = None if work_dir is None else str(pathlib.Path(work_dir))
    with tempfile.TemporaryDirectory(
        dir=temp_parent,
        prefix='catboost-main-training.') as temp_dir:
      return self.train_from_chunks_(
        chunks,
        validation_paths,
        final_test_paths,
        fit_iterations,
        patience,
        pathlib.Path(temp_dir),
        fit_steps,
        on_fit_start,
        on_fit_end)

  def train_from_chunks_(
      self,
      chunks,
      validation_paths,
      final_test_paths,
      fit_iterations,
      patience,
      work_dir,
      fit_steps,
      on_fit_start=None,
      on_fit_end=None,
  ):
    model_dir = work_dir / 'main-model'
    model_dir.mkdir()
    callback_args = {}
    if on_fit_start is not None:
      callback_args['on_fit_start'] = on_fit_start
    if on_fit_end is not None:
      callback_args['on_fit_end'] = on_fit_end
    best_model, best_logloss, trained_steps, logloss_history = self.fit_sequence_(
      chunks,
      validation_paths,
      fit_iterations,
      patience,
      model_dir,
      'Main training',
      fit_steps,
      **callback_args)
    evaluate_final = (
      self.evaluate_model_with_ctr_thresholds_
      if self.include_ctr_thresholds else self.evaluate_model_)
    final_evaluations = [
      evaluate_final(best_model, final_test)
      for final_test in final_test_paths
    ]
    final_metrics = [
      {'Logloss': evaluation['Logloss']}
      for evaluation in final_evaluations
    ]
    ctr_thresholds = (
      self.aggregate_ctr_thresholds_(final_evaluations)
      if self.include_ctr_thresholds else [])
    print(
      'Best main validation Logloss: ' + str(best_logloss) +
      ', trained chunks: ' + str(trained_steps),
      flush=True)
    print(
      'Final test metrics: ' +
      json.dumps(final_metrics, sort_keys=True),
      flush=True)

    model = CatBoostClassifier()
    model.load_model(str(best_model))
    return model, logloss_history, ctr_thresholds

  def train_aligned_from_chunks(
      self,
      chunks,
      correction_trainer,
      correction_validation_inputs,
      stable_validation_paths,
      correction_final_inputs,
      stable_final_paths,
      fit_iterations=10,
      patience=5,
      work_dir=None,
      fit_steps=None,
      on_fit_start=None,
      on_fit_end=None,
  ):
    """Train campaign correction and stable common in one OOF pass.

    Each chunk is ``(stable_svm, correction_svm, common_raw_baseline)``.
    The correction prediction used as stable baseline is produced before the
    correction model sees the current chunk.  With partitioned input this is
    an expanding-window out-of-fold prediction.  CatBoost cannot combine a
    Pool baseline with ``init_model``, so every next tree block is trained as
    a fresh delta over ``previous prediction + external baseline`` and then
    merged into the accumulated model.
    """
    temp_parent = None if work_dir is None else str(pathlib.Path(work_dir))
    with tempfile.TemporaryDirectory(
        dir=temp_parent,
        prefix='catboost-aligned-training.') as temp_dir:
      return self.train_aligned_from_chunks_(
        chunks,
        correction_trainer,
        correction_validation_inputs,
        stable_validation_paths,
        correction_final_inputs,
        stable_final_paths,
        fit_iterations,
        patience,
        pathlib.Path(temp_dir),
        fit_steps,
        on_fit_start,
        on_fit_end)

  def train_aligned_from_chunks_(
      self,
      chunks,
      correction_trainer,
      correction_validation_inputs,
      stable_validation_paths,
      correction_final_inputs,
      stable_final_paths,
      fit_iterations,
      patience,
      work_dir,
      fit_steps,
      on_fit_start=None,
      on_fit_end=None,
  ):
    if not correction_validation_inputs or not stable_validation_paths:
      raise ValueError('At least one validation set is required per component')
    if fit_iterations <= 0:
      raise ValueError('fit_iterations must be positive')
    if patience <= 0:
      raise ValueError('patience must be positive')
    if fit_steps is not None and fit_steps <= 0:
      raise ValueError('fit_steps must be positive')

    correction_dir = work_dir / 'campaign-correction'
    stable_dir = work_dir / 'stable-common'
    correction_dir.mkdir()
    stable_dir.mkdir()
    correction_model = correction_dir / 'model.cbm'
    correction_next = correction_dir / 'next-model.cbm'
    correction_best = correction_dir / 'best-model.cbm'
    stable_model = stable_dir / 'model.cbm'
    stable_next = stable_dir / 'next-model.cbm'
    stable_best = stable_dir / 'best-model.cbm'

    correction_best_logloss = None
    stable_best_logloss = None
    correction_non_improving = 0
    stable_non_improving = 0
    correction_frozen = False
    correction_history = []
    stable_history = []
    trained_steps = 0

    chunk_iterator = iter(chunks)
    try:
      chunks_to_fit = chunk_iterator
      if fit_steps is not None:
        chunks_to_fit = itertools.islice(chunk_iterator, fit_steps)
      for step, chunk in enumerate(chunks_to_fit, 1):
        if on_fit_start is not None:
          on_fit_start(step)
        stable_svm, correction_svm, common_baseline = chunk
        total = '' if fit_steps is None else '/' + str(fit_steps)
        print(
          'Aligned training: fit ' + str(step) + total,
          flush=True)

        temporary_baselines = []
        correction_oof = None
        correction_training_baseline = common_baseline
        if correction_model.exists():
          correction_oof = work_dir / (
            'campaign-correction-oof-' + str(step).zfill(3) + '.baseline')
          temporary_baselines.append(correction_oof)
          if correction_frozen:
            correction_trainer.predict_raw_(
              correction_model,
              correction_svm,
              correction_oof)
          else:
            correction_training_baseline = work_dir / (
              'campaign-correction-total-' +
              str(step).zfill(3) + '.baseline')
            temporary_baselines.append(correction_training_baseline)
            correction_trainer.predict_raw_and_combined_(
              correction_model,
              correction_svm,
              correction_oof,
              correction_training_baseline,
              common_baseline)

        early_stop = False
        try:
          if not correction_frozen:
            correction_train_metrics = correction_trainer.train_chunk_(
              correction_svm,
              correction_next,
              fit_iterations,
              baseline_file=correction_training_baseline,
              merge_model=(
                correction_model if correction_model.exists() else None))
            os.replace(correction_next, correction_model)
            correction_metrics = correction_trainer.evaluate_model_sets_(
              correction_model,
              correction_validation_inputs)
            correction_logloss = correction_metrics['Logloss']
            correction_history.append({
              'step': step,
              'train': correction_train_metrics['Logloss'],
              'test': correction_logloss,
              **({
                'peak_rss_bytes': correction_train_metrics['peak_rss_bytes'],
              } if 'peak_rss_bytes' in correction_train_metrics else {}),
              **({
                'train_rmse': correction_train_metrics['RMSE'],
                'val_rmse': correction_metrics['RMSE'],
                'train_mae': correction_train_metrics['MAE'],
                'val_mae': correction_metrics['MAE'],
              } if (
                  'RMSE' in correction_train_metrics and
                  'RMSE' in correction_metrics and
                  'MAE' in correction_train_metrics and
                  'MAE' in correction_metrics) else {}),
            })
            print(
              'Campaign correction: validation=' +
              json.dumps(correction_metrics, sort_keys=True),
              flush=True)
            if (
                correction_best_logloss is None or
                correction_logloss < correction_best_logloss):
              correction_best_logloss = correction_logloss
              correction_non_improving = 0
              shutil.copyfile(correction_model, correction_best)
            else:
              correction_non_improving += 1
              if correction_non_improving >= patience:
                correction_frozen = True
                shutil.copyfile(correction_best, correction_model)
                print(
                  'Campaign correction: frozen after ' + str(step) +
                  ' fits',
                  flush=True)

          if correction_training_baseline != common_baseline:
            correction_training_baseline.unlink()
            temporary_baselines.remove(correction_training_baseline)

          stable_training_baseline = correction_oof
          if stable_model.exists():
            stable_training_baseline = work_dir / (
              'stable-common-total-' + str(step).zfill(3) + '.baseline')
            temporary_baselines.append(stable_training_baseline)
            self.predict_raw_(
              stable_model,
              stable_svm,
              stable_training_baseline,
              correction_oof)
          stable_train_metrics = self.train_chunk_(
            stable_svm,
            stable_next,
            fit_iterations,
            baseline_file=stable_training_baseline,
            merge_model=stable_model if stable_model.exists() else None)
          os.replace(stable_next, stable_model)
          stable_metrics = self.evaluate_model_sets_(
            stable_model,
            stable_validation_paths)
          stable_logloss = stable_metrics['Logloss']
          stable_history.append({
            'step': step,
            'train': stable_train_metrics['Logloss'],
            'test': stable_logloss,
            **({
              'peak_rss_bytes': stable_train_metrics['peak_rss_bytes'],
            } if 'peak_rss_bytes' in stable_train_metrics else {}),
            **({
              'train_rmse': stable_train_metrics['RMSE'],
              'val_rmse': stable_metrics['RMSE'],
              'train_mae': stable_train_metrics['MAE'],
              'val_mae': stable_metrics['MAE'],
            } if (
                'RMSE' in stable_train_metrics and
                'RMSE' in stable_metrics and
                'MAE' in stable_train_metrics and
                'MAE' in stable_metrics) else {}),
          })
          trained_steps = step
          print(
            'Stable common: validation=' +
            json.dumps(stable_metrics, sort_keys=True),
            flush=True)
          if (
              stable_best_logloss is None or
              stable_logloss < stable_best_logloss):
            stable_best_logloss = stable_logloss
            stable_non_improving = 0
            shutil.copyfile(stable_model, stable_best)
          else:
            stable_non_improving += 1
            if stable_non_improving >= patience:
              print(
                'Stable common: early stop after ' + str(step) + ' fits',
                flush=True)
              early_stop = True
        finally:
          for baseline_file in temporary_baselines:
            try:
              baseline_file.unlink()
            except FileNotFoundError:
              pass
        if on_fit_end is not None:
          on_fit_end(step)
        if early_stop:
          break
    finally:
      close = getattr(chunk_iterator, 'close', None)
      if close is not None:
        close()

    if trained_steps == 0:
      raise ValueError('At least one training chunk is required')

    correction_evaluations = [
      correction_trainer.evaluate_model_with_ctr_thresholds_(
        correction_best,
        svm_file,
        baseline_file)
      for svm_file, baseline_file in correction_final_inputs
    ]
    stable_evaluations = [
      self.evaluate_model_with_ctr_thresholds_(stable_best, svm_file)
      for svm_file in stable_final_paths
    ]
    print(
      'Best campaign correction validation Logloss: ' +
      str(correction_best_logloss) +
      '; best stable common validation Logloss: ' +
      str(stable_best_logloss) +
      '; trained chunks: ' + str(trained_steps),
      flush=True)

    correction_model_object = CatBoostClassifier()
    correction_model_object.load_model(str(correction_best))
    stable_model_object = CatBoostClassifier()
    stable_model_object.load_model(str(stable_best))
    return {
      'campaign_correction': {
        'model': correction_model_object,
        'logloss_history': correction_history,
        'ctr_thresholds': self.aggregate_ctr_thresholds_(
          correction_evaluations),
      },
      'stable_common': {
        'model': stable_model_object,
        'logloss_history': stable_history,
        'ctr_thresholds': self.aggregate_ctr_thresholds_(stable_evaluations),
      },
    }

  def train_residual_from_chunks(
      self,
      chunks,
      validation_inputs,
      final_test_inputs,
      fit_iterations=10,
      patience=5,
      work_dir=None,
      fit_steps=None,
      on_fit_start=None,
      on_fit_end=None,
  ):
    temp_parent = None if work_dir is None else str(pathlib.Path(work_dir))
    with tempfile.TemporaryDirectory(
        dir=temp_parent,
        prefix='catboost-residual-training.') as temp_dir:
      return self.train_residual_from_chunks_(
        chunks,
        validation_inputs,
        final_test_inputs,
        fit_iterations,
        patience,
        pathlib.Path(temp_dir),
        fit_steps,
        on_fit_start,
        on_fit_end)

  def train_residual_from_chunks_(
      self,
      chunks,
      validation_inputs,
      final_test_inputs,
      fit_iterations,
      patience,
      work_dir,
      fit_steps,
      on_fit_start=None,
      on_fit_end=None,
  ):
    if not validation_inputs:
      raise ValueError('At least one residual validation set is required')
    if fit_iterations <= 0:
      raise ValueError('fit_iterations must be positive')
    if patience <= 0:
      raise ValueError('patience must be positive')
    if fit_steps is not None and fit_steps <= 0:
      raise ValueError('fit_steps must be positive')

    model_path = work_dir / 'model.cbm'
    next_model_path = work_dir / 'next-model.cbm'
    best_model_path = work_dir / 'best-model.cbm'
    best_logloss = None
    non_improving_steps = 0
    trained_steps = 0
    history = []

    chunk_iterator = iter(chunks)
    try:
      chunks_to_fit = chunk_iterator
      if fit_steps is not None:
        chunks_to_fit = itertools.islice(chunk_iterator, fit_steps)
      for step, (svm_file, external_baseline) in enumerate(chunks_to_fit, 1):
        if on_fit_start is not None:
          on_fit_start(step)
        training_baseline = external_baseline
        if model_path.exists():
          training_baseline = work_dir / (
            'combined-' + str(step).zfill(3) + '.baseline')
          self.predict_raw_(
            model_path,
            svm_file,
            training_baseline,
            external_baseline)
        try:
          train_metrics = self.train_chunk_(
            svm_file,
            next_model_path,
            fit_iterations,
            baseline_file=training_baseline,
            merge_model=model_path if model_path.exists() else None)
          os.replace(next_model_path, model_path)
        finally:
          if training_baseline != external_baseline:
            try:
              training_baseline.unlink()
            except FileNotFoundError:
              pass

        metrics = self.evaluate_model_sets_(model_path, validation_inputs)
        logloss = metrics['Logloss']
        history.append({
          'step': step,
          'train': train_metrics['Logloss'],
          'test': logloss,
          **({
            'peak_rss_bytes': train_metrics['peak_rss_bytes'],
          } if 'peak_rss_bytes' in train_metrics else {}),
          **({
            'train_rmse': train_metrics['RMSE'],
            'val_rmse': metrics['RMSE'],
            'train_mae': train_metrics['MAE'],
            'val_mae': metrics['MAE'],
          } if (
              'RMSE' in train_metrics and
              'RMSE' in metrics and
              'MAE' in train_metrics and
              'MAE' in metrics) else {}),
        })
        trained_steps = step
        print(
          'Campaign residual: validation=' +
          json.dumps(metrics, sort_keys=True),
          flush=True)
        early_stop = False
        if best_logloss is None or logloss < best_logloss:
          best_logloss = logloss
          non_improving_steps = 0
          shutil.copyfile(model_path, best_model_path)
        else:
          non_improving_steps += 1
          if non_improving_steps >= patience:
            print(
              'Campaign residual: early stop after ' + str(step) + ' fits',
              flush=True)
            early_stop = True
        if on_fit_end is not None:
          on_fit_end(step)
        if early_stop:
          break
    finally:
      close = getattr(chunk_iterator, 'close', None)
      if close is not None:
        close()

    if trained_steps == 0:
      raise ValueError('At least one residual training chunk is required')
    weight_metrics = self.optimize_prediction_weight_(
      best_model_path,
      validation_inputs)
    prediction_weight = weight_metrics['weight']
    final_evaluations = [
      self.evaluate_model_with_ctr_thresholds_(
        best_model_path,
        svm_file,
        baseline_file,
        prediction_weight)
      for svm_file, baseline_file in final_test_inputs
    ]
    model = CatBoostClassifier()
    model.load_model(str(best_model_path))
    return {
      'model': model,
      'logloss_history': history,
      'ctr_thresholds': self.aggregate_ctr_thresholds_(final_evaluations),
      'weight': prediction_weight,
      'base_logloss': weight_metrics['base_logloss'],
      'combined_logloss': weight_metrics['combined_logloss'],
    }

  def split_svm_(
      self,
      svm_file,
      total_rows,
      chunk_rows,
      validation_rows,
      validation_sets,
      output_dir,
      max_train_rows=None,
  ):
    if total_rows <= 1:
      raise ValueError('total_rows must be greater than 1')
    if chunk_rows <= 0:
      raise ValueError('chunk_rows must be positive')
    if validation_rows <= 0:
      raise ValueError('validation_rows must be positive')
    if validation_sets <= 0:
      raise ValueError('validation_sets must be positive')

    holdout_rows = validation_rows * validation_sets
    if holdout_rows >= total_rows:
      raise ValueError(
        'Validation and final test rows must be less than total_rows')
    train_rows = total_rows - holdout_rows
    if max_train_rows is not None:
      if max_train_rows <= 0:
        raise ValueError('max_train_rows must be positive')
      train_rows = min(train_rows, max_train_rows)
    chunk_count = math.ceil(train_rows / chunk_rows)
    chunk_paths = [
      output_dir / ('train-' + str(index) + '.libsvm')
      for index in range(chunk_count)
    ]
    validation_paths = [
      output_dir / ('validation-' + str(index) + '.libsvm')
      for index in range(validation_sets)
    ]
    chunk_files = [path.open('w') for path in chunk_paths]
    validation_files = [path.open('w') for path in validation_paths]
    chunk_sizes = [0] * chunk_count
    validation_sizes = [0] * validation_sets

    try:
      holdout_index = 0
      train_index = 0
      actual_rows = 0
      with pathlib.Path(svm_file).resolve().open() as input_file:
        for row_index, line in enumerate(input_file):
          actual_rows = row_index + 1
          next_holdout_count = (
            (row_index + 1) * holdout_rows // total_rows)
          if next_holdout_count > holdout_index:
            validation_index = holdout_index % validation_sets
            self.write_svm_line_(
              validation_files[validation_index],
              line,
              validation_sizes[validation_index] == 0)
            validation_sizes[validation_index] += 1
            holdout_index += 1
          else:
            if train_index >= train_rows:
              continue
            chunk_index = min(train_index // chunk_rows, chunk_count - 1)
            self.write_svm_line_(
              chunk_files[chunk_index],
              line,
              chunk_sizes[chunk_index] == 0)
            chunk_sizes[chunk_index] += 1
            train_index += 1
    finally:
      for file in chunk_files:
        file.close()
      for file in validation_files:
        file.close()

    if actual_rows != total_rows:
      raise ValueError('Actual SVM row count does not match total_rows')
    if any(size != validation_rows for size in validation_sizes):
      raise RuntimeError('Failed to create validation sets of requested size')
    return chunk_paths, validation_paths

  def fit_sequence_(
      self,
      chunk_paths,
      validation_paths,
      fit_iterations,
      patience,
      model_dir,
      description,
      fit_steps=None,
      on_fit_start=None,
      on_fit_end=None,
  ):
    if not validation_paths:
      raise ValueError('At least one validation set is required')
    if fit_iterations <= 0:
      raise ValueError('fit_iterations must be positive')
    if patience <= 0:
      raise ValueError('patience must be positive')
    if fit_steps is not None and fit_steps <= 0:
      raise ValueError('fit_steps must be positive')

    model_path = model_dir / 'model.cbm'
    next_model_path = model_dir / 'next-model.cbm'
    best_model_path = model_dir / 'best-model.cbm'
    best_logloss = None
    non_improving_steps = 0
    trained_steps = 0
    logloss_history = []

    chunk_iterator = iter(chunk_paths)
    try:
      chunks_to_fit = chunk_iterator
      if fit_steps is not None:
        chunks_to_fit = itertools.islice(chunk_iterator, fit_steps)
      for step, chunk_path in enumerate(chunks_to_fit, 1):
        if on_fit_start is not None:
          on_fit_start(step)
        total = '' if fit_steps is None else '/' + str(fit_steps)
        print(
          description + ': fit ' + str(step) + total,
          flush=True)
        train_metrics = self.train_chunk_(
          chunk_path,
          next_model_path,
          fit_iterations,
          model_path if model_path.exists() else None)
        os.replace(next_model_path, model_path)
        trained_steps = step
        metrics = self.evaluate_model_sets_(model_path, validation_paths)
        logloss = metrics['Logloss']
        logloss_history.append({
          'step': step,
          'train': train_metrics['Logloss'],
          'test': logloss,
          **({
            'peak_rss_bytes': train_metrics['peak_rss_bytes'],
          } if 'peak_rss_bytes' in train_metrics else {}),
          **({
            'train_rmse': train_metrics['RMSE'],
            'val_rmse': metrics['RMSE'],
            'train_mae': train_metrics['MAE'],
            'val_mae': metrics['MAE'],
          } if (
              'RMSE' in train_metrics and
              'RMSE' in metrics and
              'MAE' in train_metrics and
              'MAE' in metrics) else {}),
        })
        print(
          description + ': validation=' +
          json.dumps(metrics, sort_keys=True),
          flush=True)

        early_stop = False
        if best_logloss is None or logloss < best_logloss:
          best_logloss = logloss
          non_improving_steps = 0
          shutil.copyfile(model_path, best_model_path)
        else:
          non_improving_steps += 1
          if non_improving_steps >= patience:
            print(
              description + ': early stop after ' + str(step) + ' fits',
              flush=True)
            early_stop = True
        if on_fit_end is not None:
          on_fit_end(step)
        if early_stop:
          break
    finally:
      close = getattr(chunk_iterator, 'close', None)
      if close is not None:
        close()

    if trained_steps == 0:
      raise ValueError('At least one training chunk is required')
    return best_model_path, best_logloss, trained_steps, logloss_history

  def evaluate_model_sets_(self, model_file, svm_files):
    metrics = []
    for item in svm_files:
      if isinstance(item, (tuple, list)):
        svm_file, baseline_file = item
      else:
        svm_file = item
        baseline_file = None
      metrics.append(self.evaluate_model_(
        model_file,
        svm_file,
        baseline_file))
    result = {
      'Logloss': sum(metric['Logloss'] for metric in metrics) / len(metrics),
      'sets': metrics,
    }
    if metrics and all('RMSE' in metric for metric in metrics):
      result['RMSE'] = sum(metric['RMSE'] for metric in metrics) / len(metrics)
    if metrics and all('MAE' in metric for metric in metrics):
      result['MAE'] = sum(metric['MAE'] for metric in metrics) / len(metrics)
    return result

  @staticmethod
  def model_feature_indexes_(model_file):
    result = subprocess.run(
      [
        sys.executable,
        '-m',
        'rtbserver_utils.CatBoostModelFeatures',
        '--model-file',
        str(model_file),
      ],
      check=True,
      capture_output=True,
      text=True)
    return set(json.loads(result.stdout))

  def write_svm_line_(self, output_file, line, add_size_sentinel):
    feature_prefix = ' ' + str(self.features_size) + ':'
    if add_size_sentinel and feature_prefix not in line:
      output_file.write(
        line.rstrip('\r\n') + feature_prefix + '0\n')
    else:
      output_file.write(line)

  def train_chunk_(
      self,
      svm_file,
      output_model,
      iterations,
      initial_model=None,
      baseline_file=None,
      merge_model=None,
      ignored_feature_indexes=None,
  ):
    metrics_file = pathlib.Path(str(output_model) + '.metrics.json')
    command = [
      sys.executable,
      '-m',
      'rtbserver_utils.CatBoostTrainChunk',
      '--svm-file',
      str(svm_file),
      '--output-model',
      str(output_model),
      '--iterations',
      str(iterations),
      '--metrics-file',
      str(metrics_file),
    ]
    if initial_model is not None:
      command.extend(['--initial-model', str(initial_model)])
    if baseline_file is not None:
      command.extend(['--baseline-file', str(baseline_file)])
    if merge_model is not None:
      command.extend(['--merge-model', str(merge_model)])
    for feature_index in sorted(ignored_feature_indexes or ()):
      command.extend(['--ignored-feature-index', str(feature_index)])
    command.extend(['--loss-function', self.loss_function])
    if self.train_dir is not None:
      command.extend(['--train-dir', str(self.train_dir)])
    try:
      subprocess.run(command, check=True)
      with metrics_file.open() as input_file:
        metrics = json.load(input_file)
      peak_rss_bytes = int(metrics.get('peak_rss_bytes', 0))
      self.peak_rss_bytes = max(self.peak_rss_bytes, peak_rss_bytes)
      return metrics
    finally:
      try:
        metrics_file.unlink()
      except FileNotFoundError:
        pass

  @staticmethod
  def evaluate_model_(
      model_file,
      svm_file,
      baseline_file=None,
      prediction_weight=1.0,
  ):
    command = [
      sys.executable,
      '-m',
      'rtbserver_utils.CatBoostModelEvaluator',
      '--model-file',
      str(model_file),
      '--svm-file',
      str(svm_file),
    ]
    if baseline_file is not None:
      command.extend(['--baseline-file', str(baseline_file)])
    if prediction_weight != 1.0:
      command.extend(['--prediction-weight', str(prediction_weight)])
    result = subprocess.run(
      command,
      check=True,
      capture_output=True,
      text=True)
    return json.loads(result.stdout)

  @staticmethod
  def evaluate_model_with_ctr_thresholds_(
      model_file,
      svm_file,
      baseline_file=None,
      prediction_weight=1.0,
  ):
    command = [
      sys.executable,
      '-m',
      'rtbserver_utils.CatBoostModelEvaluator',
      '--model-file',
      str(model_file),
      '--svm-file',
      str(svm_file),
      '--ctr-thresholds',
    ]
    if baseline_file is not None:
      command.extend(['--baseline-file', str(baseline_file)])
    if prediction_weight != 1.0:
      command.extend(['--prediction-weight', str(prediction_weight)])
    result = subprocess.run(
      command,
      check=True,
      capture_output=True,
      text=True)
    return json.loads(result.stdout)

  @staticmethod
  def optimize_prediction_weight_(model_file, validation_inputs):
    weights = [index / 20 for index in range(21)]
    totals = [0.0] * len(weights)
    for svm_file, baseline_file in validation_inputs:
      result = subprocess.run(
        [
          sys.executable,
          '-m',
          'rtbserver_utils.CatBoostModelEvaluator',
          '--model-file',
          str(model_file),
          '--svm-file',
          str(svm_file),
          '--baseline-file',
          str(baseline_file),
          '--prediction-weights',
          ','.join(str(weight) for weight in weights),
        ],
        check=True,
        capture_output=True,
        text=True)
      evaluations = json.loads(result.stdout)['weighted_logloss']
      if len(evaluations) != len(weights):
        raise ValueError('Prediction weight evaluation count mismatch')
      for index, evaluation in enumerate(evaluations):
        if evaluation['weight'] != weights[index]:
          raise ValueError('Prediction weight evaluation mismatch')
        totals[index] += evaluation['Logloss']
    averages = [value / len(validation_inputs) for value in totals]
    best_index = min(range(len(weights)), key=lambda index: averages[index])
    return {
      'weight': weights[best_index],
      'base_logloss': averages[0],
      'combined_logloss': averages[best_index],
    }

  @staticmethod
  def evaluate_prediction_weights_(
      model_file,
      svm_file,
      baseline_file,
      prediction_weights,
  ):
    weights = [float(weight) for weight in prediction_weights]
    if not weights:
      raise ValueError('At least one prediction weight is required')
    result = subprocess.run(
      [
        sys.executable,
        '-m',
        'rtbserver_utils.CatBoostModelEvaluator',
        '--model-file',
        str(model_file),
        '--svm-file',
        str(svm_file),
        '--baseline-file',
        str(baseline_file),
        '--prediction-weights',
        ','.join(str(weight) for weight in weights),
      ],
      check=True,
      capture_output=True,
      text=True)
    evaluations = json.loads(result.stdout).get('weighted_logloss')
    if not isinstance(evaluations, list) or len(evaluations) != len(weights):
      raise ValueError('Prediction weight evaluation count mismatch')
    for index, evaluation in enumerate(evaluations):
      if float(evaluation.get('weight')) != weights[index]:
        raise ValueError('Prediction weight evaluation mismatch')
    return evaluations

  @staticmethod
  def predict_raw_(
      model_file,
      svm_file,
      output_file,
      baseline_file=None,
  ):
    command = [
      sys.executable,
      '-m',
      'rtbserver_utils.CatBoostModelEvaluator',
      '--model-file',
      str(model_file),
      '--svm-file',
      str(svm_file),
      '--raw-predictions-file',
      str(output_file),
    ]
    if baseline_file is not None:
      command.extend(['--baseline-file', str(baseline_file)])
    subprocess.run(
      command,
      check=True,
      capture_output=True,
      text=True)

  @staticmethod
  def predict_raw_and_combined_(
      model_file,
      svm_file,
      model_output_file,
      combined_output_file,
      baseline_file,
  ):
    subprocess.run(
      [
        sys.executable,
        '-m',
        'rtbserver_utils.CatBoostModelEvaluator',
        '--model-file',
        str(model_file),
        '--svm-file',
        str(svm_file),
        '--baseline-file',
        str(baseline_file),
        '--model-raw-predictions-file',
        str(model_output_file),
        '--raw-predictions-file',
        str(combined_output_file),
      ],
      check=True,
      capture_output=True,
      text=True)

  @staticmethod
  def collect_feature_prediction_statistics(
      model_file,
      svm_files,
      prediction_weight=1.0,
  ):
    model = CatBoostClassifier()
    model.load_model(str(pathlib.Path(model_file).resolve()))
    result = CatBoostTrainer.FeaturePredictionStatistics()
    for item in svm_files:
      if isinstance(item, (tuple, list)):
        svm_file, baseline_file = item
      else:
        svm_file, baseline_file = item, None
      svm_path = pathlib.Path(svm_file).resolve()
      pool = Pool('libsvm://' + str(svm_path))
      raw_predictions = numpy.asarray(
        model.predict(pool, prediction_type='RawFormulaVal'),
        dtype=numpy.float64).reshape(-1) * prediction_weight
      if baseline_file is not None:
        baseline = numpy.atleast_1d(numpy.loadtxt(
          pathlib.Path(baseline_file).resolve(),
          dtype=numpy.float64))
        if baseline.shape != raw_predictions.shape:
          raise ValueError(
            'Baseline row count does not match evaluation pool: ' +
            str(baseline.shape[0]) + ' != ' + str(raw_predictions.shape[0]))
        raw_predictions += baseline
      predictions = numpy.empty_like(raw_predictions)
      positive = raw_predictions >= 0
      predictions[positive] = 1 / (1 + numpy.exp(-raw_predictions[positive]))
      negative = ~positive
      exp_predictions = numpy.exp(raw_predictions[negative])
      predictions[negative] = exp_predictions / (1 + exp_predictions)
      result.add(svm_path, predictions)
    return result

  @staticmethod
  def aggregate_ctr_thresholds_(evaluations):
    if not evaluations:
      return []

    result = []
    first_thresholds = evaluations[0]['ctr_thresholds']
    if any(
        len(evaluation['ctr_thresholds']) != len(first_thresholds)
        for evaluation in evaluations):
      raise ValueError('CTR threshold count mismatch between final test sets')
    for index, first in enumerate(first_thresholds):
      ctr_goal = first['ctr_goal']
      impressions = 0
      clicks = 0
      predicted_ctr_sum = 0.0
      total_impressions = 0
      for evaluation in evaluations:
        threshold = evaluation['ctr_thresholds'][index]
        if threshold['ctr_goal'] != ctr_goal:
          raise ValueError('CTR threshold mismatch between final test sets')
        impressions += threshold['impressions']
        clicks += threshold['clicks']
        predicted_ctr_sum += threshold['predicted_ctr_sum']
        total_impressions += threshold['total_impressions']
      result.append({
        'ctr_goal': ctr_goal,
        'impressions': impressions,
        'clicks': clicks,
        'share': (
          impressions * 100 / total_impressions
          if total_impressions else None),
        'actual_ctr': clicks / impressions if impressions else None,
        'average_predicted_ctr': (
          predicted_ctr_sum / impressions if impressions else None),
      })
    return result

  def train(self, train_svm_file, test_svm_file=None):
    train_pool = self.load_pool_(train_svm_file)
    print("Train set: (" + str(train_pool.num_row()) + ", " +
          str(train_pool.num_col()) + ")")
    test_pool = None
    if test_svm_file is not None:
      test_pool = self.load_pool_(test_svm_file)
      print("Test set: (" + str(test_pool.num_row()) + ", " +
            str(test_pool.num_col()) + ")")
    return self.train_on_pools(train_pool, test_pool)

  @staticmethod
  def load_pool_(svm_file):
    svm_path = pathlib.Path(svm_file).resolve()
    return Pool('libsvm://' + str(svm_path))

  def save_campaign_manager_model(
      self,
      model,
      output_dir,
      timestamp=None,
      staging_dir=None,
      algorithm_id='catboost',
      feature_dictionary_file=None,
      feature_name_resolver=None,
      feature_statistics=None,
      logloss_history=None,
      dataset_sizes=None,
      ctr_thresholds=None,
      train_start=None,
      train_end=None,
      extra_traits=None,
      model_description=None,
  ):
    if self.features is None:
      raise ValueError(
        'features_config_file is required to generate a CampaignManager model')

    if timestamp is None:
      timestamp = datetime.datetime.now(datetime.timezone.utc).strftime(
        '%Y%m%d.%H%M%S')

    output_dir = pathlib.Path(output_dir)
    result_dir = output_dir / timestamp
    if result_dir.exists():
      raise FileExistsError("Model directory already exists: '" +
                            str(result_dir) + "'")

    output_dir.mkdir(parents=True, exist_ok=True)
    if staging_dir is None:
      staging_dir = pathlib.Path(tempfile.mkdtemp(
        prefix='~' + timestamp + '.',
        dir=str(output_dir)))
    else:
      staging_dir = pathlib.Path(staging_dir)
      if (
          not staging_dir.is_dir() or
          staging_dir.is_symlink() or
          staging_dir.resolve().parent != output_dir.resolve()):
        raise ValueError("Invalid model staging directory: '" +
                         str(staging_dir) + "'")
    try:
      if model_description is None:
        model_description = self.describe_model(
          model,
          feature_dictionary_file,
          feature_statistics,
          feature_name_resolver)
      model.drop_unused_features()
      model.save_model(str(staging_dir / 'model.cbm'))
      features = model_description['feature_groups']
      features_importance = model_description['features_importance']
      self.write_campaign_manager_config_(
        staging_dir / 'config.json',
        algorithm_id,
        self.features_size,
        features)
      traits_file = staging_dir / 'traits.json'
      traits_temp_file = staging_dir / '.traits.json.tmp'
      self.write_model_traits_(
        traits_temp_file,
        features_importance,
        logloss_history,
        dataset_sizes,
        ctr_thresholds,
        train_start,
        train_end,
        self.peak_rss_bytes)
      if extra_traits:
        with traits_temp_file.open() as input_file:
          traits = json.load(input_file, parse_float=decimal.Decimal)
        traits.update(extra_traits)
        self.write_json_(traits_temp_file, traits)
      os.replace(traits_temp_file, traits_file)
      os.rename(staging_dir, result_dir)
    except Exception:
      shutil.rmtree(staging_dir, ignore_errors=True)
      raise

    return result_dir

  def save_campaign_manager_model_bundle(
      self,
      output_dir,
      models,
      timestamp=None,
      staging_dir=None,
      algorithm_id='catboost',
      feature_name_resolver=None,
      train_start=None,
      train_end=None,
      prepare=None,
      post_processing=None,
  ):
    required_models = (
      'common',
      'common_denoise',
      'common_stable',
    )
    if [model.get('name') for model in models[:3]] != list(required_models):
      raise ValueError(
        'First model entries must be: ' + ', '.join(required_models))
    if len({model.get('name') for model in models}) != len(models):
      raise ValueError('Model names must be unique')
    if timestamp is None:
      timestamp = datetime.datetime.now(datetime.timezone.utc).strftime(
        '%Y%m%d.%H%M%S')

    output_dir = pathlib.Path(output_dir)
    result_dir = output_dir / timestamp
    if result_dir.exists():
      raise FileExistsError(
        "Model directory already exists: '" + str(result_dir) + "'")
    output_dir.mkdir(parents=True, exist_ok=True)
    if staging_dir is None:
      staging_dir = pathlib.Path(tempfile.mkdtemp(
        prefix='~' + timestamp + '.',
        dir=str(output_dir)))
    else:
      staging_dir = pathlib.Path(staging_dir)
      if (
          not staging_dir.is_dir() or
          staging_dir.is_symlink() or
          staging_dir.resolve().parent != output_dir.resolve()):
        raise ValueError(
          "Invalid model staging directory: '" + str(staging_dir) + "'")

    model_files = {
      'common': 'common.cbm',
      'common_denoise': 'common_denoise.cbm',
      'common_stable': 'model.cbm',
    }
    try:
      traits_models_dir = staging_dir / 'traits' / 'models'
      traits_models_dir.mkdir(parents=True, exist_ok=True)
      (staging_dir / 'traits' / 'post_processing').mkdir(
        parents=True,
        exist_ok=True)
      traits_models = []
      unresolved_feature_items = []
      runtime_models = []
      for entry in models:
        name = entry['name']
        if not re.fullmatch(r'[a-z0-9_]+', name):
          raise ValueError("Invalid model name: '" + name + "'")
        file_name = model_files.get(name, name + '.cbm')
        entry_trainer = entry['trainer']
        model = entry['model']
        model_description = entry.get('model_description')
        if model_description is None:
          model_description = entry_trainer.describe_model(
            model,
            entry.get('feature_dictionary_file'),
            entry.get('feature_statistics'))
          unresolved_feature_items.extend(
            model_description['features_importance'])
        model.drop_unused_features()
        model.save_model(str(staging_dir / file_name))
        features = model_description['feature_groups']
        features_importance = model_description['features_importance']
        model_traits = {
          'artifact_version': 2,
          'type': 'model',
          'name': name,
          **entry.get('traits', {}),
          'file': file_name,
          'feature_groups': features,
          'features_importance': features_importance,
          'logloss_history': entry.get('logloss_history', []),
          'dataset_sizes': entry.get('dataset_sizes', {}),
          'ctr_thresholds': entry.get('ctr_thresholds', []),
        }
        peak_rss_bytes = self.peak_rss_from_history_(
          model_traits['logloss_history'])
        if peak_rss_bytes:
          properties = model_traits.get('properties', [])
          if not isinstance(properties, list):
            properties = []
          if not any(
              isinstance(item, dict) and 'peak_rss_bytes' in item
              for item in properties):
            properties = [
              *properties,
              {'peak_rss_bytes': peak_rss_bytes},
            ]
          model_traits['properties'] = properties
        traits_models.append(model_traits)

        campaign_id = model_traits.get('db_campaign_id')
        whitelist_file = None
        if campaign_id is not None:
          whitelist_file = name + '.campaigns'
          with (staging_dir / whitelist_file).open('w') as output:
            output.write(str(int(campaign_id)) + '\n')
        if model_traits.get('runtime'):
          runtime_model = {
            'method': 'catboost',
            'features_size': entry_trainer.features_size,
            'weight': float(model_traits.get('weight', 1.0)),
            'predict_postprocess': 'as_is',
            'features': features,
            'file': file_name,
          }
          if whitelist_file is not None:
            runtime_model['campaigns_whitelist_file'] = whitelist_file
          runtime_models.append(runtime_model)

      if feature_name_resolver is not None and unresolved_feature_items:
        feature_names = feature_name_resolver.resolve(
          list(dict.fromkeys(
            item['feature']
            for item in unresolved_feature_items)))
        for item in unresolved_feature_items:
          name = feature_names.get(item['feature'])
          if name is not None:
            item['name'] = name

      traits_model_summaries = []
      for model_traits in traits_models:
        name = model_traits['name']
        artifact_path = 'traits/models/' + name + '.json'
        owner_step = next((
          step
          for step in reversed(model_traits.get('train_steps', []))
          if isinstance(step, dict) and step.get('id') == 'finalize_metrics'
        ), None)
        model_traits['artifact_owner'] = {
          'section': 'model',
          'name': name,
          'step_id': (
            owner_step.get('id') if owner_step is not None else None),
        }
        if owner_step is not None:
          artifacts = owner_step.setdefault('artifacts', [])
          if not any(
              isinstance(item, dict) and item.get('path') == artifact_path
              for item in artifacts):
            artifacts.append({
              'type': 'model_report',
              'path': artifact_path,
            })
        artifact_file = staging_dir / artifact_path
        artifact_temp_file = artifact_file.parent / (
          '.' + artifact_file.name + '.tmp')
        self.write_json_(
          artifact_temp_file,
          traits_with_sections(model_traits))
        os.replace(artifact_temp_file, artifact_file)
        traits_model_summaries.append(self.traits_manifest_entry_(
          model_traits,
          artifact_path))

      self.write_campaign_manager_models_config_(
        staging_dir / 'config.json',
        algorithm_id,
        runtime_models)
      traits = {
        'traits_version': 2,
        'status': 'published',
        'train_start': train_start,
        'train_end': train_end,
        'training_pipeline': {
          'published_model': 'common_stable',
          'correction_oof_strategy': 'expanding_window_hash_partitions',
        },
        'models': traits_model_summaries,
      }
      if prepare is not None:
        prepare.setdefault('artifact_version', 2)
        prepare.setdefault('type', 'prepare')
        prepare_steps = prepare.get('train_steps', [])
        prepare_owner_step = next((
          step
          for step in reversed(prepare_steps)
          if isinstance(step, dict)
        ), None)
        prepare['artifact_owner'] = {
          'section': 'prepare',
          'step_id': (
            prepare_owner_step.get('id')
            if prepare_owner_step is not None else None),
        }
        if prepare_owner_step is not None:
          prepare_artifacts = prepare_owner_step.setdefault('artifacts', [])
          if not any(
              isinstance(item, dict) and
              item.get('path') == 'traits/prepare.json'
              for item in prepare_artifacts):
            prepare_artifacts.append({
              'type': 'prepare_report',
              'path': 'traits/prepare.json',
            })
        prepare_artifact = staging_dir / 'traits' / 'prepare.json'
        prepare_temp = prepare_artifact.parent / (
          '.' + prepare_artifact.name + '.tmp')
        self.write_json_(prepare_temp, traits_with_sections(prepare))
        os.replace(prepare_temp, prepare_artifact)
        traits['prepare'] = self.traits_manifest_entry_(
          prepare,
          'traits/prepare.json')
      if post_processing is not None:
        post_processing.setdefault('artifact_version', 2)
        post_processing.setdefault('type', 'post_processing')
        post_processing_artifact = (
          staging_dir / 'traits' / 'post_processing' / 'index.json')
        post_processing_temp = post_processing_artifact.parent / (
          '.' + post_processing_artifact.name + '.tmp')
        self.write_json_(
          post_processing_temp,
          traits_with_sections(post_processing))
        os.replace(post_processing_temp, post_processing_artifact)
        traits['post_processing'] = self.traits_manifest_entry_(
          post_processing,
          'traits/post_processing/index.json')
      traits_temp_file = staging_dir / '.traits.json.tmp'
      self.write_json_(traits_temp_file, traits)
      os.replace(traits_temp_file, staging_dir / 'traits.json')
      os.rename(staging_dir, result_dir)
    except Exception:
      shutil.rmtree(staging_dir, ignore_errors=True)
      raise
    return result_dir

  @staticmethod
  def traits_manifest_entry_(traits, artifact):
    artifact_fields = frozenset((
      'ctr_thresholds',
      'dataset_sizes',
      'feature_groups',
      'features_importance',
      'logloss_history',
      'properties',
      'sections',
      'targets',
      'train_steps',
    ))
    result = {
      key: value
      for key, value in traits.items()
      if key not in artifact_fields
    }
    result['artifact'] = artifact
    for field in (
        'feature_groups', 'features_importance', 'targets', 'train_steps'):
      value = traits.get(field)
      if isinstance(value, list):
        result[field + '_count'] = len(value)
    return result

  @staticmethod
  def peak_rss_from_history_(history):
    return max(
      (
        int(item.get('peak_rss_bytes', 0))
        for item in history
        if isinstance(item, dict)
      ),
      default=0)

  def describe_model(
      self,
      model,
      feature_dictionary_file,
      feature_statistics=None,
      feature_name_resolver=None,
      feature_prediction_statistics=None,
  ):
    features, features_importance = self.model_traits_(
      model,
      feature_dictionary_file,
      feature_statistics,
      feature_prediction_statistics)
    if feature_name_resolver is not None and features_importance:
      feature_names = feature_name_resolver.resolve(
        [item['feature'] for item in features_importance])
      for item in features_importance:
        name = feature_names.get(item['feature'])
        if name is not None:
          item['name'] = name
    return {
      'feature_groups': features,
      'features_importance': features_importance,
    }

  def model_traits_(
      self,
      model,
      feature_dictionary_file,
      feature_statistics=None,
      feature_prediction_statistics=None,
  ):
    feature_importance = [
      (index, float(score))
      for index, score in enumerate(model.get_feature_importance())
      if score > 0
    ]
    feature_importance.sort(key=lambda item: (-item[1], item[0]))

    if feature_dictionary_file is None:
      return self.features, []

    dictionary = self.read_feature_dictionary_(feature_dictionary_file)
    configured_features = {
      frozenset(feature): feature
      for feature in self.features
    }
    used_feature_signatures = set()
    traits = []

    for index, score in feature_importance:
      feature_names = dictionary.get(index + 1)
      if not feature_names:
        raise ValueError(
          'Feature dictionary does not contain used SVM index ' +
          str(index + 1))

      for feature_name in feature_names:
        signature = self.feature_signature_(feature_name)
        if signature not in configured_features:
          raise ValueError(
            "Feature '" + feature_name +
            "' from dictionary is absent in feature configuration")
        used_feature_signatures.add(signature)
        trait = {
          'score': score,
          'feature': feature_name,
        }
        if (
            feature_statistics is not None or
            feature_prediction_statistics is not None):
          self.add_feature_statistics_(
            trait,
            feature_statistics,
            index + 1,
            feature_prediction_statistics)
        traits.append(trait)

    features = [
      feature
      for feature in self.features
      if frozenset(feature) in used_feature_signatures
    ]
    return features, traits

  @staticmethod
  def add_feature_statistics_(
      trait,
      feature_statistics,
      feature_index,
      feature_prediction_statistics=None,
  ):
    def ratio(numerator, denominator):
      if denominator == 0:
        return decimal.Decimal(0)
      return decimal.Decimal(str(numerator)) / decimal.Decimal(denominator)

    if feature_statistics is not None:
      total_impressions = feature_statistics.total_impressions
      total_clicks = feature_statistics.total_clicks
      yes_impressions, yes_clicks = feature_statistics.get(feature_index)
      no_impressions = total_impressions - yes_impressions
      no_clicks = total_clicks - yes_clicks
      trait['yes_share'] = ratio(yes_impressions * 100, total_impressions)
      trait['yes_ctr'] = ratio(yes_clicks, yes_impressions)
      trait['no_ctr'] = ratio(no_clicks, no_impressions)

    if feature_prediction_statistics is not None:
      yes_rows, yes_predicted_sum = feature_prediction_statistics.get(
        feature_index)
      no_rows = feature_prediction_statistics.total_rows - yes_rows
      no_predicted_sum = (
        feature_prediction_statistics.total_predicted_ctr -
        yes_predicted_sum)
      trait['yes_predicted_ctr'] = ratio(yes_predicted_sum, yes_rows)
      trait['no_predicted_ctr'] = ratio(no_predicted_sum, no_rows)

  @classmethod
  def feature_signature_(cls, feature_name):
    dictionary_names = re.findall(r'(?:^|,)([a-z_]+):', feature_name)
    if not dictionary_names:
      raise ValueError(
        "Can't extract feature domains from dictionary value '" +
        feature_name + "'")
    return frozenset(
      cls.DICTIONARY_FEATURE_NAMES.get(name, name)
      for name in dictionary_names)

  @staticmethod
  def read_feature_dictionary_(feature_dictionary_file):
    dictionary = {}
    with pathlib.Path(feature_dictionary_file).open(newline='') as input_file:
      for line_number, row in enumerate(csv.reader(input_file), 1):
        if not row:
          continue
        if len(row) != 2:
          raise ValueError(
            'Invalid feature dictionary row ' + str(line_number) + ': ' +
            repr(row))
        try:
          index = int(row[0])
        except ValueError as error:
          raise ValueError(
            'Invalid feature dictionary index at row ' +
            str(line_number) + ": '" + row[0] + "'") from error
        dictionary.setdefault(index, [])
        if row[1] not in dictionary[index]:
          dictionary[index].append(row[1])
    return dictionary

  @staticmethod
  def read_features_config_(features_config_file):
    with pathlib.Path(features_config_file).open() as input_file:
      config = json.load(input_file)

    dimension = config.get('features_dimension')
    if (
        not isinstance(dimension, int) or
        isinstance(dimension, bool) or
        dimension <= 0):
      raise ValueError(
        "Invalid features_dimension in feature config '" +
        str(features_config_file) + "'")

    features = config.get('features')
    if (
        not isinstance(features, list) or
        not features or
        any(
          not isinstance(feature, list) or
          not feature or
          any(not isinstance(name, str) or not name for name in feature)
          for feature in features)):
      raise ValueError(
        "Invalid features in feature config '" +
        str(features_config_file) + "'")

    return dimension, features

  @staticmethod
  def write_campaign_manager_config_(
      config_file,
      algorithm_id,
      features_size,
      features,
  ):
    config = {
      'version': 2,
      'default_weight': 0,
      'algorithms': [
        {
          'id': algorithm_id,
          'weight': 1,
          'threshold': 0,
          'models': [
            {
              'method': 'catboost',
              'features_size': features_size,
              'weight': 1.0,
              'features': features,
              'file': 'model.cbm',
            },
          ],
        },
      ],
    }
    with config_file.open('w') as output:
      json.dump(config, output, indent=2)
      output.write('\n')

  @staticmethod
  def write_campaign_manager_logit_config_(
      config_file,
      algorithm_id,
      features_size,
      features,
  ):
    config = {
      'version': 3,
      'default_weight': 0,
      'algorithms': [
        {
          'id': algorithm_id,
          'weight': 1,
          'threshold': 0,
          'aggregation': 'logit_sum',
          'models': [
            {
              'method': 'catboost',
              'features_size': features_size,
              'weight': 1.0,
              'predict_postprocess': 'as_is',
              'features': features,
              'file': 'model.cbm',
            },
          ],
        },
      ],
    }
    with config_file.open('w') as output:
      json.dump(config, output, indent=2)
      output.write('\n')

  @staticmethod
  def write_campaign_manager_models_config_(
      config_file,
      algorithm_id,
      models,
  ):
    if not models:
      raise ValueError('At least one runtime model is required')
    config = {
      'version': 4,
      'default_weight': 0,
      'algorithms': [
        {
          'id': algorithm_id,
          'weight': 1,
          'threshold': 0,
          'aggregation': 'logit_sum',
          'models': models,
        },
      ],
    }
    with config_file.open('w') as output:
      json.dump(config, output, indent=2)
      output.write('\n')

  @classmethod
  def write_json_(cls, output_file, value):
    with pathlib.Path(output_file).open('w') as output:
      cls.write_json_value_(output, value, 0)
      output.write('\n')

  @classmethod
  def write_json_value_(cls, output, value, indent):
    if isinstance(value, dict):
      if not value:
        output.write('{}')
        return
      output.write('{\n')
      for index, (key, item) in enumerate(value.items()):
        if index:
          output.write(',\n')
        output.write(' ' * (indent + 2))
        output.write(json.dumps(str(key), ensure_ascii=False) + ': ')
        cls.write_json_value_(output, item, indent + 2)
      output.write('\n' + ' ' * indent + '}')
    elif isinstance(value, (list, tuple)):
      if not value:
        output.write('[]')
        return
      output.write('[\n')
      for index, item in enumerate(value):
        if index:
          output.write(',\n')
        output.write(' ' * (indent + 2))
        cls.write_json_value_(output, item, indent + 2)
      output.write('\n' + ' ' * indent + ']')
    elif isinstance(value, decimal.Decimal):
      if not value.is_finite():
        raise ValueError('Non-finite decimal is not valid JSON')
      output.write(format(value, 'f'))
    elif isinstance(value, float):
      if not math.isfinite(value):
        raise ValueError('Non-finite float is not valid JSON')
      output.write(format(decimal.Decimal(repr(value)), 'f'))
    elif value is True:
      output.write('true')
    elif value is False:
      output.write('false')
    elif value is None:
      output.write('null')
    elif isinstance(value, int):
      output.write(str(value))
    elif isinstance(value, str):
      output.write(json.dumps(value, ensure_ascii=False))
    else:
      raise TypeError('Unsupported JSON value: ' + type(value).__name__)

  @staticmethod
  def write_model_traits_(
      traits_file,
      features_importance,
      logloss_history=None,
      dataset_sizes=None,
      ctr_thresholds=None,
      train_start=None,
      train_end=None,
      peak_rss_bytes=None,
  ):
    with traits_file.open('w') as output:
      output.write('{\n  "features_importance": [')
      for index, item in enumerate(features_importance):
        output.write(',\n' if index else '\n')
        output.write('    {\n')
        output.write(
          '      "score": ' +
          format(decimal.Decimal(repr(item['score'])), 'f') + ',\n')
        output.write(
          '      "feature": ' +
          json.dumps(item['feature'], ensure_ascii=False))
        if 'name' in item:
          output.write(',\n      "name": ' + json.dumps(
            item['name'], ensure_ascii=False))
        for field in (
            'yes_share',
            'yes_ctr',
            'no_ctr',
            'yes_predicted_ctr',
            'no_predicted_ctr'):
          if field in item:
            output.write(
              ',\n      "' + field + '": ' +
              format(decimal.Decimal(item[field]), 'f'))
        output.write('\n    }')
      output.write('\n  ]' if features_importance else ']')

      if logloss_history is not None:
        output.write(',\n  "logloss_history": [')
        for index, item in enumerate(logloss_history):
          output.write(',\n' if index else '\n')
          output.write('    {\n')
          output.write('      "step": ' + str(item['step']) + ',\n')
          output.write(
            '      "train": ' +
            format(decimal.Decimal(str(item['train'])), 'f') + ',\n')
          history_fields = [
            ('test', format(decimal.Decimal(str(item['test'])), 'f'))]
          for field in (
              'train_rows',
              'train_clicks',
              'peak_rss_bytes',
              'train_ctr',
              'train_rmse',
              'val_rmse',
              'train_mae',
              'val_mae'):
            if field in item:
              value = item[field]
              history_fields.append(
                (
                  field,
                  str(int(value))
                  if field in (
                    'train_rows', 'train_clicks', 'peak_rss_bytes') else
                  format(decimal.Decimal(str(value)), 'f')))
          for field_index, (field, value) in enumerate(history_fields):
            output.write(
              ('      "' + field + '": ' + value +
               (',' if field_index + 1 < len(history_fields) else '') +
               '\n'))
          output.write('    }')
        output.write('\n  ]' if logloss_history else ']')

      if dataset_sizes is not None:
        output.write(',\n  "dataset_sizes": {')
        for index, (name, size) in enumerate(dataset_sizes.items()):
          output.write(',\n' if index else '\n')
          output.write('    ' + json.dumps(name) + ': {\n')
          output.write('      "rows": ' + str(int(size['rows'])) + ',\n')
          output.write('      "clicks": ' + str(int(size['clicks'])) + '\n')
          output.write('    }')
        output.write('\n  }' if dataset_sizes else '}')

      if ctr_thresholds is not None:
        output.write(',\n  "ctr_thresholds": [')
        for index, item in enumerate(ctr_thresholds):
          output.write(',\n' if index else '\n')
          output.write('    {\n')
          output.write(
            '      "ctr_goal": ' +
            format(decimal.Decimal(str(item['ctr_goal'])), 'f') + ',\n')
          output.write(
            '      "impressions": ' + str(item['impressions']) + ',\n')
          output.write('      "clicks": ' + str(item['clicks']) + ',\n')
          for field in ('share', 'actual_ctr', 'average_predicted_ctr'):
            output.write('      "' + field + '": ')
            value = item.get(field)
            output.write(
              'null' if value is None else
              format(decimal.Decimal(str(value)), 'f'))
            output.write(
              '\n' if field == 'average_predicted_ctr' else ',\n')
          output.write('    }')
        output.write('\n  ]' if ctr_thresholds else ']')

      if peak_rss_bytes:
        output.write(
          ',\n  "properties": [{"peak_rss_bytes": ' +
          str(int(peak_rss_bytes)) + '}]')

      if train_start is not None:
        output.write(',\n  "status": "published"')
        output.write(
          ',\n  "train_start": ' +
          json.dumps(train_start, ensure_ascii=False))
      if train_end is not None:
        output.write(
          ',\n  "train_end": ' +
          json.dumps(train_end, ensure_ascii=False))

      output.write('\n}\n')


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Train catboost model.')
  parser.add_argument('--train-file', help='Train libsvm.')
  parser.add_argument('--test-file', help='Test libsvm.')
  parser.add_argument('--features-config-file', required=True)
  parser.add_argument('--feature-dictionary-file')
  parser.add_argument('--algorithm-id', default='catboost')
  parser.add_argument('--output-dir', required=True)
  args = parser.parse_args()

  trainer = CatBoostTrainer(features_config_file=args.features_config_file)
  model = trainer.train(args.train_file, args.test_file)
  trainer.save_campaign_manager_model(
    model,
    args.output_dir,
    algorithm_id=args.algorithm_id,
    feature_dictionary_file=args.feature_dictionary_file)
