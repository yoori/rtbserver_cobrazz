#!/usr/bin/python3.12

import argparse
import json
import os
import pathlib
import sys


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[1]
TORCH_SITE_PACKAGES = pathlib.Path(os.environ.get(
  'AI_TORCH_SITE_PACKAGES',
  '/u01/foros/server-ai/python3.12-torch/site-packages'))
if TORCH_SITE_PACKAGES.is_dir():
  sys.path.insert(0, str(TORCH_SITE_PACKAGES))
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from segment_model.ClickHouseClient import ClickHouseClient
from segment_model.RImpressionScenarioData import make_rimpression_scenario_source
from segment_model.ScenarioDefinition import SegmentModelScenario
from segment_model.ScenarioSeeder import seed_scenario
from segment_model.SegmentModelConfig import SegmentModelConfig
from segment_model.SegmentModelPublication import SegmentModelPublication
from segment_model.SegmentModelTrainer import SegmentModelTrainer


def main():
  parser = argparse.ArgumentParser(
    description='Train the URL segment model from a ClickHouse/ExpressionMatcher scenario.')
  parser.add_argument('--config', required=True)
  parser.add_argument('--scenario', required=True)
  output_group = parser.add_mutually_exclusive_group(required=True)
  output_group.add_argument('--output-dir')
  output_group.add_argument('--repository-root')
  parser.add_argument('--model-id')
  parser.add_argument('--clickhouse-url', default='http://localhost:8123')
  parser.add_argument('--clickhouse-user', default='default')
  parser.add_argument('--clickhouse-password', default='')
  parser.add_argument(
    '--expression-matcher-url',
    dest='expression_matcher_urls',
    action='append')
  parser.add_argument('--source-batch-rows', type=int, default=4096)
  parser.add_argument('--profile-workers', type=int, default=16)
  parser.add_argument('--http-timeout', type=float, default=10.0)
  parser.add_argument('--seed-rimpression', action='store_true')
  parser.add_argument('--seed-chunk-rows', type=int, default=100000)
  parser.add_argument('--batch-cache-dir')
  parser.add_argument('--print-details', action='store_true')
  args = parser.parse_args()
  if args.expression_matcher_urls is None:
    args.expression_matcher_urls = ['http://localhost:8080']
  loading_limits = (
    args.source_batch_rows,
    args.profile_workers,
    args.http_timeout,
    args.seed_chunk_rows)
  if min(loading_limits) <= 0:
    parser.error('source loading and seeding limits must be positive')
  config = SegmentModelConfig.from_json(args.config)
  scenario = SegmentModelScenario.from_json(args.scenario)
  _validate_model_scenario(config, scenario)
  publication = None
  output_dir = args.output_dir
  if args.repository_root:
    publication = SegmentModelPublication(
      args.repository_root,
      args.model_id,
      {
        'training_kind': 'scenario',
        'scenario': pathlib.Path(args.scenario).stem,
      })
    publication.start()
    output_dir = publication.training_path
  if args.seed_rimpression:
    clickhouse_client = ClickHouseClient(
      args.clickhouse_url,
      args.clickhouse_user,
      args.clickhouse_password)
    clickhouse_client.wait_until_ready()
    seed_scenario(clickhouse_client, scenario, args.seed_chunk_rows, reset=True)
  cache_dir = args.batch_cache_dir
  if cache_dir is None:
    cache_dir = str(pathlib.Path(output_dir) / 'batch-cache')
  source = make_rimpression_scenario_source(
    config,
    scenario,
    args.clickhouse_url,
    args.expression_matcher_urls,
    args.source_batch_rows,
    args.profile_workers,
    args.http_timeout,
    args.clickhouse_user,
    args.clickhouse_password,
    cache_dir)
  trainer = SegmentModelTrainer(
    config,
    source.dataset,
    source.training_builder,
    source.validation_builder,
    source.final_test_builder)
  try:
    history = trainer.train(
      output_dir,
      publication.update_progress if publication else None)
    metrics, rules = trainer.evaluate()
    trainer.save(output_dir, history, metrics, rules)
    _write_scenario(pathlib.Path(output_dir), scenario)
    if publication:
      output_dir = publication.publish({
        'epochs_completed': len(history),
        'segments_count': len(rules),
      })
  except BaseException as error:
    if publication:
      publication.interrupt(str(error))
    raise
  result = {
    'metrics': metrics,
    'segments': [rule.to_dict() for rule in rules],
    'ground_truth': trainer.ground_truth(),
  }
  if not args.print_details:
    result = {
      'output_dir': str(output_dir),
      'soft_ctr': metrics['soft_ctr'],
      'hard_ctr': metrics['hard_ctr'],
      'soft_hard': metrics['soft_hard'],
      'groups': metrics['groups'],
      'segments': [rule.to_dict() for rule in rules],
    }
  print(json.dumps(result, indent=2, sort_keys=True))


def _validate_model_scenario(config, scenario):
  if config.model.context_size:
    raise ValueError('scenario test requires model.context_size=0')
  if config.model.membership.initialization == 'frequency':
    raise ValueError('streaming scenario training does not support frequency initialization')
  for segment in scenario.expected_segments:
    if segment.window_seconds not in config.data.windows_seconds:
      raise ValueError('expected segment window is missing from model windows_seconds')
    if segment.min_visits not in config.data.n_values:
      raise ValueError('expected segment threshold is missing from model n_values')


def _write_scenario(output_dir, scenario):
  output_dir.mkdir(parents=True, exist_ok=True)
  output_file = output_dir / 'scenario.json'
  temporary_file = output_file.with_name(output_file.name + '.tmp')
  temporary_file.write_text(
    json.dumps(scenario.to_dict(), indent=2, sort_keys=True) + '\n',
    encoding='utf-8')
  temporary_file.replace(output_file)


if __name__ == '__main__':
  main()
