#!/usr/bin/env python3.12

import argparse
import logging

import uvicorn

from rtbserver_utils.CTRModelRepository import CTRModelRepository
from rtbserver_utils.CTRModelWebApplication import create_application
from rtbserver_utils.CTRPredictModelGeneratorConfig import load_config


def main():
  parser = argparse.ArgumentParser(description='CTR model web server.')
  parser.add_argument('--config', required=True, help='JSON configuration file.')
  args = parser.parse_args()

  config = load_config(args.config)
  repository = CTRModelRepository(config.model_root())
  uvicorn.run(
    create_application(repository),
    host=config.web_host,
    port=config.web_port,
    workers=1,
    access_log=False,
    log_config=None)


if __name__ == '__main__':
  logging.basicConfig(
    level='INFO',
    format='%(asctime)s Web[%(process)d] %(levelname)s %(message)s')
  main()
