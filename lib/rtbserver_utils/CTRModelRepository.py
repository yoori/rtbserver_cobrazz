import decimal
import json
import os
import pathlib

from rtbserver_utils.CTRModelTraits import section_value


class ModelNotFound(Exception):
  pass


class CTRModelRepository:
  REQUIRED_FILES = ('model.cbm', 'config.json', 'traits.json')
  COMPONENT_MODEL_FILES = (
    'common.cbm',
    'common_denoise.cbm',
    'campaign-correction.cbm',
  )

  def __init__(self, model_root, research_model_root=None):
    self.model_root = pathlib.Path(model_root)
    self.model_roots = [self.model_root]
    if research_model_root is not None:
      research_model_root = pathlib.Path(research_model_root)
      if research_model_root != self.model_root:
        self.model_roots.append(research_model_root)

  def model_ids(self):
    if not self.model_root.is_dir():
      return []
    return sorted(
      (
        path.name
        for path in self.model_root.iterdir()
        if self.is_published_model_(path)
      ),
      key=self.model_id_sort_key_,
      reverse=True)

  def all_model_ids(self):
    model_ids = {
      path.name
      for model_root in self.model_roots
      if model_root.is_dir()
      for path in model_root.iterdir()
      if (
        self.training_status_(path.name) is not None or
        self.is_published_model_(path))
    }
    return sorted(model_ids, key=self.model_id_sort_key_, reverse=True)

  @staticmethod
  def model_id_sort_key_(model_id):
    return model_id.removeprefix('~')

  def latest_model_id(self):
    model_ids = self.model_ids()
    if not model_ids:
      raise ModelNotFound('No published CTR models')
    return model_ids[0]

  def model_summary(self, model_id):
    training_status = self.training_status_(model_id)
    if training_status is not None:
      models = training_status.get('models')
      if not isinstance(models, list):
        models = []
      campaign_models = [
        model
        for model in models
        if isinstance(model, dict) and model.get('kind') == 'campaign'
      ]
      return {
        'id': model_id,
        'status': training_status['status'],
        'model_type': training_status.get('model_type', 'production'),
        'research_type': training_status.get('research_type'),
        'parent_model_id': training_status.get('parent_model_id'),
        'train_start': training_status['train_start'],
        'train_end': training_status.get('train_end'),
        'models_count': len(models),
        'campaign_models_count': len(campaign_models),
        'completed_models_count': sum(
          1
          for model in models
          if isinstance(model, dict) and model.get('status') == 'completed'),
        'interrupted_models_count': sum(
          1
          for model in models
          if isinstance(model, dict) and model.get('status') == 'interrupted'),
      }

    model_path = self.model_path(model_id)
    config = self.read_json_(model_path / 'config.json')
    traits = self.read_json_(model_path / 'traits.json')
    algorithm = self.first_(config.get('algorithms'))
    model = self.first_(algorithm.get('models')) if algorithm else None
    feature_groups = model.get('features', []) if model else []
    trait_models = traits.get('models')
    if not isinstance(trait_models, list):
      trait_models = []
    trait_models_by_name = {
      item.get('name'): item
      for item in trait_models
      if isinstance(item, dict) and isinstance(item.get('name'), str)
    }
    components = traits.get('components')
    if not isinstance(components, dict):
      components = {}
    training_pipeline = traits.get('training_pipeline')
    if not isinstance(training_pipeline, dict):
      training_pipeline = {}
    published_component = training_pipeline.get(
      'published_model',
      training_pipeline.get('published_component', 'stable_common'))
    published_traits = trait_models_by_name.get(
      published_component,
      components.get(published_component, traits))
    if not isinstance(published_traits, dict):
      published_traits = traits
    feature_importance = published_traits.get('features_importance', [])
    features_importance_count = published_traits.get(
      'features_importance_count')
    if not isinstance(features_importance_count, int):
      features_importance_count = len(feature_importance)
    return {
      'id': model_id,
      'status': 'published',
      'model_type': traits.get('model_type', 'production'),
      'research_type': traits.get('research_type'),
      'parent_model_id': traits.get('parent_model_id'),
      'train_start': traits.get('train_start'),
      'train_end': traits.get('train_end'),
      'algorithm_id': algorithm.get('id') if algorithm else None,
      'method': model.get('method') if model else None,
      'feature_groups': feature_groups,
      'feature_groups_count': len(feature_groups),
      'features_importance_count': features_importance_count,
      'component_names': list(components),
      'components_count': len(trait_models) or len(components),
      'model_names': list(trait_models_by_name),
      'models_count': len(trait_models),
      'published_component': (
        published_component if trait_models or components else None),
    }

  def model_properties(self, model_id):
    training_status = self.training_status_(model_id)
    if training_status is not None:
      return {
        'summary': self.model_summary(model_id),
        'config': {},
        'traits': training_status,
      }

    model_path = self.model_path(model_id)
    return {
      'summary': self.model_summary(model_id),
      'config': self.read_json_(model_path / 'config.json'),
      'traits': self.read_json_(model_path / 'traits.json'),
    }

  def component_traits(self, model_id, component):
    model_path, traits = self.model_directory_and_traits_(model_id)
    source = None
    if component == 'prepare':
      source = traits.get('prepare')
    elif component == 'post_processing':
      source = traits.get('post_processing')
    else:
      trait_models = traits.get('models')
      if isinstance(trait_models, list):
        source = next((
          item
          for item in trait_models
          if isinstance(item, dict) and item.get('name') == component
        ), None)
      if source is None:
        components = traits.get('components')
        if isinstance(components, dict):
          source = components.get(component)
    if not isinstance(source, dict):
      raise ModelNotFound("Unknown model component '" + component + "'")

    artifact_path = source.get('artifact')
    if not isinstance(artifact_path, str):
      return dict(source)
    artifact = self.read_artifact_(model_path, artifact_path)
    if not isinstance(artifact, dict):
      raise RuntimeError(
        "Model artifact '" + artifact_path + "' must contain an object")
    result = {
      **artifact,
      **source,
      '_artifact_loaded': True,
    }
    if component == 'post_processing' and result.get('status') == 'interrupted':
      targets = section_value(
        result,
        'post_processing_results')
      if isinstance(targets, list):
        interrupted_targets = [
          {
            **target,
            **(
              {'status': 'interrupted'}
              if (
                isinstance(target, dict) and
                target.get('status') == 'training') else {}),
          }
          if isinstance(target, dict) else target
          for target in targets
        ]
        if 'targets' in result:
          result['targets'] = interrupted_targets
        else:
          for section in result.get('sections', []):
            if (
                isinstance(section, dict) and
                section.get('id') == 'post_processing_results' and
                isinstance(section.get('data'), dict)):
              section['data']['targets'] = interrupted_targets
              break
    return result

  def post_processing_target(self, model_id, target_name):
    model_path, _ = self.model_directory_and_traits_(model_id)
    post_processing = self.component_traits(model_id, 'post_processing')
    targets = section_value(
      post_processing,
      'post_processing_results')
    if not isinstance(targets, list):
      raise ModelNotFound('Post-processing results are not available')
    target = next((
      item
      for item in targets
      if isinstance(item, dict) and item.get('name') == target_name
    ), None)
    if target is None:
      raise ModelNotFound(
        "Unknown post-processing target '" + target_name + "'")
    artifact_path = target.get('artifact')
    if not isinstance(artifact_path, str):
      return dict(target)
    artifact = self.read_artifact_(model_path, artifact_path)
    if not isinstance(artifact, dict):
      raise RuntimeError(
        "Post-processing artifact '" + artifact_path +
        "' must contain an object")
    return {**artifact, **target}

  def features(self, model_id, offset, limit, component=None):
    model_path = self.model_path(model_id)
    traits = self.read_json_(model_path / 'traits.json')
    source_traits = traits
    trait_models = traits.get('models')
    if isinstance(trait_models, list):
      models_by_name = {
        item.get('name'): item
        for item in trait_models
        if isinstance(item, dict) and isinstance(item.get('name'), str)
      }
      if component is None:
        training_pipeline = traits.get('training_pipeline')
        if not isinstance(training_pipeline, dict):
          training_pipeline = {}
        component = training_pipeline.get('published_model', 'common_stable')
      if component not in models_by_name:
        raise ModelNotFound("Unknown model component '" + component + "'")
      source_traits = self.component_traits(model_id, component)
    components = traits.get('components')
    if not isinstance(trait_models, list) and component is not None:
      if not isinstance(components, dict) or component not in components:
        raise ModelNotFound("Unknown model component '" + component + "'")
      source_traits = components[component]
    elif not isinstance(trait_models, list) and isinstance(components, dict):
      training_pipeline = traits.get('training_pipeline')
      if not isinstance(training_pipeline, dict):
        training_pipeline = {}
      published_component = training_pipeline.get(
        'published_component', 'stable_common')
      source_traits = components.get(published_component, {})
    if not isinstance(source_traits, dict):
      source_traits = {}
    features = section_value(
      source_traits,
      'feature_importance',
      [])
    if not isinstance(features, list):
      features = []
    return {
      'model_id': model_id,
      'component': component,
      'offset': offset,
      'limit': limit,
      'total': len(features),
      'items': features[offset:offset + limit],
    }

  def model_file(self, model_id, file_name):
    model_path = self.model_path(model_id)
    allowed_files = set(self.REQUIRED_FILES + self.COMPONENT_MODEL_FILES)
    traits = self.read_json_(model_path / 'traits.json')
    trait_models = traits.get('models')
    if isinstance(trait_models, list):
      allowed_files.update(
        item['file']
        for item in trait_models
        if (
          isinstance(item, dict) and
          isinstance(item.get('file'), str) and
          '/' not in item['file'] and
          item['file'] not in ('.', '..')))
    if file_name not in allowed_files:
      raise ModelNotFound('Unknown model file')
    path = model_path / file_name
    if not path.is_file():
      raise ModelNotFound("Model file '" + file_name + "' not found")
    return path

  def model_path(self, model_id):
    if not isinstance(model_id, str) or not model_id or model_id.startswith('~'):
      raise ModelNotFound('Invalid model id')
    for model_root in self.model_roots:
      model_path = model_root / model_id
      try:
        if model_path.resolve().parent != model_root.resolve():
          raise ModelNotFound('Invalid model id')
      except FileNotFoundError:
        continue
      if self.is_published_model_(model_path):
        return model_path
    raise ModelNotFound("Model '" + model_id + "' not found")

  def is_published_model_(self, path):
    return (
      path.is_dir() and
      not path.is_symlink() and
      not path.name.startswith('~') and
      all((path / name).is_file() for name in self.REQUIRED_FILES))

  def training_status_(self, model_id):
    if (
        not isinstance(model_id, str) or
        not model_id.startswith('~') or
        len(model_id) == 1):
      return None
    path = self.training_path_(model_id)
    if path is None:
      return None
    traits_file = path / 'traits.json'
    try:
      traits = self.read_json_(traits_file)
      pid = int(traits.get('pid', 0))
      train_start = traits['train_start']
    except (KeyError, TypeError, ValueError, RuntimeError):
      return None
    if traits.get('status') not in ('in_progress', 'interrupted'):
      return None
    if traits.get('status') == 'in_progress' and not self.process_alive_(pid):
      traits['status'] = 'interrupted'
      traits['interruption_reason'] = 'process_not_running'
      prepare = traits.get('prepare')
      if isinstance(prepare, dict) and prepare.get('status') == 'training':
        prepare['status'] = 'interrupted'
      models = traits.get('models')
      if isinstance(models, list):
        for model in models:
          if isinstance(model, dict) and model.get('status') == 'training':
            model['status'] = 'interrupted'
      post_processing = traits.get('post_processing')
      if (
          isinstance(post_processing, dict) and
          post_processing.get('status') == 'training'):
        post_processing['status'] = 'interrupted'
    return traits

  def model_directory_and_traits_(self, model_id):
    training_traits = self.training_status_(model_id)
    if training_traits is not None:
      return self.training_path_(model_id), training_traits
    model_path = self.model_path(model_id)
    return model_path, self.read_json_(model_path / 'traits.json')

  def training_path_(self, model_id):
    for model_root in self.model_roots:
      path = model_root / model_id
      try:
        if path.resolve().parent != model_root.resolve():
          return None
      except FileNotFoundError:
        continue
      if (
          path.is_dir() and
          not path.is_symlink() and
          (path / 'traits.json').is_file()):
        return path
    return None

  @classmethod
  def read_artifact_(cls, model_path, artifact_path):
    relative_path = pathlib.PurePosixPath(artifact_path)
    if (
        relative_path.is_absolute() or
        not relative_path.parts or
        any(part in ('', '.', '..') for part in relative_path.parts)):
      raise ModelNotFound('Invalid model artifact path')
    model_root = model_path.resolve()
    unresolved_path = model_path / pathlib.Path(*relative_path.parts)
    current_path = model_path
    for part in relative_path.parts:
      current_path = current_path / part
      if current_path.is_symlink():
        raise ModelNotFound('Invalid model artifact path')
    path = unresolved_path.resolve()
    try:
      path.relative_to(model_root)
    except ValueError:
      raise ModelNotFound('Invalid model artifact path') from None
    if not path.is_file():
      raise ModelNotFound("Model artifact '" + artifact_path + "' not found")
    return cls.read_json_(path)

  @staticmethod
  def process_alive_(pid):
    if pid <= 0:
      return False
    try:
      os.kill(pid, 0)
    except ProcessLookupError:
      return False
    except PermissionError:
      return True
    return True

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
