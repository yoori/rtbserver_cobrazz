#!/usr/bin/python3.12

import argparse
import os
import pathlib
import sys

import uvicorn


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from segment_model.SegmentModelRepository import SegmentModelRepository
from segment_model.SegmentModelViewerConfig import SegmentModelViewerConfig
from segment_model.SegmentModelWebApplication import create_application


class PidFile:
  def __init__(self, path):
    self.path = pathlib.Path(path)

  def __enter__(self):
    self.path.parent.mkdir(parents=True, exist_ok=True)
    self._remove_stale_file()
    descriptor = os.open(
      self.path,
      os.O_WRONLY | os.O_CREAT | os.O_EXCL,
      0o644)
    try:
      os.write(descriptor, (str(os.getpid()) + '\n').encode('ascii'))
    finally:
      os.close(descriptor)
    return self

  def __exit__(self, exception_type, exception, traceback):
    del exception_type
    del exception
    del traceback
    try:
      if int(self.path.read_text(encoding='ascii').strip()) == os.getpid():
        self.path.unlink()
    except (FileNotFoundError, ValueError):
      pass

  def _remove_stale_file(self):
    try:
      pid = int(self.path.read_text(encoding='ascii').strip())
    except FileNotFoundError:
      return
    except ValueError:
      self.path.unlink()
      return
    try:
      os.kill(pid, 0)
    except ProcessLookupError:
      self.path.unlink()
    except PermissionError:
      pass
    else:
      raise RuntimeError('SegmentModelViewer is already running')


def run_service(config):
  with PidFile(config.pid_file):
    repository = SegmentModelRepository(config.model_root)
    uvicorn.run(
      create_application(repository, config.url_path),
      host=config.web_host,
      port=config.web_port,
      workers=1,
      access_log=False,
      log_config=None)


def main():
  parser = argparse.ArgumentParser(description='Segment model viewer service.')
  parser.add_argument('--config', required=True)
  args = parser.parse_args()
  run_service(SegmentModelViewerConfig.from_json(args.config))


if __name__ == '__main__':
  main()
