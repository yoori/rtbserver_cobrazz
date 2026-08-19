#!/usr/bin/env python3.12

import importlib.util
import pathlib
import tempfile
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE_FILE = SOURCE_ROOT / 'bin' / 'CTRPredictModelGenerator.py'
SPEC = importlib.util.spec_from_file_location(
  'ctr_predict_model_generator', MODULE_FILE)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class CTRPredictModelGeneratorTest(unittest.TestCase):
  def test_json_config(self):
    config = MODULE.Config()
    config.init_json({
      'pid_file': '/var/run/ctr-generator.pid',
      'workspace_root': '/var/lib/ctr-generator',
      'clickhouse_conn': '--host click00',
      'algorithm_id': '20260819.120000',
      'generate_period': 3600,
      'train_rows': 100,
    })

    self.assertEqual('/var/lib/ctr-generator', config.workspace_root)
    self.assertEqual('20260819.120000', config.algorithm_id)
    self.assertEqual(3600.0, config.generate_period)
    self.assertEqual(100, config.train_rows)

  def test_required_workspace_root(self):
    config = MODULE.Config()
    with self.assertRaisesRegex(ValueError, 'workspace_root'):
      config.init_json({
        'pid_file': '/tmp/ctr-generator.pid',
      })

  def test_features_config_is_embedded(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      features_config_file = MODULE.prepare_features_config(pathlib.Path(temp_dir))
      self.assertEqual(MODULE.FEATURE_CONFIG, MODULE.json.loads(
        features_config_file.read_text()))

  def test_pid_file_is_exclusive_and_removed(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      pid_file = pathlib.Path(temp_dir) / 'nested' / 'generator.pid'
      with MODULE.PidFile(pid_file):
        self.assertTrue(pid_file.exists())
        self.assertTrue(pid_file.read_text().strip().isdigit())
        with self.assertRaisesRegex(RuntimeError, 'already running'):
          with MODULE.PidFile(pid_file):
            pass
      self.assertFalse(pid_file.exists())


if __name__ == '__main__':
  unittest.main()
