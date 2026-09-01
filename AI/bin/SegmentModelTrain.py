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

from segment_model.SegmentModelConfig import SegmentModelConfig
from segment_model.SegmentModelPublication import SegmentModelPublication
from segment_model.SegmentModelTrainer import SegmentModelTrainer
from segment_model.SyntheticSegmentData import generate_synthetic_dataset


def main():
  parser = argparse.ArgumentParser(description='Train differentiable URL segment model.')
  parser.add_argument('--config', required=True)
  output_group = parser.add_mutually_exclusive_group(required=True)
  output_group.add_argument('--output-dir')
  output_group.add_argument('--repository-root')
  parser.add_argument('--model-id')
  parser.add_argument('--print-details', action='store_true')
  args = parser.parse_args()
  config = SegmentModelConfig.from_json(args.config)
  dataset = generate_synthetic_dataset(config)
  trainer = SegmentModelTrainer(config, dataset)
  publication = None
  output_dir = args.output_dir
  if args.repository_root:
    publication = SegmentModelPublication(
      args.repository_root,
      args.model_id,
      {'training_kind': 'synthetic'})
    publication.start()
    output_dir = publication.training_path
  try:
    history = trainer.train(
      output_dir,
      publication.update_progress if publication else None)
    metrics, rules = trainer.evaluate()
    trainer.save(output_dir, history, metrics, rules)
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
      'recovery': {
        name: value
        for name, value in metrics['recovery'].items()
        if name != 'matches'
      },
    }
  print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
  main()
