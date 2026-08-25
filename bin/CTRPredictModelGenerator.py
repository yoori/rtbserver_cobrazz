#!/usr/bin/env python3.12

import argparse
import logging
import os
import pathlib
import signal
import subprocess
import sys
import time

from rtbserver_utils.CTRPredictModelGeneratorConfig import Config, load_config
from rtbserver_utils.PidFile import PidFile
from rtbserver_utils.SignalInterruptHandler import SignalInterruptHandler


logger = logging.getLogger(__name__)

CHILD_STOP_TIMEOUT = 10.0


def child_command(script_name, config_file, run_once=False):
  command = [
    sys.executable,
    str(pathlib.Path(__file__).resolve().with_name(script_name)),
    '--config=' + str(config_file),
  ]
  if run_once:
    command.append('--run-once')
  return command


def start_child(name, command):
  logger.info('Starting %s: %s', name, ' '.join(command))
  return subprocess.Popen(command, start_new_session=True)


def signal_process_groups(processes, sig):
  for _, process in processes:
    try:
      os.killpg(process.pid, sig)
    except ProcessLookupError:
      pass


def stop_children(processes, timeout=CHILD_STOP_TIMEOUT):
  signal_process_groups(processes, signal.SIGTERM)
  deadline = time.monotonic() + timeout
  while time.monotonic() < deadline:
    if all(process.poll() is not None for _, process in processes):
      return
    time.sleep(0.05)

  logger.error('Child stop timeout reached; sending SIGKILL')
  signal_process_groups(processes, signal.SIGKILL)
  for _, process in processes:
    try:
      process.wait(timeout=1.0)
    except subprocess.TimeoutExpired:
      logger.error('Failed to reap child pid=%d', process.pid)


def supervise(config_file):
  commands = [
    (
      'trainer',
      child_command('CTRPredictModelTrainer.py', config_file),
    ),
  ]
  processes = []
  with SignalInterruptHandler(
      [signal.SIGINT, signal.SIGTERM, signal.SIGUSR1, signal.SIGHUP],
      handler=None) as interrupter:
    try:
      for name, command in commands:
        processes.append((name, start_child(name, command)))
      while not interrupter.interrupted():
        for name, process in processes:
          return_code = process.poll()
          if return_code is not None:
            raise RuntimeError(
              name + ' exited unexpectedly with code ' + str(return_code))
        time.sleep(0.1)
    finally:
      stop_children(processes)


def run_once(config_file):
  process = start_child(
    'trainer',
    child_command('CTRPredictModelTrainer.py', config_file, run_once=True))
  with SignalInterruptHandler(
      [signal.SIGINT, signal.SIGTERM, signal.SIGUSR1, signal.SIGHUP],
      handler=None) as interrupter:
    while not interrupter.interrupted() and process.poll() is None:
      time.sleep(0.1)
    if interrupter.interrupted():
      stop_children([('trainer', process)])
      return
    return_code = process.returncode
  if return_code != 0:
    raise RuntimeError('trainer exited with code ' + str(return_code))


def run_service(config_file, run_once_mode):
  config = load_config(config_file)
  with PidFile(config.pid_file, 'CTRPredictModelGenerator'):
    if run_once_mode:
      run_once(config_file)
    else:
      supervise(config_file)


def main():
  parser = argparse.ArgumentParser(description='CTR model generator service.')
  parser.add_argument('--config', required=True, help='JSON configuration file.')
  parser.add_argument('--run-once', action='store_true')
  args = parser.parse_args()
  run_service(args.config, args.run_once)


if __name__ == '__main__':
  logging.basicConfig(
    level='DEBUG',
    format='%(asctime)s Supervisor[%(process)d] %(levelname)s %(message)s')
  try:
    main()
  except (OSError, RuntimeError, ValueError, subprocess.SubprocessError):
    logger.exception('CTR model generator service failed')
    sys.exit(1)
