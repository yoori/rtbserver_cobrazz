import decimal
import json
import pathlib


class ModelNotFound(Exception):
  pass


class CTRModelRepository:
  REQUIRED_FILES = ('model.cbm', 'config.json', 'traits.json')

  def __init__(self, model_root):
    self.model_root = pathlib.Path(model_root)

  def model_ids(self):
    if not self.model_root.is_dir():
      return []
    return sorted(
      (
        path.name
        for path in self.model_root.iterdir()
        if self.is_published_model_(path)
      ),
      reverse=True)

  def latest_model_id(self):
    model_ids = self.model_ids()
    if not model_ids:
      raise ModelNotFound('No published CTR models')
    return model_ids[0]

  def model_summary(self, model_id):
    model_path = self.model_path(model_id)
    config = self.read_json_(model_path / 'config.json')
    traits = self.read_json_(model_path / 'traits.json')
    algorithm = self.first_(config.get('algorithms'))
    model = self.first_(algorithm.get('models')) if algorithm else None
    feature_groups = model.get('features', []) if model else []
    feature_importance = traits.get('features_importance', [])
    return {
      'id': model_id,
      'algorithm_id': algorithm.get('id') if algorithm else None,
      'method': model.get('method') if model else None,
      'feature_groups': feature_groups,
      'feature_groups_count': len(feature_groups),
      'features_importance_count': len(feature_importance),
    }

  def model_properties(self, model_id):
    model_path = self.model_path(model_id)
    return {
      'summary': self.model_summary(model_id),
      'config': self.read_json_(model_path / 'config.json'),
      'traits': self.read_json_(model_path / 'traits.json'),
    }

  def features(self, model_id, offset, limit):
    model_path = self.model_path(model_id)
    traits = self.read_json_(model_path / 'traits.json')
    features = traits.get('features_importance', [])
    return {
      'model_id': model_id,
      'offset': offset,
      'limit': limit,
      'total': len(features),
      'items': features[offset:offset + limit],
    }

  def model_file(self, model_id, file_name):
    if file_name not in self.REQUIRED_FILES:
      raise ModelNotFound('Unknown model file')
    path = self.model_path(model_id) / file_name
    if not path.is_file():
      raise ModelNotFound("Model file '" + file_name + "' not found")
    return path

  def model_path(self, model_id):
    if not isinstance(model_id, str) or not model_id or model_id.startswith('~'):
      raise ModelNotFound('Invalid model id')
    model_path = self.model_root / model_id
    try:
      if model_path.resolve().parent != self.model_root.resolve():
        raise ModelNotFound('Invalid model id')
    except FileNotFoundError:
      raise ModelNotFound("Model '" + model_id + "' not found")
    if not self.is_published_model_(model_path):
      raise ModelNotFound("Model '" + model_id + "' not found")
    return model_path

  def is_published_model_(self, path):
    return (
      path.is_dir() and
      not path.is_symlink() and
      not path.name.startswith('~') and
      all((path / name).is_file() for name in self.REQUIRED_FILES))

  @staticmethod
  def read_json_(file_name):
    try:
      with file_name.open() as input_file:
        return json.load(input_file, parse_float=decimal.Decimal)
    except (OSError, ValueError) as error:
      raise RuntimeError("Can't read '" + str(file_name) + "': " + str(error))

  @staticmethod
  def first_(values):
    if isinstance(values, list) and values:
      return values[0]
    return None
