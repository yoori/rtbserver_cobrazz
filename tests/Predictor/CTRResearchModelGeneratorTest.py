#!/usr/bin/env python3.12

import datetime
import importlib.util
import json
import pathlib
import signal
import sys
import tempfile
import unittest
import unittest.mock


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))
sys.path.insert(0, str(SOURCE_ROOT / 'bin'))

from CTRPredictModelTrainer import InProgressModel
from CTRResearchModelTrainer import (
  RESEARCH_SUFFIX,
  SSP_CTR_FEATURE_CONFIG,
  add_ctr_thresholds,
  finalize_ctr_thresholds,
  latest_production_model_id,
  research_prepare_steps,
  ssp_ctr_threshold_statistics,
  ssp_ctr_train_steps,
)
from rtbserver_utils.CTRPredictModelGeneratorConfig import Config


MODULE_FILE = SOURCE_ROOT / 'bin' / 'CTRResearchModelGenerator.py'
SPEC = importlib.util.spec_from_file_location(
  'ctr_research_model_generator', MODULE_FILE)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class CTRResearchModelGeneratorTest(unittest.TestCase):
  def test_research_feature_config_excludes_target_and_campaign_identity(self):
    features = SSP_CTR_FEATURE_CONFIG['features']

    self.assertIn(['publisher'], features)
    self.assertIn(['ssp_tag_id'], features)
    self.assertIn(['ssp_viewability'], features)
    self.assertIn(['ssp_vtr'], features)
    self.assertNotIn(['ssp_ctr'], features)
    self.assertNotIn(['campaign'], features)
    self.assertNotIn(['group'], features)
    self.assertNotIn(['ccid'], features)

  def test_training_plan_has_independent_prepare_and_model_steps(self):
    config = Config()

    prepare_ids = [step['id'] for step in research_prepare_steps()]
    model_ids = [
      step['id']
      for step in ssp_ctr_train_steps(
        config,
        selection_fit_steps=2,
        training_fit_steps=3)
    ]

    self.assertEqual([
      'prepare_feature_config',
      'find_date_range',
      'fit_row_counts',
      'count_available_rows',
    ], prepare_ids)
    self.assertIn('ssp_selection_validation_libsvm_001', model_ids)
    self.assertIn('ssp_feature_selection_thresholds_001', model_ids)
    self.assertIn('ssp_feature_selection_fit_002', model_ids)
    self.assertNotIn('ssp_feature_selection_fit_003', model_ids)
    self.assertIn('prune_correlated_feature_indexes', model_ids)
    self.assertIn('deduplicate_feature_indexes', model_ids)
    self.assertIn('ssp_training_fit_003', model_ids)
    self.assertNotIn('ssp_training_fit_004', model_ids)
    self.assertIn('finalize_metrics', model_ids)

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

      result = ssp_ctr_threshold_statistics(csv_file)

      self.assertEqual(5, result['rows'])
      self.assertEqual(3, result['clicks'])
      self.assertEqual(0.0, result['ctr_thresholds'][0]['ctr_goal'])
      self.assertEqual(5, result['ctr_thresholds'][0]['impressions'])
      self.assertEqual(3, result['ctr_thresholds'][0]['clicks'])
      self.assertAlmostEqual(
        0.0636,
        result['ctr_thresholds'][0]['predicted_ctr_sum'])
      self.assertEqual(0.001, result['ctr_thresholds'][1]['ctr_goal'])
      self.assertEqual(4, result['ctr_thresholds'][1]['impressions'])
      aggregate = add_ctr_thresholds(None, result['ctr_thresholds'])
      aggregate = add_ctr_thresholds(aggregate, result['ctr_thresholds'])
      finalized = finalize_ctr_thresholds(aggregate)
      self.assertEqual(10, finalized[0]['impressions'])
      self.assertEqual(6, finalized[0]['clicks'])
      self.assertEqual(100, finalized[0]['share'])
      self.assertAlmostEqual(0.01272, finalized[0]['average_predicted_ctr'])
      self.assertEqual(80, finalized[1]['share'])

  def test_in_progress_model_uses_research_suffix_and_traits(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      model_root = pathlib.Path(temp_dir) / 'CTRResearch'
      train_start = datetime.datetime(
        2026,
        9,
        3,
        14,
        25,
        21,
        tzinfo=datetime.timezone.utc)

      with InProgressModel(
          model_root,
          train_start,
          prepare_steps=research_prepare_steps(),
          model_suffix=RESEARCH_SUFFIX,
          root_traits={
            'model_type': 'research',
            'research_type': 'common_ssp_ctr',
            'parent_model_id': '20260903.120000',
          }) as in_progress:
        self.assertEqual(
          '20260903.142521.SSP-CTR-CHECK',
          in_progress.model_id)
        traits = json.loads((in_progress.path / 'traits.json').read_text())
        self.assertEqual('research', traits['model_type'])
        self.assertEqual('common_ssp_ctr', traits['research_type'])
        self.assertEqual('20260903.120000', traits['parent_model_id'])
        self.assertEqual('in_progress', traits['status'])

      self.assertFalse(model_root.joinpath(
        '~20260903.142521.SSP-CTR-CHECK').exists())

  def test_latest_production_model_ignores_research_root(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      workspace_root = pathlib.Path(temp_dir)
      production_root = workspace_root / 'log' / 'Predictor' / 'CTRConfig'
      research_root = workspace_root / 'log' / 'Predictor' / 'CTRResearch'
      production_root.mkdir(parents=True)
      research_root.mkdir(parents=True)
      self.create_model(production_root, '20260903.120000')
      self.create_model(research_root, '20260904.120000.SSP-CTR-CHECK')
      config = Config()
      config.workspace_root = str(workspace_root)

      self.assertEqual('20260903.120000', latest_production_model_id(config))

  def test_config_has_separate_research_model_root(self):
    config = Config()
    config.workspace_root = '/var/lib/foros'

    self.assertEqual(
      pathlib.Path('/var/lib/foros/log/Predictor/CTRResearch'),
      config.research_model_root())

  def test_child_command_uses_research_trainer(self):
    command = MODULE.child_command('/tmp/config.json', run_once=True)

    self.assertTrue(command[1].endswith('/bin/CTRResearchModelTrainer.py'))
    self.assertEqual('--config=/tmp/config.json', command[2])
    self.assertEqual('--run-once', command[3])

  def test_supervisor_stops_research_trainer_after_failure(self):
    process = unittest.mock.MagicMock()
    process.poll.return_value = 1
    with (
        unittest.mock.patch.object(
          MODULE,
          'start_child',
          return_value=process) as start_child,
        unittest.mock.patch.object(MODULE, 'stop_child') as stop_child):
      with self.assertRaisesRegex(RuntimeError, 'research trainer exited'):
        MODULE.supervise('/tmp/config.json')

    start_child.assert_called_once()
    stop_child.assert_called_once_with(process)

  def test_stop_child_signals_process_group(self):
    process = unittest.mock.MagicMock()
    process.pid = 12345

    with unittest.mock.patch.object(MODULE.os, 'killpg') as killpg:
      MODULE.stop_child(process)

    killpg.assert_called_once_with(12345, signal.SIGTERM)

  @staticmethod
  def create_model(root, model_id):
    model_dir = root / model_id
    model_dir.mkdir()
    (model_dir / 'model.cbm').write_bytes(b'model')
    (model_dir / 'config.json').write_text('{}')
    (model_dir / 'traits.json').write_text('{}')


if __name__ == '__main__':
  unittest.main()
