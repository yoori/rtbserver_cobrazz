#!/usr/bin/env python3.12

import decimal
import importlib.util
import pathlib
import signal
import sys
import tempfile
import unittest
import unittest.mock


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))
from rtbserver_utils.CTRModelTraits import section_value

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
  def test_ssp_identity_and_ctr_are_not_common_denoise_features(self):
    self.assertIn(['ssp_tag_id'], TRAINER_MODULE.FEATURE_CONFIG['features'])
    self.assertIn(['ssp_ctr'], TRAINER_MODULE.FEATURE_CONFIG['features'])
    self.assertNotIn(
      ['ssp_tag_id'],
      TRAINER_MODULE.CAMPAIGN_CORRECTION_FEATURE_CONFIG['features'])
    self.assertNotIn(
      ['ssp_ctr'],
      TRAINER_MODULE.CAMPAIGN_CORRECTION_FEATURE_CONFIG['features'])
    self.assertIn(
      ['ssp_tag_id'],
      TRAINER_MODULE.CAMPAIGN_MODEL_FEATURE_CONFIG['features'])
    self.assertIn(
      ['ssp_ctr'],
      TRAINER_MODULE.CAMPAIGN_MODEL_FEATURE_CONFIG['features'])

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

  def test_training_plan_has_granular_steps_and_completion_timestamp(self):
    config = MODULE.Config()

    prepare_steps = TRAINER_MODULE.prepare_train_steps(config)
    step_ids = {step['id'] for step in prepare_steps}

    self.assertIn('fit_row_counts', step_ids)
    self.assertIn('feature_selection_export_001', step_ids)
    self.assertIn('feature_selection_libsvm_001', step_ids)
    self.assertIn('feature_selection_fit_001', step_ids)
    self.assertIn('deduplicate_feature_indexes', step_ids)
    self.assertIn('selection_validation_libsvm_001', step_ids)
    self.assertNotIn('common_validation_libsvm_001', step_ids)
    self.assertNotIn('aligned_validation_denoise_libsvm_001', step_ids)
    common_step_ids = {
      step['id']
      for step in TRAINER_MODULE.common_train_steps(config)
    }
    aligned_step_ids = {
      step['id']
      for step in TRAINER_MODULE.aligned_train_steps(config, 'Aligned')
    }
    self.assertIn('common_validation_libsvm_001', common_step_ids)
    self.assertIn(
      'aligned_validation_denoise_libsvm_001',
      aligned_step_ids)
    campaign_step_ids = {
      step['id']
      for step in TRAINER_MODULE.campaign_train_steps(
        config,
        selection_fit_steps=2,
        training_fit_steps=3)
    }
    self.assertIn(
      'campaign_selection_validation_inputs_001',
      campaign_step_ids)
    self.assertIn('campaign_feature_selection_fit_002', campaign_step_ids)
    self.assertNotIn('campaign_feature_selection_fit_003', campaign_step_ids)
    self.assertIn('campaign_training_fit_003', campaign_step_ids)
    self.assertNotIn('campaign_training_fit_004', campaign_step_ids)
    ssp_ctr_step_ids = {
      step['id']
      for step in TRAINER_MODULE.ssp_ctr_train_steps(
        config,
        selection_fit_steps=2,
        training_fit_steps=3)
    }
    self.assertIn('ssp_selection_validation_libsvm_001', ssp_ctr_step_ids)
    self.assertIn('ssp_feature_selection_fit_002', ssp_ctr_step_ids)
    self.assertNotIn('ssp_feature_selection_fit_003', ssp_ctr_step_ids)
    self.assertIn('ssp_training_fit_003', ssp_ctr_step_ids)
    self.assertIn('finalize_metrics', ssp_ctr_step_ids)
    self.assertTrue(all(step['started'] is None for step in prepare_steps))
    self.assertTrue(all(step['ended'] is None for step in prepare_steps))

  def test_json_config(self):
    config = MODULE.Config()
    config.init_json({
      'pid_file': '/var/run/ctr-generator.pid',
      'workspace_root': '/var/lib/ctr-generator',
      'clickhouse_conn': '--host click00',
      'postgres_conn': 'host=postdb00 dbname=stat',
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
    self.assertFalse(hasattr(config, 'selection_patience'))
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
        'data_delay': 86400,
      })

  def test_required_data_delay(self):
    config = MODULE.Config()
    with self.assertRaisesRegex(ValueError, 'data_delay'):
      config.init_json({
        'pid_file': '/tmp/ctr-generator.pid',
        'workspace_root': '/tmp/ctr-generator',
        'postgres_conn': 'host=postdb00 dbname=stat',
      })

  def test_required_postgres_connection(self):
    config = MODULE.Config()
    with self.assertRaisesRegex(ValueError, 'postgres_conn'):
      config.init_json({
        'pid_file': '/tmp/ctr-generator.pid',
        'workspace_root': '/tmp/ctr-generator',
        'data_delay': 86400,
      })

  def test_legacy_web_server_is_ignored(self):
    config = MODULE.Config()
    config.init_json({
      'pid_file': '/tmp/ctr-generator.pid',
      'workspace_root': '/tmp/ctr-generator',
      'postgres_conn': 'host=postdb00 dbname=stat',
      'web_server': {'host': '127.0.0.1', 'port': 18080},
      'data_delay': 86400,
    })

    self.assertFalse(hasattr(config, 'web_host'))
    self.assertFalse(hasattr(config, 'web_port'))

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

  def test_campaign_fit_steps_keep_chunks_near_the_row_limit(self):
    self.assertEqual(
      1,
      TRAINER_MODULE.campaign_fit_steps(100000, 5000000, 30))
    self.assertEqual(
      2,
      TRAINER_MODULE.campaign_fit_steps(6000000, 5000000, 30))
    self.assertEqual(
      30,
      TRAINER_MODULE.campaign_fit_steps(200000000, 5000000, 30))
    self.assertEqual(
      300,
      TRAINER_MODULE.scaled_fit_iterations(10, 30, 1))
    self.assertEqual(
      150,
      TRAINER_MODULE.scaled_fit_iterations(10, 30, 2))
    self.assertEqual(
      10,
      TRAINER_MODULE.scaled_fit_iterations(10, 30, 30))

  def test_repeated_partitioned_chunks_restart_after_source_exhaustion(self):
    class Exporter:
      def __init__(self):
        self.calls = 0

      def export_partitioned_chunks(self, *args, **kwargs):
        del args, kwargs
        self.calls += 1
        call = self.calls

        def chunks():
          yield 'chunk-' + str(call), call

        return chunks()

    exporter = Exporter()
    repeated = TRAINER_MODULE.repeat_partitioned_chunks(
      exporter,
      pathlib.Path('/tmp'),
      'training',
      1,
      1,
      1,
      'from',
      'to')

    self.assertEqual(
      [('chunk-1', 1), ('chunk-2', 2), ('chunk-3', 3)],
      [next(repeated), next(repeated), next(repeated)])
    repeated.close()
    self.assertEqual(3, exporter.calls)

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

  def test_common_ssp_ctr_config_uses_request_features_without_target(self):
    features = TRAINER_MODULE.SSP_CTR_FEATURE_CONFIG['features']

    self.assertIn(['publisher'], features)
    self.assertIn(['ssp_tag_id'], features)
    self.assertIn(['ssp_viewability'], features)
    self.assertIn(['ssp_vtr'], features)
    self.assertNotIn(['ssp_ctr'], features)
    self.assertNotIn(['campaign'], features)
    self.assertNotIn(['group'], features)
    self.assertNotIn(['ccid'], features)

  def test_final_model_properties_use_best_validation_checkpoint(self):
    properties = TRAINER_MODULE.final_model_properties(
      [
        {'step': 1, 'train': 0.5, 'test': 0.4},
        {'step': 2, 'train': 0.3, 'test': 0.2},
        {'step': 3, 'train': 0.1, 'test': 0.25},
      ],
      ssp_ctr_logloss=0.125)

    self.assertEqual([
      {'train_logloss': 0.3},
      {'val_logloss': 0.2},
      {'ssp_ctr_logloss': 0.125},
    ], properties)

  def test_final_model_properties_include_rmse_from_best_checkpoint(self):
    properties = TRAINER_MODULE.final_model_properties([
      {
        'step': 1,
        'train': 0.5,
        'test': 0.4,
        'train_rmse': 0.25,
        'val_rmse': 0.35,
        'train_mae': 0.2,
        'val_mae': 0.3,
        'peak_rss_bytes': 3221225472,
      },
      {
        'step': 2,
        'train': 0.3,
        'test': 0.2,
        'train_rmse': 0.15,
        'val_rmse': 0.2,
        'train_mae': 0.1,
        'val_mae': 0.15,
        'peak_rss_bytes': 2147483648,
      },
    ])

    self.assertEqual([
      {'train_logloss': 0.3},
      {'val_logloss': 0.2},
      {'train_rmse': 0.15},
      {'val_rmse': 0.2},
      {'train_mae': 0.1},
      {'val_mae': 0.15},
      {'peak_rss_bytes': 3221225472},
    ], properties)

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
      callback_files = []

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
          feature_statistics,
          csv_chunk_callback=lambda path, rows: callback_files.append(
            (path, rows, path.exists())))
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
      self.assertEqual([(csv_file, 1, True)], callback_files)
      self.assertEqual((1, 1), feature_statistics.get(1))

  def test_deduplicate_feature_indexes_merges_sources_and_preserves_drops(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      work_dir = pathlib.Path(temp_dir)
      source_files = [
        work_dir / 'first.libsvm',
        work_dir / 'second.libsvm',
      ]
      source_files[0].write_text('0 1:1 2:1\n')
      source_files[1].write_text('1 1:2 2:2 3:1\n')
      dropped_file = work_dir / 'dropped'
      early_dropped_file = work_dir / 'early-dropped'
      early_dropped_file.write_text('1\n')
      calls = []

      def run(command, check):
        self.assertTrue(check)
        self.assertEqual('FeatureDeduplicator', command[0])
        calls.append(command)
        source_path = pathlib.Path(command[command.index('--svm-file') + 1])
        self.assertEqual(
          '0 1:1 2:1\n1 1:2 2:2 3:1\n',
          source_path.read_text())
        indexes_path = pathlib.Path(command[command.index('--feature-indexes-file') + 1])
        self.assertEqual('1\n2\n3\n', indexes_path.read_text())
        pathlib.Path(
          command[command.index('--output-feature-indexes-file') + 1]
        ).write_text('1\n3\n')
        pathlib.Path(command[command.index('--dropped-features-file') + 1]).write_text('2\n')

      with unittest.mock.patch.object(
          TRAINER_MODULE.subprocess,
          'run',
          side_effect=run):
        result = TRAINER_MODULE.deduplicate_feature_indexes(
          source_files,
          {1, 2, 3},
          work_dir,
          dropped_features_file=dropped_file,
          early_dropped_features_file=early_dropped_file)

      self.assertEqual({1, 3}, result)
      self.assertEqual('2\n', dropped_file.read_text())
      self.assertEqual(1, len(calls))
      self.assertEqual(
        str(early_dropped_file),
        calls[0][calls[0].index('--early-dropped-features-file') + 1])

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

  def test_ssp_ctr_threshold_statistics(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      csv_file = pathlib.Path(temp_dir) / 'selection.csv'
      csv_file.write_text(
        'label,SSP_CTR\n'
        '1,0.0005\n'
        '0,0.001\n'
        '1,0.0011\n'
        '0,0.03\n'
        '1,0.031\n'
        '1,\n')

      result = TRAINER_MODULE.ssp_ctr_threshold_statistics(csv_file)

      self.assertEqual(5, result['rows'])
      self.assertEqual(3, result['clicks'])
      self.assertEqual(0.0, result['ctr_thresholds'][0]['ctr_goal'])
      self.assertEqual(5, result['ctr_thresholds'][0]['impressions'])
      self.assertEqual(3, result['ctr_thresholds'][0]['clicks'])
      self.assertAlmostEqual(
        0.0636,
        result['ctr_thresholds'][0]['predicted_ctr_sum'])
      self.assertEqual(0.001, result['ctr_thresholds'][1]['ctr_goal'])
      self.assertEqual(3, result['ctr_thresholds'][1]['impressions'])
      self.assertEqual(2, result['ctr_thresholds'][1]['clicks'])
      self.assertAlmostEqual(
        0.0621,
        result['ctr_thresholds'][1]['predicted_ctr_sum'])
      aggregate = TRAINER_MODULE.add_ctr_thresholds(
        None,
        result['ctr_thresholds'])
      aggregate = TRAINER_MODULE.add_ctr_thresholds(
        aggregate,
        result['ctr_thresholds'])
      finalized = TRAINER_MODULE.finalize_ctr_thresholds(aggregate)
      self.assertEqual(10, finalized[0]['impressions'])
      self.assertEqual(6, finalized[0]['clicks'])
      self.assertAlmostEqual(0.01272, finalized[0]['average_predicted_ctr'])

  def test_prepare_feature_selection_steps_include_ssp_ctr_thresholds(self):
    config = MODULE.Config()

    steps = TRAINER_MODULE.prepare_train_steps(config)
    ids = [step['id'] for step in steps]

    self.assertIn('feature_selection_thresholds_001', ids)

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
          in_progress.complete_model(
            'campaign_123',
            file='campaign_123.cbm',
            feature_groups=[['campaign']],
            features_importance=[{
              'score': decimal.Decimal('0.00008901938322533171'),
              'feature': 'campaign:123',
              'yes_share': decimal.Decimal('1.250000'),
              'yes_ctr': decimal.Decimal('0.002000'),
              'no_ctr': decimal.Decimal('0.001000'),
            }],
            logloss_history=[{
              'step': 1,
              'train': 0.1,
              'test': 0.2,
            }])

        traits = TRAINER_MODULE.json.loads(traits_file.read_text())
        self.assertEqual(2, traits['model_plan']['models'])
        self.assertEqual(1, traits['model_plan']['campaign_models'])
        self.assertEqual(1, traits['model_plan']['eligible_campaigns'])
        campaign = traits['models'][1]
        self.assertEqual('completed', campaign['status'])
        self.assertEqual('2026-08-24T16:00:00Z', campaign['train_start'])
        self.assertEqual('2026-08-24T16:05:00Z', campaign['train_end'])
        self.assertEqual('campaign_123.cbm', campaign['file'])
        campaign_artifact_file = in_progress.path / campaign['artifact']
        campaign_artifact = TRAINER_MODULE.json.loads(
          campaign_artifact_file.read_text())
        self.assertEqual([
          'feature_groups',
          'training_report',
          'feature_importance',
        ], [section['id'] for section in campaign_artifact['sections']])
        self.assertNotIn('features_importance', campaign_artifact)
        self.assertNotIn('logloss_history', campaign_artifact)
        self.assertEqual(
          'campaign:123',
          section_value(
            campaign_artifact,
            'feature_importance')[0]['feature'])
        self.assertEqual(
          decimal.Decimal('0.00008901938322533171'),
          section_value(
            TRAINER_MODULE.json.loads(
              campaign_artifact_file.read_text(),
              parse_float=decimal.Decimal),
            'feature_importance')[0]['score'])
        self.assertIn(
          '"yes_share": 1.250000',
          campaign_artifact_file.read_text())
        self.assertEqual(
          0.2,
          section_value(
            campaign_artifact,
            'training_report')[0]['test'])
        self.assertEqual(2, campaign_artifact['artifact_version'])
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
              'train_steps': [
                TRAINER_MODULE.train_step('fit_001', 'Fit 1/1'),
              ],
            }])
            in_progress.model_traits('common')['features_importance'] = [{
              'score': decimal.Decimal('0.00008901938322533171'),
            }]
            with in_progress.train_step('common', 'fit_001'):
              raise RuntimeError('training failed')

      self.assertTrue(in_progress.path.is_dir())
      traits = TRAINER_MODULE.json.loads(
        (in_progress.path / 'traits.json').read_text())
      self.assertEqual('interrupted', traits['status'])
      self.assertEqual('2026-08-24T16:05:00Z', traits['train_end'])
      self.assertEqual('RuntimeError', traits['interruption_reason'])
      self.assertEqual('interrupted', traits['models'][0]['status'])
      self.assertEqual(
        decimal.Decimal('0.00008901938322533171'),
        section_value(
          TRAINER_MODULE.json.loads(
            (in_progress.path / traits['models'][0]['artifact']).read_text(),
            parse_float=decimal.Decimal),
          'feature_importance')[0]['score'])
      self.assertEqual('2026-08-24T16:05:00Z', traits['models'][0]['train_end'])
      model_traits = TRAINER_MODULE.json.loads(
        (in_progress.path / traits['models'][0]['artifact']).read_text())
      step = section_value(model_traits, 'processing_steps')[0]
      self.assertEqual('2026-08-24T16:00:00Z', step['started'])
      self.assertIsNone(step['ended'])

  def test_completed_training_step_records_started_and_ended(self):
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
          model_root,
          train_start,
          prepare_steps=[
            TRAINER_MODULE.train_step('export_001', 'Export 1/1'),
          ]) as in_progress:
        with unittest.mock.patch.object(
            TRAINER_MODULE,
            'utc_now_text',
            side_effect=[
              '2026-08-24T16:00:00Z',
              '2026-08-24T16:05:00Z',
            ]):
          with in_progress.train_step('prepare', 'export_001'):
            pass

        traits = TRAINER_MODULE.json.loads(
          (in_progress.path / 'traits.json').read_text())
        prepare_traits = TRAINER_MODULE.json.loads(
          (in_progress.path / traits['prepare']['artifact']).read_text())
        step = section_value(prepare_traits, 'processing_steps')[0]
        self.assertEqual('2026-08-24T16:00:00Z', step['started'])
        self.assertEqual('2026-08-24T16:05:00Z', step['ended'])

  def test_post_processing_results_are_sharded_from_manifest(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      model_root = pathlib.Path(temp_dir)
      with TRAINER_MODULE.InProgressModel(model_root) as in_progress:
        in_progress.publish_post_processing_plan(
          [TRAINER_MODULE.train_step(
            'campaign_123_save',
            'Campaign 123: save evaluation artifact')],
          [{
            'name': 'campaign_123',
            'status': 'planned',
            'db_campaign_id': 123,
            'campaign_name': 'Campaign name',
            'artifact': 'traits/post_processing/campaign_123.json',
          }])
        in_progress.write_post_processing_target('campaign_123', {
          'name': 'campaign_123',
          'status': 'completed',
          'rows': 1000,
          'clicks': 2,
          'evaluations': [{
            'model': 'common_stable',
            'logloss': 0.0123,
          }],
        })

        manifest = TRAINER_MODULE.json.loads(
          (in_progress.path / 'traits.json').read_text())
        self.assertNotIn('targets', manifest['post_processing'])
        self.assertEqual(1, manifest['post_processing']['targets_count'])
        index = TRAINER_MODULE.json.loads(
          (in_progress.path / manifest['post_processing']['artifact'])
          .read_text())
        targets = section_value(index, 'post_processing_results')
        self.assertNotIn('evaluations', targets[0])
        target = TRAINER_MODULE.json.loads(
          (in_progress.path / targets[0]['artifact']).read_text())
        self.assertEqual(0.0123, target['evaluations'][0]['logloss'])

  def test_campaign_holdout_records_absolute_logloss_for_every_model(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      temp_path = pathlib.Path(temp_dir)
      csv_file = temp_path / 'holdout.csv'
      csv_file.write_text('label\n1\n0\n0\n')

      class Exporter:
        @staticmethod
        def validation_condition():
          return 'validation'

        @staticmethod
        def campaign_condition(campaign_id):
          return 'campaign_id = ' + str(campaign_id)

        @staticmethod
        def export_chunks(*args, **kwargs):
          del args, kwargs

          def chunks():
            yield csv_file, 3

          return chunks()

      class Trainer:
        @staticmethod
        def evaluate_model_(model_file, svm_file, baseline_file=None):
          del model_file, svm_file, baseline_file
          return {'Logloss': 0.01}

        @staticmethod
        def predict_raw_(model_file, svm_file, output_file):
          del model_file, svm_file
          output_file.write_text('0\n0\n0\n')

        @staticmethod
        def evaluate_prediction_weights_(
            model_file,
            svm_file,
            baseline_file,
            weights,
        ):
          del model_file, svm_file, baseline_file
          return [
            {'weight': weight, 'Logloss': 0.02 + index / 100}
            for index, weight in enumerate(weights)
          ]

      def fake_generate_libsvm(
          csv_file,
          svm_file,
          features_config_file,
          dictionary_file=None,
          feature_indexes_file=None,
          feature_stats_file=None,
      ):
        del (
          csv_file,
          features_config_file,
          dictionary_file,
          feature_indexes_file)
        svm_file.write_text('0 1:0\n0 1:0\n1 1:0\n')
        if feature_stats_file is not None:
          feature_stats_file.write_text('0,3,1\n')

      with TRAINER_MODULE.InProgressModel(temp_path / 'models') as progress:
        campaigns = [(123, 100000, 3)]
        progress.publish_post_processing_plan(
          TRAINER_MODULE.post_processing_train_steps(campaigns),
          [{
            'name': 'campaign_123',
            'status': 'planned',
            'db_campaign_id': 123,
            'campaign_name': 'Campaign name',
            'artifact': 'traits/post_processing/campaign_123.json',
          }])
        progress.start_post_processing()
        campaign_models = [
          {
            'name': 'campaign_123',
            'evaluation_model_file': temp_path / 'campaign_123.cbm',
            'traits': {'weight': 0.4},
          },
          {
            'name': 'campaign_456',
            'evaluation_model_file': temp_path / 'campaign_456.cbm',
            'traits': {'weight': 0.7},
          },
        ]
        with unittest.mock.patch.object(
            TRAINER_MODULE,
            'generate_libsvm',
            side_effect=fake_generate_libsvm):
          result = TRAINER_MODULE.evaluate_campaign_holdout(
            exporter=Exporter(),
            work_dir=temp_path,
            campaign_id=123,
            campaign_name='Campaign name',
            date_from='2026-08-01',
            date_to='2026-08-27',
            rows=3,
            offset_rows=6,
            common_features_config_file=temp_path / 'common.json',
            correction_features_config_file=temp_path / 'denoise.json',
            campaign_features_config_file=temp_path / 'campaign.json',
            ssp_ctr_features_config_file=temp_path / 'ssp.json',
            common_feature_indexes_file=temp_path / 'indexes',
            common_model_file=temp_path / 'common.cbm',
            correction_model_file=temp_path / 'denoise.cbm',
            stable_model_file=temp_path / 'stable.cbm',
            ssp_ctr_model_file=temp_path / 'ssp.cbm',
            common_trainer=Trainer(),
            correction_trainer=Trainer(),
            campaign_trainer=Trainer(),
            ssp_ctr_trainer=Trainer(),
            campaign_models=campaign_models,
            progress=progress)

        self.assertEqual(6, len(result['evaluations']))
        self.assertEqual(1, result['dataset']['clicks'])
        self.assertEqual(
          {'common', 'common_denoise', 'common_stable', 'common_ssp_ctr',
           'campaign_123', 'campaign_456'},
          {item['model'] for item in result['evaluations']})
        self.assertFalse(any(
          'gain' in item
          for item in result['evaluations']))
        self.assertEqual(
          0.02,
          result['evaluations'][4]['runtime_logloss'])
        self.assertEqual(
          0.03,
          result['evaluations'][4]['unit_weight_logloss'])

  def test_interrupted_post_processing_target_is_persisted(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      model_root = pathlib.Path(temp_dir)
      with self.assertRaisesRegex(RuntimeError, 'post processing failed'):
        with TRAINER_MODULE.InProgressModel(model_root) as progress:
          progress.publish_post_processing_plan(
            [TRAINER_MODULE.train_step(
              'campaign_123_campaigns',
              'Campaign 123: evaluate campaign models')],
            [{
              'name': 'campaign_123',
              'status': 'planned',
              'db_campaign_id': 123,
              'artifact': 'traits/post_processing/campaign_123.json',
            }])
          progress.start_post_processing()
          progress.write_post_processing_target('campaign_123', {
            'name': 'campaign_123',
            'status': 'training',
            'evaluations': [],
          })
          raise RuntimeError('post processing failed')

      manifest = TRAINER_MODULE.json.loads(
        (progress.path / 'traits.json').read_text())
      index = TRAINER_MODULE.json.loads(
        (progress.path / manifest['post_processing']['artifact']).read_text())
      targets = section_value(index, 'post_processing_results')
      target = TRAINER_MODULE.json.loads(
        (progress.path / targets[0]['artifact']).read_text())
      self.assertEqual('interrupted', manifest['status'])
      self.assertEqual('interrupted', index['status'])
      self.assertEqual('interrupted', targets[0]['status'])
      self.assertEqual('interrupted', target['status'])

  def test_prepare_validation_sets_streams_csv_and_collects_dataset_sizes(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      work_dir = pathlib.Path(temp_dir)
      csv_files = [
        work_dir / 'validation-000.csv',
        work_dir / 'validation-001.csv',
      ]
      for file_path in csv_files:
        file_path.write_text('source\n')
      features_config_file = work_dir / 'features.json'
      features_config_file.write_text('{}')
      feature_indexes_path = work_dir / 'feature-indexes'
      feature_indexes_path.write_text('1\n')
      export_calls = []

      class Exporter:
        @staticmethod
        def validation_condition():
          return 'validation-condition'

        def export_chunks(self, *args, **kwargs):
          export_calls.append((args, kwargs))

          def chunks():
            for file_path in csv_files:
              try:
                yield file_path, 10
              finally:
                file_path.unlink(missing_ok=True)

          return chunks()

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
        validation_files, statistics = (
          TRAINER_MODULE.prepare_validation_libsvm_sets(
          Exporter(),
          work_dir,
          'common-validation',
          features_config_file,
          '2026-08-01',
          '2026-08-02',
          10,
          2,
          validation_offset_rows=30,
          feature_indexes_file=feature_indexes_path,
          collect_statistics=True))

      self.assertEqual(2, len(validation_files))
      self.assertTrue(all(file_path.exists() for file_path in validation_files))
      self.assertFalse(any(file_path.exists() for file_path in csv_files))
      self.assertEqual(30, export_calls[0][1]['offset_rows'])
      self.assertEqual(10, statistics[0].total_impressions)
      self.assertEqual(2, statistics[0].total_clicks)
      self.assertEqual(11, statistics[1].total_impressions)
      self.assertEqual(3, statistics[1].total_clicks)
      self.assertEqual(
        {'rows': 21, 'clicks': 5},
        TRAINER_MODULE.dataset_size(statistics))
      self.assertEqual([], list(work_dir.glob('*.stats')))

  def test_denoise_validation_reexports_common_rows_inside_aligned_phase(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      work_dir = pathlib.Path(temp_dir)
      csv_files = [
        work_dir / 'aligned-source-000.csv',
        work_dir / 'aligned-source-001.csv',
      ]
      common_files = [
        work_dir / 'common-validation-000.libsvm',
        work_dir / 'common-validation-001.libsvm',
      ]
      for file_path in csv_files + common_files:
        file_path.write_text('source\n')
      correction_config = work_dir / 'correction.json'
      correction_config.write_text('{}')
      common_model = work_dir / 'common.cbm'
      common_model.write_text('model\n')
      export_calls = []
      prediction_calls = []

      class Exporter:
        @staticmethod
        def validation_condition():
          return 'validation-condition'

        def export_chunks(self, *args, **kwargs):
          export_calls.append((args, kwargs))

          def chunks():
            for file_path in csv_files:
              try:
                yield file_path, 10
              finally:
                file_path.unlink(missing_ok=True)

          return chunks()

      class CommonTrainer:
        @staticmethod
        def predict_raw_(model_file, svm_file, baseline_file):
          prediction_calls.append((model_file, svm_file, baseline_file))
          baseline_file.write_text('0.1\n')

      def generate_libsvm(
          input_file,
          output_file,
          config_file,
          dictionary_file=None,
          feature_indexes_file=None,
          feature_stats_file=None,
      ):
        del dictionary_file, feature_indexes_file
        self.assertIn(input_file, csv_files)
        self.assertEqual(correction_config, config_file)
        output_file.write_text('1 1:1\n')
        feature_stats_file.write_text('0,10,2\n')

      with unittest.mock.patch.object(
          TRAINER_MODULE,
          'generate_libsvm',
          side_effect=generate_libsvm):
        inputs, statistics = TRAINER_MODULE.prepare_denoise_validation_sets(
          Exporter(),
          work_dir,
          correction_config,
          common_model,
          CommonTrainer(),
          common_files,
          '2026-08-01',
          '2026-08-02',
          10,
          2,
          30)

      self.assertEqual(30, export_calls[0][1]['offset_rows'])
      self.assertEqual(common_files, [call[1] for call in prediction_calls])
      self.assertEqual(2, len(inputs))
      self.assertTrue(all(
        file_path.exists()
        for pair in inputs
        for file_path in pair))
      self.assertFalse(any(file_path.exists() for file_path in csv_files))
      self.assertEqual(
        {'rows': 20, 'clicks': 4},
        TRAINER_MODULE.dataset_size(statistics))
      TRAINER_MODULE.remove_training_inputs(inputs)
      self.assertFalse(any(
        file_path.exists()
        for pair in inputs
        for file_path in pair))

  def test_campaign_validation_uses_campaign_filter_and_offset(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      work_dir = pathlib.Path(temp_dir)
      csv_files = [
        work_dir / 'campaign-source-000.csv',
        work_dir / 'campaign-source-001.csv',
      ]
      for file_path in csv_files:
        file_path.write_text('source\n')
      stable_config = work_dir / 'stable.json'
      campaign_config = work_dir / 'campaign.json'
      stable_indexes = work_dir / 'stable.indexes'
      campaign_indexes = work_dir / 'campaign.indexes'
      stable_model = work_dir / 'stable.cbm'
      for file_path in (
          stable_config,
          campaign_config,
          stable_indexes,
          campaign_indexes,
          stable_model,
      ):
        file_path.write_text('data\n')
      export_calls = []

      class Exporter:
        @staticmethod
        def validation_condition():
          return 'validation-condition'

        @staticmethod
        def campaign_condition(campaign_id):
          self.assertEqual(17, campaign_id)
          return 'campaign-condition'

        def export_chunks(self, *args, **kwargs):
          export_calls.append((args, kwargs))

          def chunks():
            for file_path in csv_files:
              try:
                yield file_path, 10
              finally:
                file_path.unlink(missing_ok=True)

          return chunks()

      class StableTrainer:
        @staticmethod
        def predict_raw_(model_file, svm_file, baseline_file):
          self.assertEqual(stable_model, model_file)
          self.assertTrue(svm_file.exists())
          baseline_file.write_text('0.1\n')

      def generate_libsvm(
          input_file,
          output_file,
          config_file,
          dictionary_file=None,
          feature_indexes_file=None,
          feature_stats_file=None,
      ):
        del input_file, dictionary_file
        output_file.write_text('1 1:1\n')
        if config_file == stable_config:
          self.assertEqual(stable_indexes, feature_indexes_file)
          self.assertIsNone(feature_stats_file)
        else:
          self.assertEqual(campaign_config, config_file)
          self.assertEqual(campaign_indexes, feature_indexes_file)
          feature_stats_file.write_text('0,10,2\n')

      with unittest.mock.patch.object(
          TRAINER_MODULE,
          'generate_libsvm',
          side_effect=generate_libsvm):
        inputs, statistics = TRAINER_MODULE.prepare_campaign_validation_sets(
          Exporter(),
          work_dir,
          17,
          stable_config,
          campaign_config,
          stable_indexes,
          stable_model,
          StableTrainer(),
          '2026-08-01',
          '2026-08-02',
          10,
          2,
          campaign_feature_indexes_file=campaign_indexes,
          validation_offset_rows=30)

      self.assertEqual(30, export_calls[0][1]['offset_rows'])
      self.assertEqual(2, len(inputs))
      self.assertEqual({'rows': 20, 'clicks': 4},
                       TRAINER_MODULE.dataset_size(statistics))
      TRAINER_MODULE.remove_training_inputs(inputs)

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

  def test_supervisor_starts_only_trainer(self):
    process = unittest.mock.MagicMock()
    process.poll.return_value = 1
    with (
        unittest.mock.patch.object(
          MODULE,
          'start_child',
          return_value=process) as start_child,
        unittest.mock.patch.object(MODULE, 'stop_children') as stop_children):
      with self.assertRaisesRegex(RuntimeError, 'trainer exited'):
        MODULE.supervise('/tmp/config.json')

    start_child.assert_called_once()
    self.assertEqual('trainer', start_child.call_args.args[0])
    self.assertTrue(
      start_child.call_args.args[1][1].endswith(
        '/bin/CTRPredictModelTrainer.py'))
    stop_children.assert_called_once_with([('trainer', process)])


if __name__ == '__main__':
  unittest.main()
