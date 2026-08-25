#!/usr/bin/env python3.12

import importlib.util
import pathlib
import sys
import unittest
import unittest.mock


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from rtbserver_utils.CTRPredictModelViewerConfig import Config


MODULE_FILE = SOURCE_ROOT / 'bin' / 'CTRPredictModelViewer.py'
SPEC = importlib.util.spec_from_file_location(
  'ctr_predict_model_viewer', MODULE_FILE)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class CTRPredictModelViewerTest(unittest.TestCase):
  def test_json_config(self):
    config = Config()
    config.init_json({
      'pid_file': '/var/run/ctr-viewer.pid',
      'model_root': '/var/lib/ctr-models',
      'web_server': {
        'host': '127.0.0.1',
        'port': 18080,
      },
    })

    self.assertEqual('/var/run/ctr-viewer.pid', config.pid_file)
    self.assertEqual('/var/lib/ctr-models', config.model_root)
    self.assertEqual('127.0.0.1', config.web_host)
    self.assertEqual(18080, config.web_port)

  def test_default_host(self):
    config = Config()
    config.init_json({
      'pid_file': '/var/run/ctr-viewer.pid',
      'model_root': '/var/lib/ctr-models',
      'web_server': {'port': 8080},
    })

    self.assertEqual('0.0.0.0', config.web_host)

  def test_required_model_root(self):
    config = Config()
    with self.assertRaisesRegex(ValueError, 'model_root'):
      config.init_json({
        'pid_file': '/var/run/ctr-viewer.pid',
        'web_server': {'port': 8080},
      })

  def test_port_range(self):
    config = Config()
    with self.assertRaisesRegex(ValueError, 'range'):
      config.init_json({
        'pid_file': '/var/run/ctr-viewer.pid',
        'model_root': '/var/lib/ctr-models',
        'web_server': {'port': 70000},
      })

  def test_service_uses_viewer_config_and_pid(self):
    config = unittest.mock.MagicMock()
    config.pid_file = '/var/run/ctr-viewer.pid'
    config.model_root = '/var/lib/ctr-models'
    config.web_host = '127.0.0.1'
    config.web_port = 18080
    repository = unittest.mock.MagicMock()
    application = unittest.mock.MagicMock()

    with (
        unittest.mock.patch.object(MODULE, 'PidFile') as pid_file,
        unittest.mock.patch.object(
          MODULE,
          'CTRModelRepository',
          return_value=repository) as repository_factory,
        unittest.mock.patch.object(
          MODULE,
          'create_application',
          return_value=application) as application_factory,
        unittest.mock.patch.object(MODULE.uvicorn, 'run') as uvicorn_run):
      MODULE.run_service(config)

    pid_file.assert_called_once_with(
      '/var/run/ctr-viewer.pid',
      'CTRPredictModelViewer')
    repository_factory.assert_called_once_with('/var/lib/ctr-models')
    application_factory.assert_called_once_with(repository)
    uvicorn_run.assert_called_once_with(
      application,
      host='127.0.0.1',
      port=18080,
      workers=1,
      access_log=False,
      log_config=None)


if __name__ == '__main__':
  unittest.main()
