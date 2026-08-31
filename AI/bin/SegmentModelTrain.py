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
from segment_model.SegmentModelTrainer import SegmentModelTrainer
from segment_model.SyntheticSegmentData import generate_synthetic_dataset


def main():
  parser = argparse.ArgumentParser(description='Train differentiable URL segment model.')
  parser.add_argument('--config', required=True)
  parser.add_argument('--output-dir', required=True)
  parser.add_argument('--print-details', action='store_true')
  args = parser.parse_args()
  config = SegmentModelConfig.from_json(args.config)
  dataset = generate_synthetic_dataset(config)
  trainer = SegmentModelTrainer(config, dataset)
  history = trainer.train(args.output_dir)
  metrics, rules = trainer.evaluate()
  trainer.save(args.output_dir, history, metrics, rules)
  result = {
    'metrics': metrics,
    'segments': [rule.to_dict() for rule in rules],
    'ground_truth': trainer.ground_truth(),
  }
  if not args.print_details:
    result = {
      'output_dir': args.output_dir,
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
