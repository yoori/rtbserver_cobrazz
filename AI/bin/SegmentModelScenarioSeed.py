#!/usr/bin/python3.12

import argparse
import pathlib
import sys


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from segment_model.ClickHouseClient import ClickHouseClient
from segment_model.ScenarioDefinition import SegmentModelScenario
from segment_model.ScenarioSeeder import seed_scenario


def main():
  parser = argparse.ArgumentParser(description='Seed scenario RImpression rows in ClickHouse.')
  parser.add_argument('--scenario', required=True)
  parser.add_argument('--clickhouse-url', default='http://localhost:8123')
  parser.add_argument('--clickhouse-user', default='default')
  parser.add_argument('--clickhouse-password', default='')
  parser.add_argument('--chunk-rows', type=int, default=100000)
  parser.add_argument('--reset', action='store_true')
  args = parser.parse_args()
  if args.chunk_rows <= 0:
    parser.error('--chunk-rows must be positive')
  scenario = SegmentModelScenario.from_json(args.scenario)
  client = ClickHouseClient(
    args.clickhouse_url,
    args.clickhouse_user,
    args.clickhouse_password)
  client.wait_until_ready()
  seed_scenario(client, scenario, args.chunk_rows, args.reset)


if __name__ == '__main__':
  main()
