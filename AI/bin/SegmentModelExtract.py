#!/usr/bin/python3.12

import argparse
import json
import pathlib


def main():
  parser = argparse.ArgumentParser(description='Print extracted URL segment rules.')
  parser.add_argument('--model-dir', required=True)
  args = parser.parse_args()
  segments_file = pathlib.Path(args.model_dir) / 'segments.json'
  with segments_file.open(encoding='utf-8') as stream:
    segments = json.load(stream)
  print(json.dumps(segments, indent=2, sort_keys=True))


if __name__ == '__main__':
  main()
