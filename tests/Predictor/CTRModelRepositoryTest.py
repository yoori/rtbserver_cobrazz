#!/usr/bin/env python3.12

import json
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
      'features_importance': [{
        'score': score,
        'feature': 'channel:1',
        'name': 'Account/Channel',
      }],
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

  def test_returns_properties_and_paginated_features(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      root = pathlib.Path(temp_dir)
      self.create_model(root, '20260822.120000', 0.00001)
      repository = CTRModelRepository(root)

      properties = repository.model_properties('20260822.120000')
      self.assertEqual('catboost', properties['summary']['algorithm_id'])
      self.assertEqual(2, properties['summary']['feature_groups_count'])
      self.assertEqual(1, properties['summary']['features_importance_count'])
      features = repository.features('20260822.120000', 0, 100)
      self.assertEqual(1, features['total'])
      self.assertEqual('channel:1', features['items'][0]['feature'])

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
