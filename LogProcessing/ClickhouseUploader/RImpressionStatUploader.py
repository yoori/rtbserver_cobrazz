#!/usr/bin/env python3

import logging
import os
import sys
import signal
import time
import json
import argparse
import typing
import shutil
import jinja2


class Config :
  clickhouse_conn: str = None
  pid_file: str = None
  check_roots: typing.List[str] = []
  error_root: str = None
  batch: int = 1000

  def init_json(self, config_json) :
    self.pid_file = config_json.get('pid_file', None)
    self.clickhouse_conn = config_json.get('clickhouse_conn', '')
    self.check_roots = config_json.get('check_roots', [])
    self.error_root = config_json.get('error_root', None)
    self.batch = config_json.get('batch', 1000)


class SignalInterruptHandler(object):
  def __init__(self, sigs = [ signal.SIGINT ], handler = None):
    self._sigs = sigs
    self._handlers = []
    if handler is not None :
      self._handlers.append(handler)

  def add_handler(self, handler):
    self._handlers.append(handler)
    
  def __enter__(self):
    self._interrupted = False
    self._released = False
    self._original_handlers = {}
    for sig in self._sigs :
      self._original_handlers[sig] = signal.getsignal(sig)

    def handler(signum, frame):
      self.release()
      self._interrupted = True

      for handler in self._handlers :
        try :
          handler(signum, frame)
        except Exception as e :
          print("SignalInterruptHandler: error on handler call: " + str(e))
          pass

    for sig in self._sigs :
      signal.signal(sig, handler)
    return self

  def interrupted(self) -> bool :
    return self._interrupted

  def __exit__(self, type, value, tb):
    self.release()

  def release(self):
    if self._released:
      return False

    for sig, original_handler in self._original_handlers.items() :
      signal.signal(sig, original_handler)
    self._released = True

    return True


"""
RImpressionUploader: uploaded for RImpression logs.
"""
class RImpressionUploader(object) :
  clickhouse_conn : str
  command_line_templ : jinja2.Template
  logger = None

  def __init__(self, config, logger = None) :
    self.clickhouse_conn = config.clickhouse_conn
    self.command_line_templ = jinja2.Template(
      "RImpressionClickhouseAdapter.py {{ process_files|join(' ') }} | clickhouse-client {{clickhouse_conn}} " +
      '--query="INSERT INTO RImpression FORMAT CSV"')
    self.logger = logger

  def process(self, process_files) :
    # init sql for upload
    command_line = self.command_line_templ.render({
      'clickhouse_conn' : self.clickhouse_conn, 'process_files' : process_files
    })
    try :
      self.logger.debug("To upload " + " ".join(process_files))
      ret_code = os.system(command_line)
      self.logger.debug("From upload " + " ".join(process_files) + "': " + str(ret_code))
      if ret_code == 0 :
        for process_file in process_files:
          os.unlink(process_file)
      else :
        raise Exception("Error on upload " + " ".join(process_files) + ": command_line = '" + command_line + "'")
    except Exception as e :
      self.logger.exception("Exception on upload " + " ".join(process_files) + ": " +
        str(e) + ", command_line = '" + command_line + "'")
      raise


"""
RClickUploader: uploaded for RImpression logs.
"""
class RClickUploader(object) :
  clickhouse_conn : str
  command_line_templ : jinja2.Template
  logger = None

  def __init__(self, config, logger = None) :
    self.clickhouse_conn = config.clickhouse_conn
    self.command_line_templ = jinja2.Template(
      "RClickClickhouseAdapter.py {{process_file}} | clickhouse-client {{clickhouse_conn}} " +
      '--query="INSERT INTO RImpression FORMAT CSV"')
    self.logger = logger

  def process(self, process_files) :
    # init sql for upload
    command_line = self.command_line_templ.render({
      'clickhouse_conn' : self.clickhouse_conn,
      'process_files' : process_files})
    try :
      self.logger.debug("To upload " + " ".join(process_files))
      ret_code = os.system(command_line)
      self.logger.debug("From upload " + " ".join(process_files) + ": " + str(ret_code))
      if ret_code == 0 :
        for process_file in process_files:
          os.unlink(process_file)
      else :
        self.logger.error("Error on upload " + " ".join(process_files) + ": command_line = '" + command_line + "'")
    except Exception as e :
      self.logger.error("Exception on upload " + " ".join(process_files) + "': " + str(e) +
        ", command_line = '" + command_line + "'")


def check_stat_files(
    interrupter,
    config = None,
    logger = None,
    processors = None,
) :
  for check_root in config.check_roots :
    logger.debug("Check root '" + check_root + "'")
    check_files = [ x for x in os.listdir(check_root) ]

    processing_groups: typing.Dict[str, typing.List] = {}

    for check_file in check_files :
      logger.debug("Check file '" + check_file + "'")
      check_file_parts = check_file.replace('_', '.').split('.')
      if len(check_file_parts) > 0 :
        prefix = check_file_parts[0]
        if prefix in processors :
          full_file = os.path.join(check_root, check_file)
          if prefix not in processing_groups:
            processing_groups[prefix] = []
          processing_groups[prefix].append(full_file)

          if len(processing_groups[prefix]) >= config.batch:
            process_files = processing_groups[prefix]
            processor = processors[prefix]
            try :
              processor.process(process_files)
            except Exception as e :
              logger.exception("error on upload " + " ".join(process_files) + ": " + str(e))
              if config.error_root:
                for full_file in process_files:
                  shutil.move(full_file, config.error_root)
            processing_groups[prefix] = []

    for prefix, process_files in processing_groups.items():
      processor = processors[prefix]
      try :
        processor.process(process_files)
      except Exception as e :
        logger.exception("error on upload " + " ".join(process_files) + ": " + str(e))
        if config.error_root:
          for full_file in process_files:
            shutil.move(full_file, config.error_root)


def main() :
  parser = argparse.ArgumentParser(description = 'RImpressionStatUploader.')
  parser.add_argument("-c", "--config", type = str, default = "./rimpressionStatUploader.conf")
  args = parser.parse_args()
  config = Config()
  with open(args.config, 'r') as f :
    config_txt = f.read()
    config_json = json.loads(config_txt)
    config.init_json(config_json)

  if config.pid_file :
    pid = os.getpid()
    with open(config.pid_file, 'wb') as f:
      f.write(str(pid).encode('utf-8'))
      f.close()

  logging.basicConfig(level = 'DEBUG', format = "%(asctime)s - %(levelname)s - %(message)s")
  logger = logging.getLogger(__name__)

  # if you want to know what's happening
  logging.basicConfig(level='DEBUG')

  processors = {}
  processors['RImpression'] = RImpressionUploader(config, logger = logger)
  processors['RClick'] = RClickUploader(config, logger = logger)

  try :
    with SignalInterruptHandler(
      [ signal.SIGINT, signal.SIGUSR1, signal.SIGHUP ],
      handler = None) as interrupter :
      while True :
        logger.debug("To check stats: " + str(config.check_roots))
        check_stat_files(interrupter, config = config, logger = logger, processors = processors)
        logger.debug("From check stats")
        time.sleep(60)
  except Exception as e :
    logger.error("Global exception: " + str(e))
    
if __name__ == '__main__':
  main()
