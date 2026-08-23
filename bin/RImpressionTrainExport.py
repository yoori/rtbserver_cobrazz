#!/usr/bin/env python3.12

import argparse
import json
import logging
import subprocess
import sys

from rtbserver_utils.RImpressionTrainExporter import RImpressionTrainExporter


def main():
  parser = argparse.ArgumentParser(
    description='Export an RImpression training sample from ClickHouse.')
  parser.add_argument('--config', required=True, help='Model generator config.')
  parser.add_argument('--output', required=True, help='Output CSV file.')
  parser.add_argument(
    '--rows',
    type=int,
    help='Override the row count calculated from the configuration.')
  args = parser.parse_args()

  with open(args.config) as config_file:
    config = json.load(config_file)

  export_rows = args.rows
  if export_rows is None:
    selection_rows = (
      int(config.get('selection_chunk_rows', 7000000)) *
      int(config.get('selection_fit_steps', 10)))
    training_rows = (
      int(config.get('main_chunk_rows', 10000000)) *
      int(config.get('training_fit_steps', 30)))
    validation_sets = (
      int(config.get('selection_validation_sets', 3)) +
      int(config.get('training_validation_sets', 3)) +
      int(config.get('final_test_sets', 3)))
    export_rows = (
      max(selection_rows, training_rows) +
      int(config.get('validation_set_rows', 200000)) * validation_sets)
  try:
    data_delay = int(config['data_delay'])
  except (KeyError, TypeError, ValueError):
    raise ValueError(
      "Configuration value 'data_delay' must be a positive integer")

  exporter = RImpressionTrainExporter(config.get('clickhouse_conn', ''))
  date_from = exporter.export(args.output, export_rows, data_delay)
  print(
    'output=' + args.output +
    ' rows=' + str(export_rows) +
    ' date_from=' + date_from)


if __name__ == '__main__':
  logging.basicConfig(
    level='DEBUG',
    format='%(asctime)s - %(levelname)s - %(message)s')
  try:
    main()
  except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
    print(str(error), file=sys.stderr)
    sys.exit(1)
