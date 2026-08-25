#!/usr/bin/env python3.12

import argparse
import logging

import uvicorn

from rtbserver_utils.CTRModelRepository import CTRModelRepository
from rtbserver_utils.CTRModelWebApplication import create_application
from rtbserver_utils.CTRPredictModelViewerConfig import load_config
from rtbserver_utils.PidFile import PidFile


def run_service(config):
  with PidFile(config.pid_file, 'CTRPredictModelViewer'):
    repository = CTRModelRepository(config.model_root)
    uvicorn.run(
      create_application(repository),
      host=config.web_host,
      port=config.web_port,
      workers=1,
      access_log=False,
      log_config=None)


def main():
  parser = argparse.ArgumentParser(description='CTR model viewer service.')
  parser.add_argument('--config', required=True, help='JSON configuration file.')
  args = parser.parse_args()

  run_service(load_config(args.config))


if __name__ == '__main__':
  logging.basicConfig(
    level='INFO',
    format='%(asctime)s Viewer[%(process)d] %(levelname)s %(message)s')
  main()
