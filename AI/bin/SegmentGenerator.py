#!/usr/bin/python3.12

import argparse
import json
import os
import pathlib
import signal
import time

import torch


class StopRequested(object):
  def __init__(self):
    self.value = False

  def __call__(self, signum, frame):
    del signum
    del frame
    self.value = True


def load_config(config_file):
  with pathlib.Path(config_file).open(encoding='utf-8') as input_file:
    config = json.load(input_file)
  pid_file = config.get('pid_file')
  if not pid_file:
    raise ValueError('pid_file is required')
  return pathlib.Path(pid_file)


def write_pid_file(pid_file):
  pid_file.parent.mkdir(parents=True, exist_ok=True)
  temporary_file = pid_file.with_name(pid_file.name + '.tmp')
  temporary_file.write_text(str(os.getpid()) + '\n', encoding='ascii')
  os.replace(temporary_file, pid_file)


def remove_pid_file(pid_file):
  try:
    if int(pid_file.read_text(encoding='ascii').strip()) == os.getpid():
      pid_file.unlink()
  except (FileNotFoundError, ValueError):
    pass


def run(config_file):
  pid_file = load_config(config_file)
  stop_requested = StopRequested()
  signal.signal(signal.SIGINT, stop_requested)
  signal.signal(signal.SIGTERM, stop_requested)
  signal.signal(signal.SIGHUP, stop_requested)

  write_pid_file(pid_file)
  try:
    print(
      'SegmentGenerator started: pid=' + str(os.getpid()) +
      ', torch=' + torch.__version__,
      flush=True)
    while not stop_requested.value:
      time.sleep(1)
  finally:
    remove_pid_file(pid_file)


def main():
  parser = argparse.ArgumentParser(description='AI segment generator service.')
  parser.add_argument('--config', required=True)
  args = parser.parse_args()
  run(args.config)


if __name__ == '__main__':
  main()
