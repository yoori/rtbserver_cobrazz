#!/usr/bin/env python3.12

import argparse
import errno
import fcntl
import json
import logging
import os
import pathlib
import signal
import subprocess
import sys
import time

from rtbserver_utils.CatBoostTrainer import CatBoostTrainer
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


class Config:
  def __init__(self):
    self.clickhouse_conn = ''
    self.pid_file = None
    self.workspace_root = None
    self.generate_period = 3600.0
    self.train_rows = 1000000
    self.data_delay = None
    self.algorithm_id = 'catboost'

  def init_json(self, config_json):
    def required_string(name):
      value = config_json.get(name)
      if not isinstance(value, str) or not value:
        raise ValueError("Configuration value '" + name + "' is required")
      return value

    self.pid_file = required_string('pid_file')
    self.workspace_root = required_string('workspace_root')
    self.clickhouse_conn = config_json.get('clickhouse_conn', '')
    self.algorithm_id = config_json.get('algorithm_id', 'catboost')
    self.generate_period = float(config_json.get('generate_period', 3600.0))
    self.train_rows = int(config_json.get('train_rows', 1000000))
    try:
      self.data_delay = int(config_json['data_delay'])
    except (KeyError, TypeError, ValueError):
      raise ValueError(
        "Configuration value 'data_delay' must be a positive integer")

    if not isinstance(self.clickhouse_conn, str):
      raise ValueError("Configuration value 'clickhouse_conn' must be a string")
    if not isinstance(self.algorithm_id, str) or not self.algorithm_id:
      raise ValueError("Configuration value 'algorithm_id' must be non-empty")
    if self.generate_period <= 0:
      raise ValueError('generate_period must be positive')
    if self.train_rows <= 0:
      raise ValueError('train_rows must be positive')
    if self.data_delay <= 0:
      raise ValueError('data_delay must be positive')


class PidFile:
  def __init__(self, file_name):
    self.path = pathlib.Path(file_name)
    self.file = None

  def __enter__(self):
    self.path.parent.mkdir(parents=True, exist_ok=True)
    self.file = self.path.open('a+')
    try:
      fcntl.flock(self.file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError as error:
      self.file.close()
      self.file = None
      if error.errno in (errno.EACCES, errno.EAGAIN):
        raise RuntimeError(
          'Another CTRPredictModelGenerator instance is already running: ' +
          str(self.path))
      raise

    self.file.seek(0)
    self.file.truncate()
    self.file.write(str(os.getpid()) + '\n')
    self.file.flush()
    return self

  def __exit__(self, exception_type, exception_value, traceback):
    if self.file is None:
      return
    try:
      try:
        self.path.unlink()
      except FileNotFoundError:
        pass
      fcntl.flock(self.file.fileno(), fcntl.LOCK_UN)
    finally:
      self.file.close()
      self.file = None


def count_lines(file_path):
  with open(file_path, 'r') as file:
    return sum(1 for _ in file)


def prepare_features_config(work_dir):
  features_config_file = work_dir / 'CTRGeneratorConfig.json'
  if not features_config_file.exists():
    with features_config_file.open('w') as output_file:
      json.dump(FEATURE_CONFIG, output_file, separators=(',', ':'))
      output_file.write('\n')
  return features_config_file


def generate_model(config):
  workspace_root = pathlib.Path(config.workspace_root)
  work_dir = workspace_root / 'CTRPredictModelGenerator'
  output_dir = workspace_root / 'log' / 'Predictor' / 'CTRConfig'
  work_dir.mkdir(parents=True, exist_ok=True)
  features_config_file = prepare_features_config(work_dir)
  csv_file = work_dir / 'RImpressionTrain.csv'
  svm_file = work_dir / 'RImpressionTrain.libsvm'
  dictionary_file = work_dir / 'RImpressionTrain.features'

  logger.debug('Loading data from ClickHouse')
  exporter = RImpressionTrainExporter(config.clickhouse_conn, logger)
  exporter.export(csv_file, config.train_rows, config.data_delay)

  logger.debug('Generating LibSVM file')
  with csv_file.open() as input_file, svm_file.open('w') as output_file:
    subprocess.run(
      [
        'CTRGenerator',
        'generate-svm',
        str(features_config_file),
        '--model=catboost',
        '--dictionary=' + str(dictionary_file),
      ],
      check=True,
      stdin=input_file,
      stdout=output_file)

  process_rows = count_lines(svm_file)
  if process_rows == 0:
    raise RuntimeError('Generated LibSVM file is empty')
  logger.debug('Training on %d rows', process_rows)

  trainer = CatBoostTrainer(
    features_config_file=features_config_file,
    train_dir=work_dir / 'catboost_info')
  model = trainer.split_and_train(svm_file)
  result_dir = trainer.save_campaign_manager_model(
    model,
    output_dir,
    algorithm_id=config.algorithm_id,
    feature_dictionary_file=dictionary_file)
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
  with PidFile(config.pid_file):
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


def load_config(file_name):
  with open(file_name, 'r') as file:
    config_json = json.load(file)
  config = Config()
  config.init_json(config_json)
  return config


def main():
  parser = argparse.ArgumentParser(description='CTR model generator.')
  parser.add_argument('--config', required=True, help='JSON configuration file.')
  parser.add_argument('--run-once', action='store_true', help='Generate one model and exit.')
  args = parser.parse_args()
  run_service(load_config(args.config), args.run_once)


if __name__ == '__main__':
  logging.basicConfig(level='DEBUG', format='%(asctime)s - %(levelname)s - %(message)s')
  try:
    main()
  except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError):
    logger.exception('CTR model generator failed')
    sys.exit(1)
