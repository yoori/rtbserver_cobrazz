#!/usr/bin/env python3.12

import json
import os
import pathlib
import sys
import tempfile
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from rtbserver_utils.CTRModelRepository import CTRModelRepository, ModelNotFound


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

  def create_in_progress_model(self, root, model_id, pid=None):
    model_dir = root / model_id
    model_dir.mkdir()
    (model_dir / 'traits.json').write_text(json.dumps({
      'status': 'in_progress',
      'train_start': '2026-08-24T15:55:15Z',
      'pid': os.getpid() if pid is None else pid,
    }))
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
      self.create_in_progress_model(root, '~20260824.155515')
      self.create_in_progress_model(root, '~20260824.140000', pid=99999999)

      repository = CTRModelRepository(root)

      self.assertEqual([
        '~20260824.155515',
        '20260823.120000',
      ], repository.all_model_ids())
      summary = repository.model_summary('~20260824.155515')
      self.assertEqual('in_progress', summary['status'])
      self.assertEqual('2026-08-24T15:55:15Z', summary['train_start'])
      properties = repository.model_properties('~20260824.155515')
      self.assertEqual({}, properties['config'])
      self.assertEqual({}, properties['traits'])
      self.assertEqual('20260823.120000', repository.latest_model_id())

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
