#! /usr/bin/env python3

import os
import argparse
import json
import time
import logging
import signal
import clickhouse_connect

from rtbserver_utils.SignalInterruptHandler import SignalInterruptHandler


class Config :
  clickhouse_conn: str = None
  pid_file: str = None
  tmp_dir: str = None
  generate_period: str = None
  train_rows: int = 1000000

  def init_json(self, config_json) :
    self.pid_file = config_json.get('pid_file', None)
    self.clickhouse_conn = config_json.get('clickhouse_conn', '')
    self.tmp_dir = config_json.get('tmp_dir', None)
    self.train_rows = config_json.get('train_rows', 1000000)


def generate_model(config: Config):
  # load data from clickhouse to csv in PRImpression format
  client = clickhouse_connect.get_client(
    host='click00',
    port=8123,  # Default HTTP port, or 9440 for HTTPS
    username='',
    password='',
    database=''
  )
  where_crit = ''
  query_result = client.query(
    "SELECT toDate(timestamp), count(*) FROM RImpression " +
    where_crit +
    "GROUP BY toDate(timestamp) ORDER BY toDate(timestamp) DESC"
  )

  date_end = None
  cur_rows = 0
  for row in query_result.result_rows:
    date_end = row[0]
    cur_rows += row[1]
    if cur_rows >= config.train_rows:
      break

  tmp_csv_file = '/tmp/PRImpression.csv'
  query = (
    """SELECT
    If(click_timestamp IS NOT NULL, 1, 0) AS label,
    timestamp,
    device AS Device,
    url AS Link,
    publisher_id AS Publisher,
    tag_id AS Tag,
    etag AS ETag,
    campaign_id AS Campaign,
    ccg_id AS Group,
    ccid AS CCID,
    geo_ch AS GeoCh,
    user_ch AS UserCh,
    size_id AS SizeID,
    colo_id AS Colo,
    campaign_freq AS Campaign_Freq
    FROM RImpression WHERE timestamp >= '""" + str(date_end) + "'"
  ).strip().replace('\n', ' ')

  cmd = (
    'clickhouse-client -h click00 --query="' + query + ' FORMAT CSVWithNames" >"' +
    tmp_csv_file + '"'
  )

  # convert PRImpression to libsvm
  os.system(cmd)


def generate_model_loop(config: Config):
  with SignalInterruptHandler(
    [ signal.SIGINT, signal.SIGUSR1, signal.SIGHUP ],
    handler=None
  ) as interrupter :
    while True :
      try :
        logger.debug("To generate CTR model")
        generate_model(config)
        logger.debug("From generate CTR model")
        time.sleep(config.generate_period)
      except Exception as e :
        logger.error("Global exception: " + str(e))


if __name__ == "__main__":
  logging.basicConfig(level = 'DEBUG', format = "%(asctime)s - %(levelname)s - %(message)s")
  logger = logging.getLogger(__name__)

  parser = argparse.ArgumentParser(description='CTR model generator.')
  parser.add_argument('--config', help='config file.')
  parser.add_argument('--run-once', action='store_true', help='Generate model once and exit')
  args = parser.parse_args()
  config = Config()
  with open(args.config, 'r') as f :
    config_txt = f.read()
    config_json = json.loads(config_txt)
    config.init_json(config_json)
  if args.run_once:
    generate_model(config)
  else:
    generate_model_loop(config)
