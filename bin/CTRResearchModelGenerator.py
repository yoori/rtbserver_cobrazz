#!/usr/bin/env python3.12

import argparse
import logging
import os
import pathlib
import signal
import subprocess
import sys
import time

from rtbserver_utils.CTRPredictModelGeneratorConfig import load_config
from rtbserver_utils.PidFile import PidFile
from rtbserver_utils.SignalInterruptHandler import SignalInterruptHandler


logger = logging.getLogger(__name__)

CHILD_STOP_TIMEOUT = 10.0


def child_command(config_file, run_once=False):
  command = [
    sys.executable,
    str(pathlib.Path(__file__).resolve().with_name(
      'CTRResearchModelTrainer.py')),
    '--config=' + str(config_file),
  ]
  if run_once:
    command.append('--run-once')
  return command


def start_child(command):
  logger.info('Starting research trainer: %s', ' '.join(command))
  return subprocess.Popen(command, start_new_session=True)


def stop_child(process, timeout=CHILD_STOP_TIMEOUT):
  try:
    os.killpg(process.pid, signal.SIGTERM)
  except ProcessLookupError:
    return
  try:
    process.wait(timeout=timeout)
  except subprocess.TimeoutExpired:
    logger.error('Child stop timeout reached; sending SIGKILL')
    try:
      os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
      pass
    try:
      process.wait(timeout=1.0)
    except subprocess.TimeoutExpired:
      logger.error('Failed to reap research trainer pid=%d', process.pid)


def supervise(config_file):
  process = start_child(child_command(config_file))
  with SignalInterruptHandler(
      [signal.SIGINT, signal.SIGTERM, signal.SIGUSR1, signal.SIGHUP],
      handler=None) as interrupter:
    try:
      while not interrupter.interrupted():
        return_code = process.poll()
        if return_code is not None:
          raise RuntimeError(
            'research trainer exited unexpectedly with code ' +
            str(return_code))
        time.sleep(0.1)
    finally:
      stop_child(process)


def run_once(config_file):
  process = start_child(child_command(config_file, run_once=True))
  with SignalInterruptHandler(
      [signal.SIGINT, signal.SIGTERM, signal.SIGUSR1, signal.SIGHUP],
      handler=None) as interrupter:
    while not interrupter.interrupted() and process.poll() is None:
      time.sleep(0.1)
    if interrupter.interrupted():
      stop_child(process)
      return
  if process.returncode != 0:
    raise RuntimeError(
      'research trainer exited with code ' + str(process.returncode))


def run_service(config_file, run_once_mode):
  config = load_config(config_file)
  with PidFile(config.pid_file, 'CTRResearchModelGenerator'):
    if run_once_mode:
      run_once(config_file)
    else:
      supervise(config_file)


def main():
  parser = argparse.ArgumentParser(
    description='CTR research model generator service.')
  parser.add_argument('--config', required=True, help='JSON configuration file.')
  parser.add_argument('--run-once', action='store_true')
  args = parser.parse_args()
  run_service(args.config, args.run_once)


if __name__ == '__main__':
  logging.basicConfig(
    level='DEBUG',
    format='%(asctime)s ResearchSupervisor[%(process)d] %(levelname)s %(message)s')
  try:
    main()
  except (OSError, RuntimeError, ValueError, subprocess.SubprocessError):
    logger.exception('CTR research model generator service failed')
    sys.exit(1)
