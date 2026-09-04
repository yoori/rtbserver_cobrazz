#!/usr/bin/env python3.12

import decimal
import json
import os
import pathlib
import sys
import tempfile
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from rtbserver_utils.CTRModelRepository import CTRModelRepository, ModelNotFound
from rtbserver_utils.CTRModelTraits import section_value, traits_with_sections


class CTRModelRepositoryTest(unittest.TestCase):
  def create_model(self, root, model_id, score=1.0):
    model_dir = root / model_id
    model_dir.mkdir()
    (model_dir / 'model.cbm').write_bytes(b'model')
    (model_dir / 'config.json').write_text(json.dumps({
      'algorithms': [{
        'id': 'catboost',
        'models': [{
          'method': 'catboost',
          'features': [['publisher'], ['channel']],
        }],
      }],
    }))
    (model_dir / 'traits.json').write_text(json.dumps({
      'train_start': '2026-08-22T12:00:00Z',
      'train_end': '2026-08-22T13:30:00Z',
      'features_importance': [{
        'score': score,
        'feature': 'channel:1',
        'name': 'Account/Channel',
      }],
    }))
    return model_dir

  def create_in_progress_model(self, root, model_id, pid=None, models=None):
    model_dir = root / model_id
    model_dir.mkdir()
    traits = {
      'status': 'in_progress',
      'train_start': '2026-08-24T15:55:15Z',
      'pid': os.getpid() if pid is None else pid,
    }
    if models is not None:
      traits['models'] = models
    (model_dir / 'traits.json').write_text(json.dumps(traits))
    return model_dir

  def test_lists_only_atomically_published_models(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      self.create_model(root, '20260822.120000')
      self.create_model(root, '20260823.120000')
      self.create_model(root, '~20260824.120000')
      incomplete = root / '20260825.120000'
      incomplete.mkdir()
      (incomplete / 'config.json').write_text('{}')

      repository = CTRModelRepository(root)
      self.assertEqual([
        '20260823.120000',
        '20260822.120000',
      ], repository.model_ids())
      self.assertEqual('20260823.120000', repository.latest_model_id())

  def test_lists_live_in_progress_models_before_published_models(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      self.create_model(root, '20260823.120000')
      self.create_in_progress_model(root, '~20260824.155515', models=[
        {'name': 'common', 'kind': 'common', 'status': 'completed'},
        {
          'name': 'campaign_123',
          'kind': 'campaign',
          'status': 'training',
        },
      ])
      self.create_in_progress_model(
        root,
        '~20260824.140000',
        pid=99999999,
        models=[{
          'name': 'common_stable',
          'kind': 'common_stable',
          'status': 'training',
        }])

      repository = CTRModelRepository(root)

      self.assertEqual([
        '~20260824.155515',
        '~20260824.140000',
        '20260823.120000',
      ], repository.all_model_ids())
      summary = repository.model_summary('~20260824.155515')
      self.assertEqual('in_progress', summary['status'])
      self.assertEqual('2026-08-24T15:55:15Z', summary['train_start'])
      self.assertEqual(2, summary['models_count'])
      self.assertEqual(1, summary['campaign_models_count'])
      self.assertEqual(1, summary['completed_models_count'])
      properties = repository.model_properties('~20260824.155515')
      self.assertEqual({}, properties['config'])
      self.assertEqual(2, len(properties['traits']['models']))

      interrupted = repository.model_properties('~20260824.140000')
      self.assertEqual('interrupted', interrupted['summary']['status'])
      self.assertEqual(1, interrupted['summary']['interrupted_models_count'])
      self.assertEqual(
        'process_not_running',
        interrupted['traits']['interruption_reason'])
      self.assertEqual(
        'interrupted',
        interrupted['traits']['models'][0]['status'])
      self.assertEqual('20260823.120000', repository.latest_model_id())

  def test_sorts_training_and_published_models_without_tilde_prefix(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      self.create_model(root, '20260825.120000')
      self.create_in_progress_model(root, '~20260824.155515', pid=os.getpid())

      repository = CTRModelRepository(root)

      self.assertEqual([
        '20260825.120000',
        '~20260824.155515',
      ], repository.all_model_ids())

  def test_combines_research_models_without_changing_latest_production(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      production_root = root / 'CTRConfig'
      research_root = root / 'CTRResearch'
      production_root.mkdir()
      research_root.mkdir()
      self.create_model(production_root, '20260903.120000')
      research = self.create_model(
        research_root,
        '20260903.142521.SSP-CTR-CHECK')
      traits = json.loads((research / 'traits.json').read_text())
      traits.update({
        'model_type': 'research',
        'research_type': 'common_ssp_ctr',
        'parent_model_id': '20260903.120000',
      })
      (research / 'traits.json').write_text(json.dumps(traits))

      repository = CTRModelRepository(production_root, research_root)

      self.assertEqual([
        '20260903.142521.SSP-CTR-CHECK',
        '20260903.120000',
      ], repository.all_model_ids())
      self.assertEqual(['20260903.120000'], repository.model_ids())
      self.assertEqual('20260903.120000', repository.latest_model_id())
      summary = repository.model_summary(
        '20260903.142521.SSP-CTR-CHECK')
      self.assertEqual('research', summary['model_type'])
      self.assertEqual('common_ssp_ctr', summary['research_type'])
      self.assertEqual('20260903.120000', summary['parent_model_id'])
      self.assertEqual(
        research,
        repository.model_path('20260903.142521.SSP-CTR-CHECK'))
      self.assertEqual(
        research / 'model.cbm',
        repository.model_file(
          '20260903.142521.SSP-CTR-CHECK',
          'model.cbm'))

  def test_returns_properties_and_paginated_features(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      self.create_model(root, '20260822.120000', 0.00001)
      repository = CTRModelRepository(root)

      properties = repository.model_properties('20260822.120000')
      self.assertEqual('catboost', properties['summary']['algorithm_id'])
      self.assertEqual('published', properties['summary']['status'])
      self.assertEqual(
        '2026-08-22T12:00:00Z',
        properties['summary']['train_start'])
      self.assertEqual(
        '2026-08-22T13:30:00Z',
        properties['summary']['train_end'])
      self.assertEqual(2, properties['summary']['feature_groups_count'])
      self.assertEqual(1, properties['summary']['features_importance_count'])
      features = repository.features('20260822.120000', 0, 100)
      self.assertEqual(1, features['total'])
      self.assertEqual('channel:1', features['items'][0]['feature'])

  def test_reads_component_traits_and_selects_component_features(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      model_dir = self.create_model(root, '20260824.120000')
      (model_dir / 'common.cbm').write_bytes(b'common')
      (model_dir / 'campaign-correction.cbm').write_bytes(b'correction')
      (model_dir / 'traits.json').write_text(json.dumps({
        'status': 'published',
        'training_pipeline': {'published_component': 'stable_common'},
        'components': {
          'common': {
            'features_importance': [
              {'score': 1, 'feature': 'publisher:1'},
            ],
          },
          'campaign_correction': {
            'features_importance': [
              {'score': 2, 'feature': 'campaign:2'},
            ],
          },
          'stable_common': {
            'features_importance': [
              {'score': 3, 'feature': 'tag:3'},
            ],
          },
        },
      }))
      repository = CTRModelRepository(root)

      summary = repository.model_summary('20260824.120000')
      self.assertEqual(3, summary['components_count'])
      self.assertEqual('stable_common', summary['published_component'])
      self.assertEqual(1, summary['features_importance_count'])
      default_features = repository.features(
        '20260824.120000', 0, 100)
      self.assertEqual('tag:3', default_features['items'][0]['feature'])
      correction_features = repository.features(
        '20260824.120000', 0, 100, 'campaign_correction')
      self.assertEqual(
        'campaign:2',
        correction_features['items'][0]['feature'])
      self.assertEqual(
        b'common',
        repository.model_file(
          '20260824.120000', 'common.cbm').read_bytes())

  def test_reads_flat_model_list_and_published_common_stable(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      model_dir = self.create_model(root, '20260825.120000')
      (model_dir / 'traits.json').write_text(json.dumps({
        'training_pipeline': {'published_model': 'common_stable'},
        'models': [
          {
            'name': 'common',
            'features_importance': [],
          },
          {
            'name': 'common_denoise',
            'features_importance': [],
          },
          {
            'name': 'common_stable',
            'features_importance': [
              {'score': 3, 'feature': 'tag:3'},
            ],
          },
          {
            'name': 'campaign_123',
            'db_campaign_id': 123,
            'campaign_name': 'Campaign name',
            'features_importance': [
              {'score': 4, 'feature': 'ccid:4'},
            ],
          },
        ],
      }))
      repository = CTRModelRepository(root)

      summary = repository.model_summary('20260825.120000')
      self.assertEqual(4, summary['models_count'])
      self.assertEqual('common_stable', summary['published_component'])
      self.assertEqual(1, summary['features_importance_count'])
      campaign_features = repository.features(
        '20260825.120000', 0, 100, 'campaign_123')
      self.assertEqual('ccid:4', campaign_features['items'][0]['feature'])

  def test_reads_manifest_artifacts_lazily(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      model_dir = self.create_model(root, '20260826.120000')
      (model_dir / 'traits' / 'models').mkdir(parents=True)
      (model_dir / 'traits' / 'post_processing').mkdir()
      stable_artifact = 'traits/models/common_stable.json'
      (model_dir / stable_artifact).write_text(json.dumps(traits_with_sections({
        'name': 'common_stable',
        'kind': 'common_stable',
        'features_importance': [{
          'score': 3,
          'feature': 'tag:3',
        }],
        'properties': [{'val_logloss': 0.01}],
        'train_steps': [],
      })))
      post_index = 'traits/post_processing/index.json'
      target_artifact = 'traits/post_processing/campaign_123.json'
      (model_dir / post_index).write_text(json.dumps(traits_with_sections({
        'name': 'post_processing',
        'kind': 'post_processing',
        'targets': [{
          'name': 'campaign_123',
          'artifact': target_artifact,
        }],
      })))
      (model_dir / target_artifact).write_text(json.dumps({
        'name': 'campaign_123',
        'evaluations': [{
          'model': 'common_stable',
          'logloss': 0.0123,
        }],
      }))
      (model_dir / 'traits.json').write_text(json.dumps({
        'traits_version': 2,
        'status': 'published',
        'training_pipeline': {'published_model': 'common_stable'},
        'models': [{
          'name': 'common_stable',
          'kind': 'common_stable',
          'artifact': stable_artifact,
          'features_importance_count': 1,
        }],
        'post_processing': {
          'name': 'post_processing',
          'kind': 'post_processing',
          'artifact': post_index,
          'targets_count': 1,
        },
      }))
      repository = CTRModelRepository(root)

      properties = repository.model_properties('20260826.120000')
      self.assertNotIn(
        'features_importance',
        properties['traits']['models'][0])
      self.assertEqual(
        1,
        properties['summary']['features_importance_count'])
      stable = repository.component_traits(
        '20260826.120000',
        'common_stable')
      self.assertEqual(
        decimal.Decimal('0.01'),
        section_value(stable, 'properties')[0]['val_logloss'])
      features = repository.features(
        '20260826.120000',
        0,
        10,
        'common_stable')
      self.assertEqual('tag:3', features['items'][0]['feature'])
      target = repository.post_processing_target(
        '20260826.120000',
        'campaign_123')
      self.assertEqual(
        decimal.Decimal('0.0123'),
        target['evaluations'][0]['logloss'])

  def test_rejects_path_traversal_and_symlinks(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir) / 'models'
      root.mkdir()
      external = pathlib.Path(temp_dir) / 'external'
      self.create_model(pathlib.Path(temp_dir), 'external')
      (root / 'linked').symlink_to(external, target_is_directory=True)
      repository = CTRModelRepository(root)

      with self.assertRaises(ModelNotFound):
        repository.model_path('../external')
      with self.assertRaises(ModelNotFound):
        repository.model_path('linked')


if __name__ == '__main__':
  unittest.main()
