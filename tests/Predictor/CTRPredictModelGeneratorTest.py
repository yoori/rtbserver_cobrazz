#!/usr/bin/env python3.12

import importlib.util
import pathlib
import signal
import sys
import tempfile
import unittest
import unittest.mock


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))
MODULE_FILE = SOURCE_ROOT / 'bin' / 'CTRPredictModelGenerator.py'
SPEC = importlib.util.spec_from_file_location(
  'ctr_predict_model_generator', MODULE_FILE)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

TRAINER_MODULE_FILE = SOURCE_ROOT / 'bin' / 'CTRPredictModelTrainer.py'
TRAINER_SPEC = importlib.util.spec_from_file_location(
  'ctr_predict_model_trainer', TRAINER_MODULE_FILE)
TRAINER_MODULE = importlib.util.module_from_spec(TRAINER_SPEC)
TRAINER_SPEC.loader.exec_module(TRAINER_MODULE)


class CTRPredictModelGeneratorTest(unittest.TestCase):
  def test_training_defaults_cover_multiple_chunks(self):
    config = MODULE.Config()

    self.assertEqual(7000000, config.selection_chunk_rows)
    self.assertEqual(10000000, config.main_chunk_rows)
    self.assertEqual(10, config.selection_fit_steps)
    self.assertEqual(30, config.training_fit_steps)
    self.assertEqual(
      70000000,
      config.selection_chunk_rows * config.selection_fit_steps)
    self.assertEqual(
      300000000,
      config.main_chunk_rows * config.training_fit_steps)

  def test_json_config(self):
    config = MODULE.Config()
    config.init_json({
      'pid_file': '/var/run/ctr-generator.pid',
      'workspace_root': '/var/lib/ctr-generator',
      'clickhouse_conn': '--host click00',
      'postgres_conn': 'host=postdb00 dbname=stat',
      'web_server': {
        'host': '127.0.0.1',
        'port': 18080,
      },
      'algorithm_id': '20260819.120000',
      'generate_period': 3600,
      'selection_chunk_rows': 70,
      'main_chunk_rows': 90,
      'validation_set_rows': 20,
      'selection_validation_sets': 2,
      'training_validation_sets': 3,
      'final_test_sets': 4,
      'selection_fit_steps': 7,
      'training_fit_steps': 12,
      'fit_iterations': 8,
      'selection_patience': 2,
      'training_patience': 6,
      'data_delay': 86400,
    })

    self.assertEqual('/var/lib/ctr-generator', config.workspace_root)
    self.assertEqual('host=postdb00 dbname=stat', config.postgres_conn)
    self.assertEqual('127.0.0.1', config.web_host)
    self.assertEqual(18080, config.web_port)
    self.assertEqual('20260819.120000', config.algorithm_id)
    self.assertEqual(3600.0, config.generate_period)
    self.assertEqual(70, config.selection_chunk_rows)
    self.assertEqual(90, config.main_chunk_rows)
    self.assertEqual(20, config.validation_set_rows)
    self.assertEqual(2, config.selection_validation_sets)
    self.assertEqual(3, config.training_validation_sets)
    self.assertEqual(4, config.final_test_sets)
    self.assertEqual(7, config.selection_fit_steps)
    self.assertEqual(12, config.training_fit_steps)
    self.assertEqual(8, config.fit_iterations)
    self.assertEqual(2, config.selection_patience)
    self.assertEqual(6, config.training_patience)
    self.assertEqual(86400, config.data_delay)

  def test_required_workspace_root(self):
    config = MODULE.Config()
    with self.assertRaisesRegex(ValueError, 'workspace_root'):
      config.init_json({
        'pid_file': '/tmp/ctr-generator.pid',
        'postgres_conn': 'host=postdb00 dbname=stat',
        'web_server': {'port': 18080},
        'data_delay': 86400,
      })

  def test_required_data_delay(self):
    config = MODULE.Config()
    with self.assertRaisesRegex(ValueError, 'data_delay'):
      config.init_json({
        'pid_file': '/tmp/ctr-generator.pid',
        'workspace_root': '/tmp/ctr-generator',
        'postgres_conn': 'host=postdb00 dbname=stat',
        'web_server': {'port': 18080},
      })

  def test_required_postgres_connection(self):
    config = MODULE.Config()
    with self.assertRaisesRegex(ValueError, 'postgres_conn'):
      config.init_json({
        'pid_file': '/tmp/ctr-generator.pid',
        'workspace_root': '/tmp/ctr-generator',
        'web_server': {'port': 18080},
        'data_delay': 86400,
      })

  def test_required_web_server(self):
    config = MODULE.Config()
    with self.assertRaisesRegex(ValueError, 'web_server'):
      config.init_json({
        'pid_file': '/tmp/ctr-generator.pid',
        'workspace_root': '/tmp/ctr-generator',
        'postgres_conn': 'host=postdb00 dbname=stat',
        'data_delay': 86400,
      })

  def test_features_config_is_embedded(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      features_config_file = TRAINER_MODULE.prepare_features_config(
        pathlib.Path(temp_dir))
      self.assertEqual(TRAINER_MODULE.FEATURE_CONFIG, TRAINER_MODULE.json.loads(
        features_config_file.read_text()))

  def test_stream_libsvm_chunks_removes_processed_files(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      work_dir = pathlib.Path(temp_dir)
      csv_file = work_dir / 'source.csv'
      csv_file.write_text('label,Device\n1,Mobile\n')
      features_config_file = work_dir / 'features.json'
      features_config_file.write_text('{}')
      feature_indexes_file = work_dir / 'indexes'
      feature_indexes_file.write_text('1\n')
      dictionary_lines = set()

      def csv_chunks():
        try:
          yield csv_file, 1
        finally:
          csv_file.unlink(missing_ok=True)

      def generate_libsvm(
          input_file,
          output_file,
          config_file,
          dictionary_file=None,
          indexes_file=None,
      ):
        self.assertEqual(csv_file, input_file)
        self.assertEqual(features_config_file, config_file)
        self.assertEqual(feature_indexes_file, indexes_file)
        output_file.write_text('1 1:1\n')
        dictionary_file.write_bytes(b'1,"device:Mobile"\n')

      with unittest.mock.patch.object(
          TRAINER_MODULE,
          'generate_libsvm',
          side_effect=generate_libsvm):
        chunks = TRAINER_MODULE.stream_libsvm_chunks(
          csv_chunks(),
          work_dir,
          'training',
          features_config_file,
          feature_indexes_file,
          dictionary_lines)
        svm_file = next(chunks)
        self.assertTrue(csv_file.exists())
        self.assertTrue(svm_file.exists())
        with self.assertRaises(StopIteration):
          next(chunks)

      self.assertFalse(csv_file.exists())
      self.assertFalse(svm_file.exists())
      self.assertEqual({b'1,"device:Mobile"\n'}, dictionary_lines)

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

  def test_stop_children_signals_process_group(self):
    process = unittest.mock.MagicMock()
    process.pid = 12345
    process.poll.return_value = 0

    with unittest.mock.patch.object(MODULE.os, 'killpg') as killpg:
      MODULE.stop_children([('trainer', process)])

    killpg.assert_called_once_with(12345, signal.SIGTERM)

  def test_child_command_uses_separate_entry_point(self):
    command = MODULE.child_command(
      'CTRPredictModelTrainer.py',
      '/tmp/config.json',
      run_once=True)

    self.assertTrue(command[1].endswith('/bin/CTRPredictModelTrainer.py'))
    self.assertEqual('--config=/tmp/config.json', command[2])
    self.assertEqual('--run-once', command[3])

  def test_supervisor_stops_first_child_if_second_start_fails(self):
    process = unittest.mock.MagicMock()
    with (
        unittest.mock.patch.object(
          MODULE,
          'start_child',
          side_effect=[process, RuntimeError('start failed')]),
        unittest.mock.patch.object(MODULE, 'stop_children') as stop_children):
      with self.assertRaisesRegex(RuntimeError, 'start failed'):
        MODULE.supervise('/tmp/config.json')

    stop_children.assert_called_once_with([('trainer', process)])


if __name__ == '__main__':
  unittest.main()
