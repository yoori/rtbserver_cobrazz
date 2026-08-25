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
      'campaign_model_activity_period': 604800,
      'min_campaign_model_imps': 250000,
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
    self.assertEqual(604800, config.campaign_model_activity_period)
    self.assertEqual(250000, config.min_campaign_model_imps)
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

  def test_campaign_model_defaults(self):
    config = MODULE.Config()

    self.assertEqual(14 * 24 * 60 * 60, config.campaign_model_activity_period)
    self.assertEqual(100000, config.min_campaign_model_imps)

  def test_campaign_correction_config_is_separate_and_campaign_conditioned(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      work_dir = pathlib.Path(temp_dir)
      common_file = TRAINER_MODULE.prepare_features_config(work_dir)
      correction_file = TRAINER_MODULE.prepare_features_config(
        work_dir,
        TRAINER_MODULE.CAMPAIGN_CORRECTION_FEATURE_CONFIG,
        'CTRGeneratorCampaignCorrectionConfig.json')

      self.assertNotEqual(common_file, correction_file)
      correction_config = TRAINER_MODULE.json.loads(
        correction_file.read_text())
      self.assertIn(['campaign'], correction_config['features'])
      self.assertIn(
        ['campaign', 'userch'],
        correction_config['features'])
      self.assertNotIn(['userch'], correction_config['features'])

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
      feature_statistics = TRAINER_MODULE.FeatureStatistics()

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
          stats_file=None,
      ):
        self.assertEqual(csv_file, input_file)
        self.assertEqual(features_config_file, config_file)
        self.assertEqual(feature_indexes_file, indexes_file)
        output_file.write_text('1 1:1\n')
        dictionary_file.write_bytes(b'1,"device:Mobile"\n')
        stats_file.write_text('0,1,1\n1,1,1\n')

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
          dictionary_lines,
          feature_statistics)
        svm_file = next(chunks)
        self.assertTrue(csv_file.exists())
        self.assertTrue(svm_file.exists())
        with self.assertRaises(StopIteration):
          next(chunks)

      self.assertFalse(csv_file.exists())
      self.assertFalse(svm_file.exists())
      self.assertEqual({b'1,"device:Mobile"\n'}, dictionary_lines)
      self.assertEqual(1, feature_statistics.total_impressions)
      self.assertEqual(1, feature_statistics.total_clicks)
      self.assertEqual((1, 1), feature_statistics.get(1))

  def test_feature_statistics_merges_chunks(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      first = temp_path / 'first.stats'
      second = temp_path / 'second.stats'
      first.write_text('0,10,2\n1,4,1\n2,7,2\n')
      second.write_text('0,5,1\n1,3,1\n')

      statistics = TRAINER_MODULE.FeatureStatistics()
      statistics.add_file(first)
      statistics.add_file(second)

      self.assertEqual(15, statistics.total_impressions)
      self.assertEqual(3, statistics.total_clicks)
      self.assertEqual((7, 2), statistics.get(1))
      self.assertEqual((7, 2), statistics.get(2))
      self.assertEqual((0, 0), statistics.get(3))

  def test_in_progress_model_uses_traits_and_is_removed(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      model_root = pathlib.Path(temp_dir)
      train_start = TRAINER_MODULE.datetime.datetime(
        2026,
        8,
        24,
        15,
        55,
        15,
        tzinfo=TRAINER_MODULE.datetime.timezone.utc)

      with TRAINER_MODULE.InProgressModel(
          model_root, train_start) as in_progress:
        self.assertEqual('20260824.155515', in_progress.model_id)
        traits_file = in_progress.path / 'traits.json'
        self.assertTrue(traits_file.is_file())
        traits = TRAINER_MODULE.json.loads(traits_file.read_text())
        self.assertEqual('in_progress', traits['status'])
        self.assertEqual('2026-08-24T15:55:15Z', traits['train_start'])
        self.assertGreater(traits['pid'], 0)

        in_progress.publish_model_plan([
          {
            'name': 'common',
            'kind': 'common',
            'status': 'planned',
          },
          {
            'name': 'campaign_123',
            'kind': 'campaign',
            'status': 'planned',
            'db_campaign_id': 123,
            'campaign_name': 'Campaign name',
          },
        ], eligible_campaigns=1)
        with unittest.mock.patch.object(
            TRAINER_MODULE,
            'utc_now_text',
            side_effect=[
              '2026-08-24T16:00:00Z',
              '2026-08-24T16:05:00Z',
            ]):
          in_progress.start_models('campaign_123')
          in_progress.complete_models('campaign_123')

        traits = TRAINER_MODULE.json.loads(traits_file.read_text())
        self.assertEqual(2, traits['model_plan']['models'])
        self.assertEqual(1, traits['model_plan']['campaign_models'])
        self.assertEqual(1, traits['model_plan']['eligible_campaigns'])
        campaign = traits['models'][1]
        self.assertEqual('completed', campaign['status'])
        self.assertEqual('2026-08-24T16:00:00Z', campaign['train_start'])
        self.assertEqual('2026-08-24T16:05:00Z', campaign['train_end'])
        self.assertFalse((in_progress.path / '.traits.json.tmp').exists())

      self.assertFalse(in_progress.path.exists())

  def test_interrupted_model_is_preserved_with_current_phase(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      model_root = pathlib.Path(temp_dir)
      train_start = TRAINER_MODULE.datetime.datetime(
        2026,
        8,
        24,
        15,
        55,
        15,
        tzinfo=TRAINER_MODULE.datetime.timezone.utc)

      with unittest.mock.patch.object(
          TRAINER_MODULE,
          'utc_now_text',
          side_effect=[
            '2026-08-24T16:00:00Z',
            '2026-08-24T16:05:00Z',
          ]):
        with self.assertRaisesRegex(RuntimeError, 'training failed'):
          with TRAINER_MODULE.InProgressModel(
              model_root, train_start) as in_progress:
            in_progress.publish_model_plan([{
              'name': 'common',
              'kind': 'common',
              'status': 'planned',
            }])
            in_progress.start_models('common')
            raise RuntimeError('training failed')

      self.assertTrue(in_progress.path.is_dir())
      traits = TRAINER_MODULE.json.loads(
        (in_progress.path / 'traits.json').read_text())
      self.assertEqual('interrupted', traits['status'])
      self.assertEqual('2026-08-24T16:05:00Z', traits['train_end'])
      self.assertEqual('RuntimeError', traits['interruption_reason'])
      self.assertEqual('interrupted', traits['models'][0]['status'])
      self.assertEqual('2026-08-24T16:05:00Z', traits['models'][0]['train_end'])

  def test_filter_validation_sets_collects_dataset_sizes(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      work_dir = pathlib.Path(temp_dir)
      csv_files = [
        work_dir / 'validation-000.csv',
        work_dir / 'validation-001.csv',
      ]
      selection_files = [
        work_dir / 'selection-validation-000.libsvm',
        work_dir / 'selection-validation-001.libsvm',
      ]
      for file_path in csv_files + selection_files:
        file_path.write_text('source\n')
      features_config_file = work_dir / 'features.json'
      features_config_file.write_text('{}')
      feature_indexes_path = work_dir / 'feature-indexes'
      feature_indexes_path.write_text('1\n')

      def generate_libsvm(
          input_file,
          output_file,
          config_file,
          dictionary_file=None,
          feature_indexes_file=None,
          feature_stats_file=None,
      ):
        self.assertIn(input_file, csv_files)
        self.assertEqual(features_config_file, config_file)
        self.assertIsNone(dictionary_file)
        self.assertEqual(feature_indexes_path, feature_indexes_file)
        output_file.write_text('1 1:1\n')
        chunk_index = csv_files.index(input_file)
        feature_stats_file.write_text(
          '0,' + str(10 + chunk_index) + ',' + str(2 + chunk_index) + '\n')

      with unittest.mock.patch.object(
          TRAINER_MODULE,
          'generate_libsvm',
          side_effect=generate_libsvm):
        validation_files, statistics = TRAINER_MODULE.filter_validation_sets(
          csv_files,
          selection_files,
          work_dir,
          features_config_file,
          feature_indexes_path)

      self.assertEqual(2, len(validation_files))
      self.assertTrue(all(file_path.exists() for file_path in validation_files))
      self.assertFalse(any(file_path.exists() for file_path in csv_files))
      self.assertFalse(any(file_path.exists() for file_path in selection_files))
      self.assertEqual(10, statistics[0].total_impressions)
      self.assertEqual(2, statistics[0].total_clicks)
      self.assertEqual(11, statistics[1].total_impressions)
      self.assertEqual(3, statistics[1].total_clicks)
      self.assertEqual(
        {'rows': 21, 'clicks': 5},
        TRAINER_MODULE.dataset_size(statistics))
      self.assertEqual([], list(work_dir.glob('*.stats')))

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
