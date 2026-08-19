#!/usr/bin/python3.12

import logging
import os
import sys
import signal
import time
import json
import argparse
import typing
import datetime
import re
import shutil
import atexit
import jinja2


R_IMPRESSION_CREATE_TABLE_QUERY = (
  "CREATE TABLE IF NOT EXISTS RImpression ("
  "request_id String, "
  "timestamp SimpleAggregateFunction(any, Nullable(DateTime('UTC'))), "
  "device SimpleAggregateFunction(any, Nullable(UInt64)), "
  "ip SimpleAggregateFunction(any, Nullable(String)), "
  "uid SimpleAggregateFunction(any, Nullable(String)), "
  "url SimpleAggregateFunction(any, Nullable(String)), "
  "publisher_id SimpleAggregateFunction(any, Nullable(UInt64)), "
  "tag_id SimpleAggregateFunction(any, Nullable(UInt64)), "
  "etag SimpleAggregateFunction(any, Nullable(String)), "
  "campaign_id SimpleAggregateFunction(any, Nullable(UInt64)), "
  "ccg_id SimpleAggregateFunction(any, Nullable(UInt64)), "
  "ccid SimpleAggregateFunction(any, Nullable(UInt64)), "
  "geo_ch SimpleAggregateFunction(any, Nullable(String)), "
  "user_ch SimpleAggregateFunction(any, Nullable(String)), "
  "imp_ch SimpleAggregateFunction(any, Nullable(String)), "
  "bid_price SimpleAggregateFunction(any, Nullable(Float64)), "
  "bid_floor SimpleAggregateFunction(any, Nullable(Float64)), "
  "alg_id SimpleAggregateFunction(any, Nullable(String)), "
  "size_id SimpleAggregateFunction(any, Nullable(UInt64)), "
  "colo_id SimpleAggregateFunction(any, Nullable(UInt64)), "
  "predicted_ctr SimpleAggregateFunction(any, Nullable(Float64)), "
  "campaign_freq SimpleAggregateFunction(any, Nullable(UInt32)), "
  "cr_alg_id SimpleAggregateFunction(any, Nullable(String)), "
  "predicted_cr SimpleAggregateFunction(any, Nullable(Float64)), "
  "win_price SimpleAggregateFunction(any, Nullable(Float64)), "
  "viewability SimpleAggregateFunction(any, Nullable(Int32)), "
  "click_timestamp SimpleAggregateFunction(any, Nullable(DateTime('UTC'))), "
  "ssp_tag_id SimpleAggregateFunction(any, Nullable(String)), "
  "ssp_ctr SimpleAggregateFunction(any, Nullable(Float64)), "
  "ssp_viewability SimpleAggregateFunction(any, Nullable(Float64)), "
  "ssp_vtr SimpleAggregateFunction(any, Nullable(Float64))"
  ") ENGINE = AggregatingMergeTree "
  "PARTITION BY sipHash64(request_id) % 100 "
  "ORDER BY request_id "
  "SETTINGS index_granularity = 8192, parts_to_throw_insert = 16000"
)


R_ACTION_CREATE_TABLE_QUERY = (
  "CREATE TABLE IF NOT EXISTS RAction ("
  "timestamp DateTime, "
  "device Nullable(UInt64), "
  "ip String, "
  "uid String, "
  "url String, "
  "action_id Nullable(UInt64), "
  "order_id String, "
  "order_value Decimal(18, 8)"
  ") ENGINE = MergeTree "
  "ORDER BY (timestamp, uid, ifNull(action_id, 0))"
)


R_GEO_CREATE_TABLE_QUERY = (
  "CREATE TABLE IF NOT EXISTS RGeo ("
  "date Date, "
  "ip String, "
  "source String, "
  "latitude Decimal(18, 8), "
  "longitude Decimal(18, 8), "
  "type String, "
  "country String, "
  "region String, "
  "city String"
  ") ENGINE = ReplacingMergeTree "
  "ORDER BY (date, ip, source)"
)


BID_COST_CREATE_TABLE_QUERY = (
  "CREATE TABLE IF NOT EXISTS BidCost ("
  "timestamp Date, "
  "tag_id UInt64, "
  "ext_tag_id String, "
  "url String, "
  "cost Decimal(18, 6), "
  "unverified_imps Int64, "
  "imps Int64, "
  "clicks Int64"
  ") ENGINE = SummingMergeTree "
  "PARTITION BY timestamp "
  "ORDER BY (timestamp, tag_id, ext_tag_id, url, cost)"
)


class Config(object):
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
ClickhouseCsvUploader: uploaded CSV logs to ClickHouse.
"""
class ClickhouseCsvUploader(object) :
  clickhouse_conn : str
  command_line_templ : jinja2.Template
  create_table_command_line_templ : jinja2.Template
  create_table_query : str
  logger = None
  raise_on_upload_error : bool

  def __init__(
      self,
      config,
      adapter_script,
      target_table,
      logger = None,
      create_table_query = None,
      raise_on_upload_error = True,
      adapter_accepts_date = False,
  ) :
    self.clickhouse_conn = config.clickhouse_conn
    self.create_table_query = create_table_query
    self.adapter_accepts_date = adapter_accepts_date
    self.command_line_templ = jinja2.Template(
      adapter_script +
      "{% if adapter_date %} --date {{ adapter_date }}{% endif %} " +
      "{{ process_files|join(' ') }} | clickhouse-client {{clickhouse_conn}} " +
      '--query="INSERT INTO ' + target_table + ' FORMAT CSV"')
    self.create_table_command_line_templ = jinja2.Template(
      'clickhouse-client {{clickhouse_conn}} --query="{{create_table_query}}"')
    self.logger = logger
    self.raise_on_upload_error = raise_on_upload_error

  def init_storage(self) :
    if not self.create_table_query:
      return

    command_line = self.create_table_command_line_templ.render({
      'clickhouse_conn' : self.clickhouse_conn,
      'create_table_query' : self.create_table_query,
    })
    try :
      self.logger.debug("To create target table: " + command_line)
      ret_code = os.system(command_line)
      self.logger.debug("From create target table: " + str(ret_code))
      if ret_code != 0 :
        raise Exception("Error on create target table: command_line = '" + command_line + "'")
    except Exception as e :
      self.logger.exception("Exception on create target table: " +
        str(e) + ", command_line = '" + command_line + "'")
      raise

  def process(self, process_files, log_date = None) :
    # init sql for upload
    command_line = self.command_line_templ.render({
      'clickhouse_conn' : self.clickhouse_conn,
      'process_files' : process_files,
      'adapter_date' : log_date if self.adapter_accepts_date else None,
    })
    try :
      self.logger.debug("To upload " + " ".join(process_files))
      ret_code = os.system(command_line)
      self.logger.debug("From upload " + " ".join(process_files) + ": " + str(ret_code))
      if ret_code == 0 :
        for process_file in process_files:
          os.unlink(process_file)
      else :
        message = "Error on upload " + " ".join(process_files) + ": command_line = '" + command_line + "'"
        if self.raise_on_upload_error:
          raise Exception(message)
        else :
          self.logger.error(message)
    except Exception as e :
      if self.raise_on_upload_error:
        self.logger.exception("Exception on upload " + " ".join(process_files) + ": " +
          str(e) + ", command_line = '" + command_line + "'")
        raise
      else :
        self.logger.error("Exception on upload " + " ".join(process_files) + ": " + str(e) +
          ", command_line = '" + command_line + "'")


"""
RImpressionUploader: uploaded for RImpression logs.
"""
class RImpressionUploader(ClickhouseCsvUploader) :
  def __init__(self, config, logger = None) :
    super().__init__(
      config,
      'RImpressionClickhouseAdapter.py',
      'RImpression',
      logger = logger,
      create_table_query = R_IMPRESSION_CREATE_TABLE_QUERY)


"""
RClickUploader: uploaded for RImpression logs.
"""
class RClickUploader(ClickhouseCsvUploader) :
  def __init__(self, config, logger = None) :
    super().__init__(
      config,
      'RClickClickhouseAdapter.py',
      'RImpression',
      logger = logger,
      raise_on_upload_error = False)


"""
RActionUploader: uploaded for RAction logs.
"""
class RActionUploader(ClickhouseCsvUploader) :
  def __init__(self, config, logger = None) :
    super().__init__(
      config,
      'RActionClickhouseAdapter.py',
      'RAction',
      logger = logger,
      create_table_query = R_ACTION_CREATE_TABLE_QUERY)


"""
GeoUploader: uploads Geo logs to RGeo.
"""
class GeoUploader(ClickhouseCsvUploader) :
  def __init__(self, config, logger = None) :
    super().__init__(
      config,
      'GeoClickhouseAdapter.py',
      'RGeo',
      logger = logger,
      create_table_query = R_GEO_CREATE_TABLE_QUERY,
      adapter_accepts_date = True)


class BidCostUploader(ClickhouseCsvUploader) :
  def __init__(self, config, logger = None) :
    super().__init__(
      config,
      'BidCostClickhouseAdapter.py',
      'BidCost',
      logger = logger,
      create_table_query = BID_COST_CREATE_TABLE_QUERY)


def check_stat_files(
    interrupter,
    config = None,
    logger = None,
    processors = None,
) :
  def file_date(full_file):
    file_name = os.path.basename(full_file)
    match = re.search(r'_(\d{8})\d{6}', file_name)
    if not match:
      return None
    return datetime.datetime.strptime(match.group(1), '%Y%m%d').date().isoformat()

  def upload_files(processor, files_to_process, log_date):
    processor.process(files_to_process, log_date)

  for check_root in config.check_roots :
    logger.debug("Check root '" + check_root + "'")
    if not os.path.isdir(check_root):
      logger.debug("Skip missing root '" + check_root + "'")
      continue

    check_files = [ x for x in os.listdir(check_root) ]

    processing_groups: typing.Dict[str, typing.Dict[str, typing.List]] = {}

    for check_file in check_files :
      logger.debug("Check file '" + check_file + "'")
      check_file_parts = check_file.replace('_', '.').split('.')
      if len(check_file_parts) > 0 :
        prefix = check_file_parts[0]
        if prefix in processors :
          full_file = os.path.join(check_root, check_file)
          log_date = file_date(full_file)
          if prefix not in processing_groups:
            processing_groups[prefix] = {}
          if log_date not in processing_groups[prefix]:
            processing_groups[prefix][log_date] = []
          processing_groups[prefix][log_date].append(full_file)

          if len(processing_groups[prefix][log_date]) >= config.batch:
            files_to_process = processing_groups[prefix][log_date]
            processor = processors[prefix]
            try :
              upload_files(processor, files_to_process, log_date)
            except Exception as e :
              logger.exception("error on upload " + " ".join(files_to_process) + ": " + str(e))
              if config.error_root:
                for full_file in files_to_process:
                  shutil.move(full_file, config.error_root)
            processing_groups[prefix][log_date] = []

    for prefix, date_groups in processing_groups.items():
      processor = processors[prefix]
      for log_date, process_files_list in date_groups.items():
        if not process_files_list:
          continue
        try :
          upload_files(processor, process_files_list, log_date)
        except Exception as e :
          logger.exception("error on upload " + " ".join(process_files_list) + ": " + str(e))
          if config.error_root:
            for full_file in process_files_list:
              shutil.move(full_file, config.error_root)


def main() :
  parser = argparse.ArgumentParser(description = 'ClickhouseStatUploader.')
  parser.add_argument("-c", "--config", type = str, default = "./rimpressionStatUploader.conf")
  args = parser.parse_args()
  config = Config()
  with open(args.config, 'r') as f :
    config_txt = f.read()
    config_json = json.loads(config_txt)
    config.init_json(config_json)

  if config.pid_file :
    pid_file_dir = os.path.dirname(config.pid_file)
    if pid_file_dir:
      os.makedirs(pid_file_dir, exist_ok = True)

    pid = os.getpid()
    with open(config.pid_file, 'w') as f:
      f.write(str(pid))

    def remove_pid_file() :
      try :
        if os.path.exists(config.pid_file) :
          os.unlink(config.pid_file)
      except Exception:
        pass

    atexit.register(remove_pid_file)

  logging.basicConfig(level = 'DEBUG', format = "%(asctime)s - %(levelname)s - %(message)s")
  logger = logging.getLogger(__name__)

  # if you want to know what's happening
  logging.basicConfig(level='DEBUG')

  processors = {}
  processors['RImpression'] = RImpressionUploader(config, logger = logger)
  processors['RClick'] = RClickUploader(config, logger = logger)
  processors['RAction'] = RActionUploader(config, logger = logger)
  processors['Geo'] = GeoUploader(config, logger = logger)
  processors['BidCost'] = BidCostUploader(config, logger = logger)

  for processor in processors.values():
    processor.init_storage()

  with SignalInterruptHandler(
    [ signal.SIGINT, signal.SIGUSR1, signal.SIGHUP ],
    handler = None) as interrupter:
    while not interrupter.interrupted():
      try:
        logger.debug("To check stats: " + str(config.check_roots))
        check_stat_files(interrupter, config = config, logger = logger, processors = processors)
        logger.debug("From check stats")
        for i in range(60):
          if interrupter.interrupted():
            break
          time.sleep(1)
      except Exception as e:
        logger.error("Global exception: " + str(e))
    
if __name__ == '__main__':
  main()
