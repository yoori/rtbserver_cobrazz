import dataclasses
import datetime
import json
import math
import os
import pathlib
import zlib

import numpy

from .ClickHouseClient import ClickHouseClient
from .ExpressionMatcherClient import ExpressionMatcherClient
from .SegmentModelData import SegmentBatch
from .SyntheticSegmentData import SyntheticDataset
from .SyntheticSegmentData import SyntheticRule
from .UrlHash import url_bucket


class RImpressionBatchBuilder:
  def __init__(
    self,
    scenario,
    windows_seconds,
    clickhouse_url,
    expression_matcher_urls,
    batch_rows=4096,
    profile_workers=16,
    http_timeout=10.0,
    clickhouse_user='default',
    clickhouse_password='',
    range_begin=0,
    range_end=None,
    cache_dir=None,
    cache_prefix='batch',
    url_buckets=1000000):
    if batch_rows <= 0 or profile_workers <= 0:
      raise ValueError('batch_rows and profile_workers must be positive')
    self.scenario = scenario
    self.windows_seconds = numpy.asarray(windows_seconds, dtype=numpy.int64)
    self.clickhouse_url = clickhouse_url
    self.expression_matcher_urls = expression_matcher_urls
    self.batch_rows = batch_rows
    self.profile_workers = profile_workers
    self.http_timeout = http_timeout
    self.clickhouse_user = clickhouse_user
    self.clickhouse_password = clickhouse_password
    self.range_begin = range_begin
    self.range_end = scenario.rows if range_end is None else range_end
    if not 0 <= self.range_begin < self.range_end <= scenario.rows:
      raise ValueError('RImpression range must be inside the scenario')
    self.cache_dir = pathlib.Path(cache_dir) if cache_dir is not None else None
    self.cache_prefix = cache_prefix
    if self.cache_dir is not None:
      self.cache_dir.mkdir(parents=True, exist_ok=True)
    bucket_by_url = {url: url_bucket(url, url_buckets) for url in scenario.urls}
    self.history_url_ids = numpy.asarray(sorted(set(bucket_by_url.values())), dtype=numpy.int64)
    position_by_bucket = {
      int(bucket): index
      for index, bucket in enumerate(self.history_url_ids)
    }
    self.url_indices = {
      url: position_by_bucket[bucket]
      for url, bucket in bucket_by_url.items()
    }

  @property
  def batches(self):
    return int(math.ceil((self.range_end - self.range_begin) / self.batch_rows))

  @property
  def batches_per_epoch(self):
    return self.batches

  def label_statistics(self):
    client = ClickHouseClient(
      self.clickhouse_url,
      self.clickhouse_user,
      self.clickhouse_password,
      self.http_timeout)
    query = (
      'SELECT count(), countIf(click_timestamp IS NOT NULL) '
      'FROM RImpression '
      'WHERE impression_id >= ' + str(self.range_begin) + ' '
      'AND impression_id < ' + str(self.range_end) + ' '
      'FORMAT TabSeparated')
    values = client.execute(query).decode('utf-8').strip().split('\t')
    if len(values) != 2 or int(values[0]) != self.range_end - self.range_begin:
      raise RuntimeError('ClickHouse returned invalid training label statistics')
    return int(values[0]), int(values[1])

  def __call__(self, request):
    if not 0 <= request.batch_index < self.batches:
      raise ValueError('batch index is outside the configured RImpression range')
    cache_file = self._cache_file(request.batch_index)
    if cache_file is not None and cache_file.exists():
      return self._read_cache(cache_file)
    begin = self.range_begin + request.batch_index * self.batch_rows
    end = min(self.range_end, begin + self.batch_rows)
    impressions = self._read_impressions(begin, end)
    if len(impressions) != end - begin:
      raise RuntimeError('ClickHouse returned an incomplete RImpression range')
    profiles = self._read_profiles(impressions)
    batch = self._make_batch(impressions, profiles)
    if cache_file is not None:
      self._write_cache(cache_file, batch)
    return batch

  def _read_impressions(self, begin, end):
    client = ClickHouseClient(
      self.clickhouse_url,
      self.clickhouse_user,
      self.clickhouse_password,
      self.http_timeout)
    query = (
      'SELECT impression_id, toUnixTimestamp(timestamp), assumeNotNull(uid), '
      'if(click_timestamp IS NOT NULL, 1, 0) '
      'FROM RImpression '
      'WHERE impression_id >= ' + str(begin) + ' AND impression_id < ' + str(end) + ' '
      'ORDER BY impression_id FORMAT TabSeparated')
    result = []
    for line in client.execute(query).decode('utf-8').splitlines():
      fields = line.split('\t')
      if len(fields) != 4:
        raise RuntimeError('ClickHouse returned a malformed RImpression row')
      result.append((int(fields[0]), int(fields[1]), fields[2], int(fields[3])))
    return result

  def _read_profiles(self, impressions):
    client = ExpressionMatcherClient(
      self.expression_matcher_urls,
      self.http_timeout,
      workers=self.profile_workers)
    groups = {}
    for row_index, impression in enumerate(impressions):
      day = impression[1] - impression[1] % 86400
      groups.setdefault(day, []).append((row_index, impression[2]))
    result = [None] * len(impressions)
    for day, rows in groups.items():
      profiles = client.profiles([user_id for _, user_id in rows], day)
      for row_index, user_id in rows:
        result[row_index] = profiles[user_id]
    return result

  def _make_batch(self, impressions, profiles):
    rows = len(impressions)
    histories = numpy.zeros(
      (rows, len(self.url_indices), len(self.windows_seconds)),
      dtype=numpy.float32)
    labels = numpy.empty(rows, dtype=numpy.float32)
    sample_indices = numpy.empty(rows, dtype=numpy.int64)
    timestamps = numpy.empty(rows, dtype=numpy.int64)
    group_ids = numpy.empty(rows, dtype=numpy.int64)
    variant_ids = numpy.empty(rows, dtype=numpy.int64)
    for row_index, (impression, navigations) in enumerate(zip(impressions, profiles)):
      impression_id, timestamp, user_id, clicked = impression
      numeric_uid = self.scenario.numeric_uid(user_id)
      if numeric_uid != impression_id:
        raise RuntimeError('scenario user_id does not match impression_id')
      if clicked != self.scenario.clicked(numeric_uid):
        raise RuntimeError('RImpression click does not match the scenario')
      histories[row_index] = build_history_counts(
        navigations,
        timestamp,
        self.url_indices,
        self.windows_seconds)
      labels[row_index] = clicked
      sample_indices[row_index] = impression_id
      timestamps[row_index] = timestamp
      group_id, variant_id, _ = self.scenario.placement(numeric_uid)
      group_ids[row_index] = group_id
      variant_ids[row_index] = variant_id
    return SegmentBatch(
      history_counts=histories,
      history_url_ids=self.history_url_ids,
      existing_channels=numpy.empty((rows, 0), dtype=numpy.float32),
      context_features=numpy.empty((rows, 0), dtype=numpy.float32),
      labels=labels,
      sample_indices=sample_indices,
      timestamps=timestamps,
      group_ids=group_ids,
      variant_ids=variant_ids)

  def _cache_file(self, batch_index):
    if self.cache_dir is None:
      return None
    return self.cache_dir / (self.cache_prefix + '-' + str(batch_index).zfill(6) + '.npz')

  @staticmethod
  def _read_cache(cache_file):
    with numpy.load(cache_file, allow_pickle=False) as value:
      return SegmentBatch(
        history_counts=value['history_counts'],
        history_url_ids=value['history_url_ids'],
        existing_channels=value['existing_channels'],
        context_features=value['context_features'],
        labels=value['labels'],
        sample_indices=value['sample_indices'],
        timestamps=value['timestamps'],
        group_ids=value['group_ids'],
        variant_ids=value['variant_ids'])

  @staticmethod
  def _write_cache(cache_file, batch):
    temporary_file = cache_file.with_name(cache_file.name + '.tmp')
    with temporary_file.open('wb') as stream:
      numpy.savez(
        stream,
        history_counts=batch.history_counts,
        history_url_ids=batch.history_url_ids,
        existing_channels=batch.existing_channels,
        context_features=batch.context_features,
        labels=batch.labels,
        sample_indices=batch.sample_indices,
        timestamps=batch.timestamps,
        group_ids=batch.group_ids,
        variant_ids=batch.variant_ids)
    os.replace(temporary_file, cache_file)


def build_history_counts(navigations, impression_timestamp, url_indices, windows_seconds):
  windows = numpy.asarray(windows_seconds, dtype=numpy.int64)
  result = numpy.zeros((len(url_indices), len(windows)), dtype=numpy.float32)
  impression_date = impression_timestamp - impression_timestamp % 86400
  for navigation in navigations:
    if navigation['url'] not in url_indices:
      raise RuntimeError('ExpressionMatcher returned an unknown URL: ' + navigation['url'])
    navigation_date = _parse_navigation_date(navigation['date'])
    age = impression_date - navigation_date
    if age <= 0 or age > windows[-1]:
      continue
    count = int(navigation['count'])
    if count <= 0:
      raise RuntimeError('ExpressionMatcher returned a non-positive navigation count')
    first_window = int(numpy.searchsorted(windows, age, side='left'))
    result[url_indices[navigation['url']], first_window:] += count
  return result


def _parse_navigation_date(value):
  try:
    parsed = datetime.datetime.strptime(value, '%Y-%m-%d')
  except (TypeError, ValueError) as error:
    raise RuntimeError('ExpressionMatcher returned an invalid navigation date') from error
  return int(parsed.replace(tzinfo=datetime.timezone.utc).timestamp())


@dataclasses.dataclass(frozen=True)
class RImpressionScenarioSource:
  dataset: SyntheticDataset
  training_builder: RImpressionBatchBuilder
  validation_builder: RImpressionBatchBuilder
  final_test_builder: RImpressionBatchBuilder


def make_rimpression_scenario_source(
  config,
  scenario,
  clickhouse_url,
  expression_matcher_urls,
  batch_rows=4096,
  profile_workers=16,
  http_timeout=10.0,
  clickhouse_user='default',
  clickhouse_password='',
  cache_dir=None):
  _verify_rimpression_source(
    scenario,
    clickhouse_url,
    clickhouse_user,
    clickhouse_password,
    http_timeout)
  validation_size = max(1, int(scenario.rows * scenario.validation_fraction))
  final_test_size = max(1, int(scenario.rows * scenario.final_test_fraction))
  validation_begin = scenario.rows - validation_size - final_test_size
  final_test_begin = scenario.rows - final_test_size
  if validation_begin <= 0:
    raise ValueError('scenario validation and final-test ranges leave no training rows')
  cache_dir = _cache_namespace(cache_dir, config, scenario)
  training_builder = RImpressionBatchBuilder(
    scenario,
    config.data.windows_seconds,
    clickhouse_url,
    expression_matcher_urls,
    batch_rows,
    profile_workers,
    http_timeout,
    clickhouse_user,
    clickhouse_password,
    range_begin=0,
    range_end=validation_begin,
    cache_dir=cache_dir,
    cache_prefix='train',
    url_buckets=config.data.url_buckets)
  validation_builder = RImpressionBatchBuilder(
    scenario,
    config.data.windows_seconds,
    clickhouse_url,
    expression_matcher_urls,
    batch_rows,
    profile_workers,
    http_timeout,
    clickhouse_user,
    clickhouse_password,
    range_begin=validation_begin,
    range_end=final_test_begin,
    cache_dir=cache_dir,
    cache_prefix='validation',
    url_buckets=config.data.url_buckets)
  final_test_builder = RImpressionBatchBuilder(
    scenario,
    config.data.windows_seconds,
    clickhouse_url,
    expression_matcher_urls,
    batch_rows,
    profile_workers,
    http_timeout,
    clickhouse_user,
    clickhouse_password,
    range_begin=final_test_begin,
    range_end=scenario.rows,
    cache_dir=cache_dir,
    cache_prefix='final-test',
    url_buckets=config.data.url_buckets)
  return RImpressionScenarioSource(
    _make_dataset_metadata(config, scenario),
    training_builder,
    validation_builder,
    final_test_builder)


def _verify_rimpression_source(scenario, clickhouse_url, user, password, timeout):
  client = ClickHouseClient(clickhouse_url, user, password, timeout)
  values = client.execute(
    'SELECT count(), min(impression_id), max(impression_id) FROM RImpression').decode(
      'utf-8').strip().split('\t')
  if len(values) != 3 or int(values[0]) != scenario.rows:
    raise RuntimeError('RImpression row count does not match the scenario')
  if int(values[1]) != 0 or int(values[2]) != scenario.rows - 1:
    raise RuntimeError('RImpression impression_id range does not match the scenario')


def _make_dataset_metadata(config, scenario):
  url_indices = {url: index for index, url in enumerate(scenario.urls)}
  bucket_dictionary = {}
  for url in scenario.urls:
    bucket_dictionary.setdefault(url_bucket(url, config.data.url_buckets), []).append(url)
  true_rules = [
    SyntheticRule(
      segment_id=segment_id,
      url_ids=tuple(url_indices[url] for url in segment.urls),
      window_seconds=segment.window_seconds,
      min_visits=segment.min_visits,
      weight=segment.weight)
    for segment_id, segment in enumerate(scenario.expected_segments)
  ]
  return SyntheticDataset(
    urls=list(scenario.urls),
    url_bucket_ids=numpy.asarray([
      url_bucket(url, config.data.url_buckets)
      for url in scenario.urls
    ], dtype=numpy.int64),
    history_url_ids=numpy.asarray(sorted(bucket_dictionary), dtype=numpy.int64),
    url_bucket_dictionary=bucket_dictionary,
    events=[],
    impressions=[],
    history_counts=None,
    existing_channels=numpy.empty((0, 0), dtype=numpy.float32),
    context_features=numpy.empty((0, config.model.context_size), dtype=numpy.float32),
    labels=None,
    timestamps=None,
    true_rules=true_rules,
    existing_rules=[],
    train_indices=numpy.empty(0, dtype=numpy.int64),
    validation_indices=numpy.empty(0, dtype=numpy.int64),
    final_test_indices=numpy.empty(0, dtype=numpy.int64),
    group_ids=None,
    group_names=tuple(cohort.name for cohort in scenario.cohorts),
    variant_ids=None,
    group_variant_names=tuple(
      tuple(_variant_name(variant) for variant in cohort.profile_variants)
      for cohort in scenario.cohorts),
    source_metadata={
      'type': 'rimpression-expression-matcher-scenario',
      'scenario': scenario.to_dict(),
    })


def _variant_name(variant):
  return '+'.join(visit.url for visit in variant)


def _cache_namespace(cache_dir, config, scenario):
  if cache_dir is None:
    return None
  value = {
    'scenario': scenario.to_dict(),
    'windows_seconds': config.data.windows_seconds,
    'url_buckets': config.data.url_buckets,
    'context_size': config.model.context_size,
  }
  encoded = json.dumps(value, sort_keys=True, separators=(',', ':')).encode('utf-8')
  cache_key = format(zlib.crc32(encoded), '08x')
  return pathlib.Path(cache_dir) / cache_key
