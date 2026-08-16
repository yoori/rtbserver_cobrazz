#! /usr/bin/env python3

import argparse
import json
import pathlib
import time
import logging
import signal
import subprocess

from rtbserver_utils.SignalInterruptHandler import SignalInterruptHandler
from rtbserver_utils.CatBoostTrainer import CatBoostTrainer
from rtbserver_utils.RImpressionTrainExporter import RImpressionTrainExporter


logger = logging.getLogger(__name__)


class Config :
  clickhouse_conn: str = None
  pid_file: str = None
  tmp_dir: str = None
  generate_period: float = 3600.0
  train_rows: int = 1000000
  features_config_file: str = None
  features_dimension: int = 14

  def init_json(self, config_json) :
    self.pid_file = config_json.get('pid_file', None)
    self.clickhouse_conn = config_json.get('clickhouse_conn', '')
    self.tmp_dir = config_json.get('tmp_dir', None)
    self.generate_period = float(config_json.get('generate_period', 3600.0))
    self.train_rows = int(config_json.get('train_rows', 1000000))
    self.features_config_file = config_json.get('features_config_file', None)
    self.features_dimension = int(config_json.get('features_dimension', 14))


def count_lines(file_path: str):
  with open(file_path, 'r') as f:
    line_count = sum(1 for line in f)
  return line_count


def generate_model(config: Config):
  tmp_dir = pathlib.Path(config.tmp_dir or '/tmp')
  tmp_csv_file = tmp_dir / 'RImpressionTrain.csv'
  tmp_svm_file = tmp_dir / 'RImpressionTrain.libsvm'

  logger.debug("To load data from clickhouse")
  exporter = RImpressionTrainExporter(config.clickhouse_conn, logger)
  exporter.export(tmp_csv_file, config.train_rows)

  logger.debug("To generate svm file")
  with tmp_csv_file.open() as csv_file, tmp_svm_file.open('w') as svm_file:
    subprocess.run(
      [
        'CTRGenerator',
        'generate-svm',
        str(config.features_config_file),
      ],
      check=True,
      stdin=csv_file,
      stdout=svm_file)

  # train catboost model by svm
  process_rows = count_lines(tmp_svm_file)
  logger.debug("Train on " + str(process_rows) + " rows")

  model_file = tmp_dir / 'model.cbm'
  trainer = CatBoostTrainer(features_dimension=config.features_dimension)
  model = trainer.split_and_train(tmp_svm_file)
  model.save_model(str(model_file))


def generate_model_loop(config: Config):
  with SignalInterruptHandler(
    [ signal.SIGINT, signal.SIGUSR1, signal.SIGHUP ],
    handler=None
  ) as interrupter :
    while True :
      try :
        logger.debug("To generate CTR model")
        generate_model(config)
        logger.debug("From generate CTR model")
        time.sleep(config.generate_period)
      except Exception as e :
        logger.error("Global exception: " + str(e))


if __name__ == "__main__":
  logging.basicConfig(level = 'DEBUG', format = "%(asctime)s - %(levelname)s - %(message)s")

  parser = argparse.ArgumentParser(description='CTR model generator.')
  parser.add_argument('--config', help='config file.')
  parser.add_argument('--run-once', action='store_true', help='Generate model once and exit')
  args = parser.parse_args()
  config = Config()
  with open(args.config, 'r') as f :
    config_txt = f.read()
    config_json = json.loads(config_txt)
    config.init_json(config_json)
  if args.run_once:
    generate_model(config)
  else:
    generate_model_loop(config)
