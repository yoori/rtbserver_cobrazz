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
  feature_logit_std: float = 1e-3
  leaf_logit_std: float = 1e-3
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
  url_duplicate: float = 0.1
  activation_duplicate: float = 0.01
  duplicate_existing: float = 1e-3
  duplicate_threshold: float = 0.95
  duplicate_jaccard_margin: float = 0.8
  duplicate_activation_margin: float = 0.9
  duplicate_pairs: int = 256
  duplicate_regularization_start_epoch: int = 5
  duplicate_regularization_ramp_epochs: int = 15
  enable_candidate_reseed: bool = False
  candidate_reseed_jaccard_threshold: float = 0.95
  candidate_reseed_activation_threshold: float = 0.98


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
class CandidateOpeningConfig:
  enabled: bool = False
  mode: str = 'fixed'
  first_active_candidates: int = 1
  open_every_epochs: int = 20
  previous_candidate_lr_mode: str = 'reduced'
  previous_candidate_lr_multiplier: float = 0.1
  url_temperature_floor: float = 0.5
  joint_finetune_epochs: int = 0
  joint_finetune_lr_multiplier: float = 0.1
  reset_forest_on_candidate_open: bool = False


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
  candidate_opening: CandidateOpeningConfig = dataclasses.field(
    default_factory=CandidateOpeningConfig)
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
      candidate_opening=_make_dataclass(
        CandidateOpeningConfig,
        value.pop('candidate_opening', {})),
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
    membership_modes = (
      'frequency',
      'random_single_seed',
      'symmetric_with_noise',
      'random_multi_seed',
    )
    if self.model.membership.initialization not in membership_modes:
      raise ValueError('unsupported membership initialization')
    if (
        self.model.membership.initialization == 'random_single_seed' and
        self.model.membership.initial_urls_per_candidate != 1):
      raise ValueError('random_single_seed requires one initial URL per candidate')
    if (
        self.model.membership.initialization == 'random_multi_seed' and
        self.model.membership.initial_urls_per_candidate < 2):
      raise ValueError('random_multi_seed requires at least two initial URLs per candidate')
    if self.model.membership.logit_std < 0:
      raise ValueError('membership logit_std must not be negative')
    if (
        self.model.membership.initialization != 'symmetric_with_noise' and
        self.model.membership.selected_logit <= self.model.membership.unselected_logit):
      raise ValueError('selected_logit must be greater than unselected_logit')
    if self.model.forest.trees <= 0 or self.model.forest.depth <= 0:
      raise ValueError('forest trees and depth must be positive')
    if self.model.forest.features_per_node <= 0:
      raise ValueError('forest features_per_node must be positive')
    if self.model.forest.feature_logit_std < 0 or self.model.forest.leaf_logit_std < 0:
      raise ValueError('forest initialization standard deviations must not be negative')
    if self.training.discovery_epochs <= 0 or self.training.structuring_epochs <= 0:
      raise ValueError('both training stages must contain at least one epoch')
    scheduled_epochs = self.training.discovery_epochs + self.training.structuring_epochs
    if self.training.max_epochs < scheduled_epochs:
      raise ValueError('max_epochs must cover discovery and temperature structuring epochs')
    if self.training.early_stopping_patience <= 0:
      raise ValueError('early_stopping_patience must be positive')
    if self.training.early_stopping_min_delta < 0:
      raise ValueError('early_stopping_min_delta must not be negative')
    opening = self.candidate_opening
    if opening.mode != 'fixed':
      raise ValueError("candidate opening mode must be 'fixed'")
    if not 1 <= opening.first_active_candidates <= self.model.candidates:
      raise ValueError('first_active_candidates must be inside the candidate range')
    if opening.open_every_epochs <= 0:
      raise ValueError('open_every_epochs must be positive')
    if opening.previous_candidate_lr_mode not in ('full', 'reduced'):
      raise ValueError("previous candidate LR mode must be 'full' or 'reduced'")
    lr_multipliers = (
      opening.previous_candidate_lr_multiplier,
      opening.joint_finetune_lr_multiplier,
    )
    if any(not 0 < value <= 1 for value in lr_multipliers):
      raise ValueError('candidate learning-rate multipliers must be inside (0, 1]')
    if opening.url_temperature_floor <= 0:
      raise ValueError('candidate opening URL temperature floor must be positive')
    if opening.joint_finetune_epochs < 0:
      raise ValueError('joint_finetune_epochs must not be negative')
    if opening.reset_forest_on_candidate_open:
      raise ValueError('reset_forest_on_candidate_open is not implemented')
    if opening.enabled:
      all_open_epoch = (
        self.model.candidates - opening.first_active_candidates
      ) * opening.open_every_epochs
      required_epochs = all_open_epoch + opening.joint_finetune_epochs
      if self.training.max_epochs <= required_epochs:
        raise ValueError('max_epochs must include training after candidate opening')
    loss_weights = (
      self.loss.sparsity,
      self.loss.binarization,
      self.loss.url_duplicate,
      self.loss.activation_duplicate,
      self.loss.duplicate_existing,
    )
    if any(value < 0 for value in loss_weights):
      raise ValueError('loss coefficients must not be negative')
    margins = (
      self.loss.duplicate_threshold,
      self.loss.duplicate_jaccard_margin,
      self.loss.duplicate_activation_margin,
      self.loss.candidate_reseed_jaccard_threshold,
      self.loss.candidate_reseed_activation_threshold,
    )
    if any(not 0 <= value <= 1 for value in margins):
      raise ValueError('duplicate margins and thresholds must be inside [0, 1]')
    if self.loss.duplicate_pairs < 0:
      raise ValueError('duplicate_pairs must not be negative')
    if (
        self.loss.duplicate_regularization_start_epoch < 0 or
        self.loss.duplicate_regularization_ramp_epochs < 0):
      raise ValueError('duplicate regularization schedule must not be negative')
    duplicate_regularization_enabled = (
      self.loss.url_duplicate > 0 or self.loss.activation_duplicate > 0)
    duplicate_schedule_end = (
      self.loss.duplicate_regularization_start_epoch +
      self.loss.duplicate_regularization_ramp_epochs)
    if (
        (duplicate_regularization_enabled or self.loss.enable_candidate_reseed) and
        self.training.max_epochs <= duplicate_schedule_end):
      raise ValueError('max_epochs must include an epoch after duplicate regularization warm-up')
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
