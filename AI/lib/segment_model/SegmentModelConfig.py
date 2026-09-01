import dataclasses
import json
import pathlib


@dataclasses.dataclass(frozen=True)
class TemperatureSchedule:
  start: float = 0.5
  end: float = 0.1
  schedule: str = 'exponential'
  start_progress: float = 0.4
  end_progress: float = 1.0

  def validate(self, name):
    if self.start <= 0 or self.end <= 0:
      raise ValueError(name + ' temperatures must be positive')
    if self.schedule not in ('linear', 'exponential'):
      raise ValueError(name + " schedule must be 'linear' or 'exponential'")
    if not 0 <= self.start_progress < self.end_progress <= 1:
      raise ValueError(name + ' progress range must be inside [0, 1]')


@dataclasses.dataclass(frozen=True)
class MembershipConfig:
  initialization: str = 'frequency'
  initial_urls_per_candidate: int = 1
  unselected_logit: float = -2.0
  selected_logit: float = 1.0
  logit_std: float = 0.25


@dataclasses.dataclass(frozen=True)
class DataConfig:
  windows_seconds: tuple = (60, 300, 3600, 86400, 604800)
  n_values: tuple = (1, 2, 3, 5, 10)
  url_buckets: int = 1000000
  batch_size: int = 128
  batch_workers: int = 2
  ready_batches: int = 4
  batch_start_method: str = 'fork'


@dataclasses.dataclass(frozen=True)
class ForestConfig:
  trees: int = 32
  depth: int = 5
  features_per_node: int = 16
  feature_initial_logit: float = 2.0
  seed: int = 19


@dataclasses.dataclass(frozen=True)
class ModelConfig:
  candidates: int = 20
  context_size: int = 0
  aggregation: str = 'softmax_max'
  activation_boundary: float = 0.5
  choice_initial_logit: float = 2.0
  membership: MembershipConfig = dataclasses.field(default_factory=MembershipConfig)
  forest: ForestConfig = dataclasses.field(default_factory=ForestConfig)


@dataclasses.dataclass(frozen=True)
class LossConfig:
  sparsity: float = 1e-3
  binarization: float = 1e-3
  diversity: float = 1e-3
  duplicate_existing: float = 1e-3
  duplicate_threshold: float = 0.95
  diversity_pairs: int = 256


@dataclasses.dataclass(frozen=True)
class TrainingConfig:
  discovery_epochs: int = 5
  structuring_epochs: int = 5
  max_epochs: int = 10000
  early_stopping_patience: int = 20
  early_stopping_min_delta: float = 1e-6
  learning_rate: float = 5e-3
  weight_decay: float = 0.0
  device: str = 'cpu'
  seed: int = 17


@dataclasses.dataclass(frozen=True)
class SyntheticConfig:
  samples: int = 4000
  users: int = 400
  urls: int = 200
  true_segments: int = 10
  existing_channels: int = 8
  events_per_user: int = 80
  segment_activation_probability: float = 0.15
  horizon_seconds: int = 1209600
  validation_fraction: float = 0.1
  final_test_fraction: float = 0.1
  seed: int = 23


@dataclasses.dataclass(frozen=True)
class SegmentModelConfig:
  data: DataConfig = dataclasses.field(default_factory=DataConfig)
  model: ModelConfig = dataclasses.field(default_factory=ModelConfig)
  loss: LossConfig = dataclasses.field(default_factory=LossConfig)
  training: TrainingConfig = dataclasses.field(default_factory=TrainingConfig)
  synthetic: SyntheticConfig = dataclasses.field(default_factory=SyntheticConfig)
  url_temperature: TemperatureSchedule = dataclasses.field(default_factory=TemperatureSchedule)
  window_temperature: TemperatureSchedule = dataclasses.field(
    default_factory=lambda: TemperatureSchedule(0.75, 0.1, 'exponential'))
  threshold_temperature: TemperatureSchedule = dataclasses.field(
    default_factory=lambda: TemperatureSchedule(0.75, 0.1, 'exponential'))
  activation_temperature: TemperatureSchedule = dataclasses.field(
    default_factory=lambda: TemperatureSchedule(0.5, 0.05, 'exponential'))
  aggregation_temperature: TemperatureSchedule = dataclasses.field(
    default_factory=lambda: TemperatureSchedule(0.5, 0.05, 'exponential'))
  forest_feature_temperature: TemperatureSchedule = dataclasses.field(
    default_factory=lambda: TemperatureSchedule(1.0, 0.1, 'exponential'))
  forest_split_temperature: TemperatureSchedule = dataclasses.field(
    default_factory=lambda: TemperatureSchedule(0.5, 0.05, 'exponential'))

  @classmethod
  def from_json(cls, config_file):
    with pathlib.Path(config_file).open(encoding='utf-8') as stream:
      return cls.from_dict(json.load(stream))

  @classmethod
  def from_dict(cls, value):
    value = dict(value)
    temperatures = value.pop('temperatures', {})
    temperature_names = {
      'url',
      'window',
      'threshold',
      'activation',
      'aggregation',
      'forest_feature',
      'forest_split',
    }
    unknown_temperatures = set(temperatures) - temperature_names
    if unknown_temperatures:
      raise ValueError('Unknown temperatures: ' + ', '.join(sorted(unknown_temperatures)))
    config = cls(
      data=_make_dataclass(DataConfig, value.pop('data', {})),
      model=_make_model_config(value.pop('model', {})),
      loss=_make_dataclass(LossConfig, value.pop('loss', {})),
      training=_make_dataclass(TrainingConfig, value.pop('training', {})),
      synthetic=_make_dataclass(SyntheticConfig, value.pop('synthetic', {})),
      **{
        name + '_temperature': _make_dataclass(TemperatureSchedule, temperatures.get(name, {}))
        for name in temperature_names
        if name in temperatures
      })
    if value:
      raise ValueError('Unknown configuration keys: ' + ', '.join(sorted(value)))
    config.validate()
    return config

  def validate(self):
    if not self.data.windows_seconds or any(value <= 0 for value in self.data.windows_seconds):
      raise ValueError('windows_seconds must contain positive values')
    if tuple(sorted(set(self.data.windows_seconds))) != self.data.windows_seconds:
      raise ValueError('windows_seconds must be strictly increasing')
    if not self.data.n_values or any(value <= 0 for value in self.data.n_values):
      raise ValueError('n_values must contain positive values')
    if tuple(sorted(set(self.data.n_values))) != self.data.n_values:
      raise ValueError('n_values must be strictly increasing')
    if self.data.url_buckets <= 0:
      raise ValueError('url_buckets must be positive')
    if self.data.batch_size <= 0 or self.data.batch_workers <= 0:
      raise ValueError('batch_size and batch_workers must be positive')
    if self.data.ready_batches < self.data.batch_workers:
      raise ValueError('ready_batches must be at least batch_workers')
    if self.data.batch_start_method not in ('fork', 'forkserver', 'spawn'):
      raise ValueError('unsupported batch_start_method')
    if self.model.candidates <= 0:
      raise ValueError('candidates must be positive')
    if self.model.context_size < 0:
      raise ValueError('context_size must not be negative')
    if self.model.aggregation not in ('sum', 'softmax_max'):
      raise ValueError("aggregation must be 'sum' or 'softmax_max'")
    if self.model.activation_boundary < 0:
      raise ValueError('activation_boundary must not be negative')
    if self.model.membership.initial_urls_per_candidate <= 0:
      raise ValueError('initial_urls_per_candidate must be positive')
    if self.model.membership.initialization not in ('frequency', 'random'):
      raise ValueError("membership initialization must be 'frequency' or 'random'")
    if self.model.membership.logit_std < 0:
      raise ValueError('membership logit_std must not be negative')
    if self.model.forest.trees <= 0 or self.model.forest.depth <= 0:
      raise ValueError('forest trees and depth must be positive')
    if self.model.forest.features_per_node <= 0:
      raise ValueError('forest features_per_node must be positive')
    if self.training.discovery_epochs <= 0 or self.training.structuring_epochs <= 0:
      raise ValueError('both training stages must contain at least one epoch')
    scheduled_epochs = self.training.discovery_epochs + self.training.structuring_epochs
    if self.training.max_epochs < scheduled_epochs:
      raise ValueError('max_epochs must cover discovery and temperature structuring epochs')
    if self.training.early_stopping_patience <= 0:
      raise ValueError('early_stopping_patience must be positive')
    if self.training.early_stopping_min_delta < 0:
      raise ValueError('early_stopping_min_delta must not be negative')
    for field in dataclasses.fields(self.loss):
      if getattr(self.loss, field.name) < 0:
        raise ValueError('loss coefficients must not be negative')
    if not 0 < self.synthetic.segment_activation_probability < 1:
      raise ValueError('segment_activation_probability must be between zero and one')
    if self.synthetic.urls < 2 or self.synthetic.users <= 0 or self.synthetic.samples <= 1:
      raise ValueError('synthetic urls, users and samples are too small')
    if self.synthetic.events_per_user < 0 or self.synthetic.existing_channels < 0:
      raise ValueError('synthetic event and channel counts must not be negative')
    if not 0 < self.synthetic.validation_fraction < 1:
      raise ValueError('validation_fraction must be between zero and one')
    if not 0 < self.synthetic.final_test_fraction < 1:
      raise ValueError('final_test_fraction must be between zero and one')
    if self.synthetic.validation_fraction + self.synthetic.final_test_fraction >= 1:
      raise ValueError('validation and final-test fractions must leave training samples')
    if self.synthetic.horizon_seconds <= self.data.windows_seconds[-1]:
      raise ValueError('synthetic horizon_seconds must exceed the largest window')
    for field in dataclasses.fields(self):
      if isinstance(getattr(self, field.name), TemperatureSchedule):
        getattr(self, field.name).validate(field.name)


def _make_dataclass(class_type, value):
  value = dict(value)
  for field in dataclasses.fields(class_type):
    if field.name in value and isinstance(field.default, tuple):
      value[field.name] = tuple(value[field.name])
  return class_type(**value)


def _make_model_config(value):
  value = dict(value)
  forest = _make_dataclass(ForestConfig, value.pop('forest', {}))
  membership = _make_dataclass(MembershipConfig, value.pop('membership', {}))
  return ModelConfig(forest=forest, membership=membership, **value)
