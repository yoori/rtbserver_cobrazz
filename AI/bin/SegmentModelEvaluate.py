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

import torch

from segment_model.SegmentModelConfig import SegmentModelConfig
from segment_model.SegmentModelTrainer import SegmentModelTrainer
from segment_model.SyntheticSegmentData import generate_synthetic_dataset


def main():
  parser = argparse.ArgumentParser(description='Evaluate differentiable URL segment model.')
  parser.add_argument('--model-dir', required=True)
  args = parser.parse_args()
  model_dir = pathlib.Path(args.model_dir)
  config = SegmentModelConfig.from_json(model_dir / 'config.json')
  dataset = generate_synthetic_dataset(config)
  trainer = SegmentModelTrainer(config, dataset)
  checkpoint = torch.load(model_dir / 'checkpoint.pt', map_location=trainer.device)
  trainer.model.load_state_dict(checkpoint['model'])
  metrics, rules = trainer.evaluate()
  print(json.dumps({
    'metrics': metrics,
    'segments': [rule.to_dict() for rule in rules],
    'ground_truth': trainer.ground_truth(),
  }, indent=2, sort_keys=True))


if __name__ == '__main__':
  main()
