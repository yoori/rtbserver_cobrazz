#!/usr/bin/python3.12

import json
import pathlib
import sys
import tempfile
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelViewerConfig import SegmentModelViewerConfig


class SegmentModelViewerConfigTest(unittest.TestCase):
  def test_loads_viewer_configuration(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      config_file = pathlib.Path(temp_dir) / 'config.json'
      config_file.write_text(json.dumps({
        'pid_file': '/tmp/SegmentModelViewer.pid',
        'model_root': '/tmp/segment-models',
        'web_server': {'host': '127.0.0.1', 'port': 11437},
        'url_path': '/segments',
      }))

      config = SegmentModelViewerConfig.from_json(config_file)

      self.assertEqual(pathlib.Path('/tmp/segment-models'), config.model_root)
      self.assertEqual('127.0.0.1', config.web_host)
      self.assertEqual(11437, config.web_port)
      self.assertEqual('/segments', config.url_path)

  def test_rejects_invalid_port(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      config_file = pathlib.Path(temp_dir) / 'config.json'
      config_file.write_text(json.dumps({
        'pid_file': '/tmp/viewer.pid',
        'model_root': '/tmp/models',
        'web_server': {'host': '127.0.0.1', 'port': 70000},
      }))

      with self.assertRaisesRegex(ValueError, 'between 1 and 65535'):
        SegmentModelViewerConfig.from_json(config_file)


if __name__ == '__main__':
  unittest.main()
