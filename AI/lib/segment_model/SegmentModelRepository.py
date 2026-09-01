import json
import os
import pathlib


class SegmentModelNotFound(Exception):
  pass


class SegmentModelRepository:
  REQUIRED_FILES = (
    'config.json',
    'training.json',
    'metrics.json',
    'segments.json',
  )
  VIEWABLE_FILES = REQUIRED_FILES + (
    'traits.json',
    'training-summary.json',
    'source.json',
    'scenario.json',
    'synthetic-ground-truth.json',
    'vocabulary.json',
    'url-bucket-dictionary.json',
  )

  def __init__(self, model_root):
    self.model_root = pathlib.Path(model_root)
    self.json_cache = {}

  def model_ids(self):
    if not self.model_root.is_dir():
      return []
    return self._sorted_model_ids(
      path.name
      for path in self.model_root.iterdir()
      if self._is_published_model(path))

  def all_model_ids(self):
    if not self.model_root.is_dir():
      return []
    return self._sorted_model_ids(
      path.name
      for path in self.model_root.iterdir()
      if self._is_published_model(path) or self._training_traits(path) is not None)

  def latest_model_id(self):
    model_ids = self.model_ids()
    if not model_ids:
      raise SegmentModelNotFound('No published segment models')
    return model_ids[0]

  def model_summary(self, model_id):
    model_path, status, traits = self._model_directory(model_id)
    config = self._read_optional_json(model_path / 'config.json', {})
    if not isinstance(config, dict):
      raise RuntimeError("'config.json' must contain an object")
    progress = traits.get('progress')
    if not isinstance(progress, dict):
      progress = {}
    summary = self._read_optional_json(model_path / 'training-summary.json', {})
    if not isinstance(summary, dict):
      summary = {}
    metrics = self._read_optional_json(model_path / 'metrics.json', {})
    if not isinstance(metrics, dict):
      metrics = {}
    segments_count = traits.get('segments_count')
    if not isinstance(segments_count, int):
      segments = self._read_optional_json(model_path / 'segments.json', [])
      segments_count = len(segments) if isinstance(segments, list) else 0
    model_config = config.get('model')
    if not isinstance(model_config, dict):
      model_config = {}
    diagnostics = metrics.get('diagnostics')
    if not isinstance(diagnostics, dict):
      diagnostics = {}
    soft_ctr = metrics.get('soft_ctr')
    if not isinstance(soft_ctr, dict):
      soft_ctr = {}
    hard_ctr = metrics.get('hard_ctr')
    if not isinstance(hard_ctr, dict):
      hard_ctr = {}
    return {
      'id': model_id,
      'status': status,
      'train_start': traits.get('train_start'),
      'train_end': traits.get('train_end'),
      'stage': progress.get('stage'),
      'epoch': progress.get('epoch'),
      'completed_batches': progress.get('completed_batches'),
      'total_batches': progress.get('total_batches'),
      'training_elapsed_seconds': progress.get('training_elapsed_seconds'),
      'candidates': model_config.get('candidates'),
      'aggregation': model_config.get('aggregation'),
      'epochs_completed': summary.get('epochs_completed'),
      'best_epoch': summary.get('best_epoch'),
      'segments_count': segments_count,
      'empty_segments': diagnostics.get('empty_segments'),
      'highly_similar_segment_pairs': diagnostics.get(
        'highly_similar_segment_pairs'),
      'soft_logloss': soft_ctr.get('logloss'),
      'hard_logloss': hard_ctr.get('logloss'),
      'interruption_reason': traits.get('interruption_reason'),
    }

  def model_properties(self, model_id):
    model_path, _, traits = self._model_directory(model_id)
    return {
      'summary': self.model_summary(model_id),
      'traits': traits,
      'config': self._read_optional_json(model_path / 'config.json', {}),
      'training': self._read_optional_json(model_path / 'training.json', []),
      'training_summary': self._read_optional_json(
        model_path / 'training-summary.json', {}),
      'metrics': self._read_optional_json(model_path / 'metrics.json', {}),
      'source': self._read_optional_json(model_path / 'source.json', None),
      'scenario': self._read_optional_json(model_path / 'scenario.json', None),
      'files': [
        file_name
        for file_name in self.VIEWABLE_FILES
        if (model_path / file_name).is_file()
      ],
    }

  def segments(self, model_id, offset=0, limit=100, search=None):
    model_path, _, _ = self._model_directory(model_id)
    values = self._read_optional_json(model_path / 'segments.json', [])
    if not isinstance(values, list):
      raise RuntimeError("'segments.json' must contain an array")
    metrics = self._read_optional_json(model_path / 'metrics.json', {})
    diagnostics = metrics.get('diagnostics', {}) if isinstance(metrics, dict) else {}
    if not isinstance(diagnostics, dict):
      diagnostics = {}
    candidates = diagnostics.get('candidates', [])
    if not isinstance(candidates, list):
      candidates = []
    candidates_by_id = {
      candidate.get('segment_id'): candidate
      for candidate in candidates
      if isinstance(candidate, dict)
    }
    items = [
      {
        **value,
        **{
          key: candidate[key]
          for key in ('average_activation', 'top_url_gates')
          if key in candidate
        },
      }
      for value in values
      if isinstance(value, dict)
      for candidate in [candidates_by_id.get(value.get('segment_id'), {})]
    ]
    if search:
      normalized = search.casefold()
      items = [item for item in items if self._segment_matches(item, normalized)]
    return {
      'model_id': model_id,
      'offset': offset,
      'limit': limit,
      'total': len(items),
      'items': items[offset:offset + limit],
    }

  def segment(self, model_id, segment_id):
    model_path, _, _ = self._model_directory(model_id)
    values = self._read_optional_json(model_path / 'segments.json', [])
    if not isinstance(values, list):
      raise RuntimeError("'segments.json' must contain an array")
    for segment in values:
      if isinstance(segment, dict) and segment.get('segment_id') == segment_id:
        result = self.segments(model_id, 0, len(values))
        return next(
          item
          for item in result['items']
          if item.get('segment_id') == segment_id)
    raise SegmentModelNotFound(
      "Segment '" + str(segment_id) + "' is not present in model '" + model_id + "'")

  def model_file(self, model_id, file_name):
    if file_name not in self.VIEWABLE_FILES:
      raise SegmentModelNotFound('Unknown segment model file')
    model_path, _, _ = self._model_directory(model_id)
    path = model_path / file_name
    if not path.is_file() or path.is_symlink():
      raise SegmentModelNotFound("Model file '" + file_name + "' not found")
    return path

  def _model_directory(self, model_id):
    path = self._checked_model_path(model_id)
    training_traits = self._training_traits(path)
    if training_traits is not None:
      return path, training_traits['status'], training_traits
    if not self._is_published_model(path):
      raise SegmentModelNotFound("Segment model '" + model_id + "' not found")
    traits = self._read_optional_json(path / 'traits.json', {})
    if not isinstance(traits, dict):
      traits = {}
    return path, 'published', traits

  def _checked_model_path(self, model_id):
    if not isinstance(model_id, str) or not model_id or '/' in model_id:
      raise SegmentModelNotFound('Invalid segment model id')
    path = self.model_root / model_id
    try:
      if path.resolve().parent != self.model_root.resolve():
        raise SegmentModelNotFound('Invalid segment model id')
    except FileNotFoundError:
      raise SegmentModelNotFound("Segment model '" + model_id + "' not found") from None
    return path

  def _is_published_model(self, path):
    return (
      path.is_dir() and
      not path.is_symlink() and
      not path.name.startswith('~') and
      all((path / file_name).is_file() for file_name in self.REQUIRED_FILES))

  def _training_traits(self, path):
    if (
        not path.name.startswith('~') or
        not path.is_dir() or
        path.is_symlink()):
      return None
    traits = self._read_optional_json(path / 'traits.json', None)
    if not isinstance(traits, dict):
      return None
    if traits.get('status') not in ('in_progress', 'interrupted'):
      return None
    result = dict(traits)
    if result['status'] == 'in_progress':
      try:
        pid = int(result.get('pid', 0))
      except (TypeError, ValueError):
        pid = 0
      if not self._process_alive(pid):
        result['status'] = 'interrupted'
        result['interruption_reason'] = 'process_not_running'
    return result

  def _read_optional_json(self, path, default):
    if not path.is_file() or path.is_symlink():
      return default
    return self._read_json(path)

  def _read_json(self, path):
    try:
      stat = path.stat()
      cache_key = (stat.st_mtime_ns, stat.st_size)
      cached = self.json_cache.get(path)
      if cached is not None and cached[0] == cache_key:
        return cached[1]
      with path.open(encoding='utf-8') as input_file:
        value = json.load(input_file)
      self.json_cache[path] = (cache_key, value)
      return value
    except (OSError, ValueError) as error:
      raise RuntimeError("Can't read '" + str(path) + "': " + str(error)) from error

  @staticmethod
  def _sorted_model_ids(model_ids):
    return sorted(
      model_ids,
      key=lambda model_id: model_id.removeprefix('~'),
      reverse=True)

  @staticmethod
  def _process_alive(pid):
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
  def _segment_matches(segment, search):
    if search in str(segment.get('segment_id', '')).casefold():
      return True
    urls = segment.get('urls')
    if not isinstance(urls, list):
      urls = []
    if any(search in str(url).casefold() for url in urls):
      return True
    relations = segment.get('relations')
    if not isinstance(relations, list):
      relations = []
    return any(
      search in str(relation.get('channel_id', '')).casefold() or
      search in str(relation.get('relation', '')).casefold()
      for relation in relations
      if isinstance(relation, dict))
