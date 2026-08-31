import dataclasses
import math

import numpy

from .SegmentModelData import SegmentBatch


@dataclasses.dataclass(frozen=True)
class NavigationEvent:
  user_id: int
  timestamp: int
  url_id: int


@dataclasses.dataclass(frozen=True)
class SyntheticImpression:
  user_id: int
  timestamp: int
  clicked: int


@dataclasses.dataclass(frozen=True)
class SyntheticRule:
  segment_id: int
  url_ids: tuple
  window_seconds: int
  min_visits: int
  weight: float


@dataclasses.dataclass(frozen=True)
class ExistingChannelRule:
  channel_id: int
  url_ids: tuple
  window_seconds: int
  min_visits: int
  source_segment_id: int


@dataclasses.dataclass
class SyntheticDataset:
  urls: list
  events: list
  impressions: list
  history_counts: numpy.ndarray
  existing_channels: numpy.ndarray
  context_features: numpy.ndarray
  labels: numpy.ndarray
  timestamps: numpy.ndarray
  true_rules: list
  existing_rules: list
  train_indices: numpy.ndarray
  validation_indices: numpy.ndarray
  group_ids: numpy.ndarray = None
  group_names: tuple = ()
  source_metadata: dict = None


class SyntheticBatchBuilder:
  def __init__(self, dataset, sample_indices, batch_size, seed=1):
    self.dataset = dataset
    self.sample_indices = numpy.asarray(sample_indices, dtype=numpy.int64)
    self.batch_size = batch_size
    self.seed = seed

  @property
  def batches_per_epoch(self):
    return int(math.ceil(len(self.sample_indices) / self.batch_size))

  def __call__(self, request):
    generator = numpy.random.default_rng(self.seed + request.epoch)
    shuffled = generator.permutation(self.sample_indices)
    begin = request.batch_index * self.batch_size
    indices = shuffled[begin:begin + self.batch_size]
    existing_channels = self.dataset.existing_channels[indices]
    if not request.include_existing_channels:
      existing_channels = numpy.zeros_like(existing_channels)
    return SegmentBatch(
      history_counts=numpy.ascontiguousarray(self.dataset.history_counts[indices]),
      existing_channels=numpy.ascontiguousarray(existing_channels),
      context_features=numpy.ascontiguousarray(self.dataset.context_features[indices]),
      labels=numpy.ascontiguousarray(self.dataset.labels[indices]),
      sample_indices=numpy.ascontiguousarray(indices))


def generate_synthetic_dataset(config):
  if config.synthetic.true_segments <= 0:
    raise ValueError('synthetic true_segments must be positive')
  if config.synthetic.true_segments > config.model.candidates:
    raise ValueError('synthetic true_segments must not exceed model candidates')
  generator = numpy.random.default_rng(config.synthetic.seed)
  windows = numpy.asarray(config.data.windows_seconds, dtype=numpy.int64)
  n_values = numpy.asarray(config.data.n_values, dtype=numpy.int64)
  urls = ['u' + str(index) for index in range(config.synthetic.urls)]
  true_rules = _generate_true_rules(generator, config, windows, n_values)
  existing_rules = _generate_existing_rules(generator, config, true_rules)
  users = generator.integers(0, config.synthetic.users, size=config.synthetic.samples)
  min_timestamp = int(windows[-1])
  timestamps = generator.integers(
    min_timestamp,
    config.synthetic.horizon_seconds,
    size=config.synthetic.samples,
    dtype=numpy.int64)
  order = numpy.argsort(timestamps, kind='stable')
  users = users[order]
  timestamps = timestamps[order]
  events, events_by_user = _generate_events(generator, config, users, timestamps, true_rules)
  history_counts = _build_history_counts(
    users,
    timestamps,
    events_by_user,
    config.synthetic.urls,
    windows)
  true_activations = _activate_rules(history_counts, true_rules, windows)
  existing_channels = _activate_rules(history_counts, existing_rules, windows)
  context_shape = (config.synthetic.samples, config.model.context_size)
  context_features = generator.normal(0.0, 1.0, size=context_shape).astype(numpy.float32)
  logits = numpy.full(config.synthetic.samples, -3.5, dtype=numpy.float64)
  for rule_index, rule in enumerate(true_rules):
    logits += rule.weight * true_activations[:, rule_index]
  if config.model.context_size:
    context_weights = generator.normal(0.0, 0.25, size=config.model.context_size)
    logits += context_features @ context_weights
  probabilities = 1.0 / (1.0 + numpy.exp(-numpy.clip(logits, -30.0, 30.0)))
  labels = generator.binomial(1, probabilities).astype(numpy.float32)
  impressions = [
    SyntheticImpression(int(users[index]), int(timestamps[index]), int(labels[index]))
    for index in range(config.synthetic.samples)
  ]
  validation_size = max(1, int(config.synthetic.samples * config.synthetic.validation_fraction))
  split = config.synthetic.samples - validation_size
  return SyntheticDataset(
    urls=urls,
    events=events,
    impressions=impressions,
    history_counts=history_counts,
    existing_channels=existing_channels.astype(numpy.float32),
    context_features=context_features,
    labels=labels,
    timestamps=timestamps,
    true_rules=true_rules,
    existing_rules=existing_rules,
    train_indices=numpy.arange(0, split, dtype=numpy.int64),
    validation_indices=numpy.arange(split, config.synthetic.samples, dtype=numpy.int64))


def _generate_true_rules(generator, config, windows, n_values):
  rules = []
  available_urls = numpy.arange(config.synthetic.urls)
  for segment_id in range(config.synthetic.true_segments):
    url_count = int(generator.integers(2, min(7, config.synthetic.urls + 1)))
    url_ids = tuple(sorted(generator.choice(available_urls, url_count, replace=False).tolist()))
    weight = float(generator.choice((1.25, 1.75, 2.25, -1.0, -1.5)))
    rules.append(SyntheticRule(
      segment_id,
      url_ids,
      int(generator.choice(windows)),
      int(generator.choice(n_values)),
      weight))
  return rules


def _generate_existing_rules(generator, config, true_rules):
  rules = []
  for channel_index in range(config.synthetic.existing_channels):
    source = true_rules[channel_index % len(true_rules)]
    urls = list(source.url_ids)
    relation = channel_index % 3
    if relation == 1 and len(urls) > 1:
      urls.pop()
    elif relation == 2:
      candidates = [url_id for url_id in range(config.synthetic.urls) if url_id not in urls]
      if candidates:
        urls.append(int(generator.choice(candidates)))
    rules.append(ExistingChannelRule(
      channel_id=1000 + channel_index,
      url_ids=tuple(sorted(urls)),
      window_seconds=source.window_seconds,
      min_visits=source.min_visits,
      source_segment_id=source.segment_id))
  return rules


def _generate_events(generator, config, impression_users, impression_times, true_rules):
  events_by_user = []
  for user_id in range(config.synthetic.users):
    timestamps = numpy.sort(generator.integers(
      0,
      config.synthetic.horizon_seconds,
      size=config.synthetic.events_per_user,
      dtype=numpy.int64))
    url_ids = generator.integers(
      0,
      config.synthetic.urls,
      size=config.synthetic.events_per_user,
      dtype=numpy.int64)
    events_by_user.append(list(zip(timestamps.tolist(), url_ids.tolist())))
  for user_id, impression_time in zip(impression_users, impression_times):
    for rule in true_rules:
      if generator.random() >= config.synthetic.segment_activation_probability:
        continue
      url_id = int(generator.choice(rule.url_ids))
      ages = generator.integers(1, rule.window_seconds + 1, size=rule.min_visits, dtype=numpy.int64)
      events_by_user[int(user_id)].extend((int(impression_time - age), url_id) for age in ages)
  events = []
  for user_id, user_events in enumerate(events_by_user):
    user_events.sort()
    events.extend(
      NavigationEvent(user_id, int(timestamp), int(url_id))
      for timestamp, url_id in user_events)
  return events, events_by_user


def _build_history_counts(users, timestamps, events_by_user, url_count, windows):
  result = numpy.zeros((len(users), url_count, len(windows)), dtype=numpy.float32)
  max_window = int(windows[-1])
  for sample_index, (user_id, impression_time) in enumerate(zip(users, timestamps)):
    for event_time, url_id in events_by_user[int(user_id)]:
      age = int(impression_time) - event_time
      if age <= 0 or age > max_window:
        continue
      first_window = int(numpy.searchsorted(windows, age, side='left'))
      result[sample_index, url_id, first_window:] += 1.0
  return result


def _activate_rules(history_counts, rules, windows):
  result = numpy.zeros((history_counts.shape[0], len(rules)), dtype=numpy.float32)
  window_index = {int(window): index for index, window in enumerate(windows)}
  for rule_index, rule in enumerate(rules):
    counts = history_counts[:, list(rule.url_ids), window_index[rule.window_seconds]]
    result[:, rule_index] = (numpy.max(counts, axis=1) >= rule.min_visits).astype(numpy.float32)
  return result
