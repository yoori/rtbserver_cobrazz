import datetime
import json
import os
import pathlib
import re


ARTIFACT_SCHEMA_VERSION = 1
MODEL_ID_PATTERN = re.compile(r'^[A-Za-z0-9][A-Za-z0-9._-]*$')
REQUIRED_MODEL_FILES = (
  'config.json',
  'training.json',
  'metrics.json',
  'segments.json',
)


def default_model_id():
  return datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d.%H%M%S')


def utc_timestamp():
  return datetime.datetime.now(datetime.timezone.utc).isoformat().replace('+00:00', 'Z')


def write_json_atomic(output_file, value):
  output_file = pathlib.Path(output_file)
  output_file.parent.mkdir(parents=True, exist_ok=True)
  temporary_file = output_file.with_name(output_file.name + '.tmp')
  temporary_file.write_text(
    json.dumps(value, indent=2, sort_keys=True) + '\n',
    encoding='utf-8')
  temporary_file.replace(output_file)


class SegmentModelPublication:
  def __init__(self, model_root, model_id=None, metadata=None):
    self.model_root = pathlib.Path(model_root)
    self.model_id = model_id or default_model_id()
    if not MODEL_ID_PATTERN.fullmatch(self.model_id):
      raise ValueError('invalid segment model id')
    self.training_path = self.model_root / ('~' + self.model_id)
    self.published_path = self.model_root / self.model_id
    self.traits = dict(metadata or {})
    self.traits.update({
      'artifact_schema_version': ARTIFACT_SCHEMA_VERSION,
      'status': 'in_progress',
      'pid': os.getpid(),
      'train_start': utc_timestamp(),
    })
    self.started = False
    self.completed = False

  def __enter__(self):
    return self.start()

  def __exit__(self, exception_type, exception, traceback):
    del exception_type
    del traceback
    if exception is not None and self.started and not self.completed:
      self.interrupt(str(exception))
    return False

  def start(self):
    if self.started:
      return self
    self.model_root.mkdir(parents=True, exist_ok=True)
    if self.training_path.exists() or self.published_path.exists():
      raise FileExistsError("segment model '" + self.model_id + "' already exists")
    self.training_path.mkdir()
    self.started = True
    self._write_traits()
    return self

  def update_progress(self, progress):
    self._require_started()
    self.traits['progress'] = dict(progress)
    self._write_traits()

  def update(self, values):
    self._require_started()
    self.traits.update(values)
    self._write_traits()

  def publish(self, values=None):
    self._require_started()
    if self.completed:
      return self.published_path
    missing_files = [
      file_name
      for file_name in REQUIRED_MODEL_FILES
      if not (self.training_path / file_name).is_file()
    ]
    if missing_files:
      raise RuntimeError(
        'segment model is incomplete: ' + ', '.join(missing_files))
    if values:
      self.traits.update(values)
    self.traits.update({
      'status': 'published',
      'train_end': utc_timestamp(),
    })
    self._write_traits()
    self.training_path.replace(self.published_path)
    self.completed = True
    return self.published_path

  def interrupt(self, reason='interrupted'):
    self._require_started()
    if self.completed:
      return
    self.traits.update({
      'status': 'interrupted',
      'train_end': utc_timestamp(),
      'interruption_reason': reason,
    })
    self._write_traits()

  def _require_started(self):
    if not self.started:
      raise RuntimeError('segment model publication is not started')

  def _write_traits(self):
    write_json_atomic(self.training_path / 'traits.json', self.traits)
