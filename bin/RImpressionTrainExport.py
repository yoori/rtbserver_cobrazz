#!/usr/bin/env python3

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
    help='Override train_rows from the configuration.')
  args = parser.parse_args()

  with open(args.config) as config_file:
    config = json.load(config_file)

  train_rows = args.rows
  if train_rows is None:
    train_rows = int(config.get('train_rows', 1000000))

  exporter = RImpressionTrainExporter(config.get('clickhouse_conn', ''))
  date_from = exporter.export(args.output, train_rows)
  print(
    'output=' + args.output +
    ' rows=' + str(train_rows) +
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
