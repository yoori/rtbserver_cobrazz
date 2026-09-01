import dataclasses
import datetime
import json
import pathlib


@dataclasses.dataclass(frozen=True)
class ScenarioVisit:
  url: str
  ages_seconds: tuple


@dataclasses.dataclass(frozen=True)
class ScenarioCohort:
  name: str
  uid_mod: int
  uid_remainder: int
  profile_variants: tuple
  click_every_n_per_variant: int

  def matches(self, numeric_uid):
    return numeric_uid % self.uid_mod == self.uid_remainder

  def position(self, numeric_uid):
    cohort_index = (numeric_uid - self.uid_remainder) // self.uid_mod
    variant_index = cohort_index % len(self.profile_variants)
    variant_ordinal = cohort_index // len(self.profile_variants)
    return cohort_index, variant_index, variant_ordinal


@dataclasses.dataclass(frozen=True)
class ScenarioExpectedSegment:
  urls: tuple
  window_seconds: int
  min_visits: int
  weight: float


@dataclasses.dataclass(frozen=True)
class SegmentModelScenario:
  rows: int
  uid_prefix: str
  timestamp_start: int
  timestamp_step_seconds: int
  validation_fraction: float
  final_test_fraction: float
  cohorts: tuple
  expected_segments: tuple

  @classmethod
  def from_json(cls, scenario_file):
    with pathlib.Path(scenario_file).open(encoding='utf-8') as stream:
      return cls.from_dict(json.load(stream))

  @classmethod
  def from_dict(cls, value):
    value = dict(value)
    value.setdefault('final_test_fraction', 0.1)
    cohorts = tuple(_make_cohort(item) for item in value.pop('cohorts'))
    expected_segments = tuple(
      _make_expected_segment(item) for item in value.pop('expected_segments', ()))
    timestamp_start = _parse_timestamp(value.pop('timestamp_start'))
    scenario = cls(
      cohorts=cohorts,
      expected_segments=expected_segments,
      timestamp_start=timestamp_start,
      **value)
    scenario.validate()
    return scenario

  def validate(self):
    if self.rows <= 1:
      raise ValueError('scenario rows must be greater than one')
    if not self.uid_prefix:
      raise ValueError('scenario uid_prefix must not be empty')
    if self.timestamp_step_seconds <= 0:
      raise ValueError('timestamp_step_seconds must be positive')
    if not 0 < self.validation_fraction < 1:
      raise ValueError('validation_fraction must be between zero and one')
    if not 0 < self.final_test_fraction < 1:
      raise ValueError('final_test_fraction must be between zero and one')
    if self.validation_fraction + self.final_test_fraction >= 1:
      raise ValueError('validation and final-test fractions must leave training rows')
    if not self.cohorts:
      raise ValueError('scenario must contain cohorts')
    names = set()
    for cohort in self.cohorts:
      if not cohort.name or cohort.name in names:
        raise ValueError('cohort names must be non-empty and unique')
      names.add(cohort.name)
      if cohort.uid_mod <= 0:
        raise ValueError('cohort uid_mod must be positive')
      if not 0 <= cohort.uid_remainder < cohort.uid_mod:
        raise ValueError('cohort uid_remainder must be inside its modulo')
      if not cohort.profile_variants:
        raise ValueError('cohort must contain profile_variants')
      if cohort.click_every_n_per_variant < 0:
        raise ValueError('click_every_n_per_variant must not be negative')
      for variant in cohort.profile_variants:
        if not variant:
          raise ValueError('profile variant must contain at least one visit rule')
        for visit in variant:
          if not visit.url or not visit.ages_seconds:
            raise ValueError('scenario visit must contain a URL and ages_seconds')
          if any(age <= 0 for age in visit.ages_seconds):
            raise ValueError('navigation ages must be positive')
    available_urls = set(self.urls)
    for segment in self.expected_segments:
      if not segment.urls or not set(segment.urls) <= available_urls:
        raise ValueError('expected segment contains an unknown URL')
      if segment.window_seconds <= 0 or segment.min_visits <= 0:
        raise ValueError('expected segment window and threshold must be positive')
    for numeric_uid in range(min(self.rows, _validation_period(self.cohorts))):
      self.cohort_index(numeric_uid)

  @property
  def urls(self):
    result = []
    present = set()
    for cohort in self.cohorts:
      for variant in cohort.profile_variants:
        for visit in variant:
          if visit.url not in present:
            present.add(visit.url)
            result.append(visit.url)
    return tuple(result)

  def user_id(self, numeric_uid):
    return self.uid_prefix + str(numeric_uid)

  def numeric_uid(self, user_id):
    if not user_id.startswith(self.uid_prefix):
      raise ValueError('user_id does not start with the configured prefix')
    numeric_part = user_id[len(self.uid_prefix):]
    if not numeric_part.isdigit():
      raise ValueError('user_id does not contain a numeric suffix')
    return int(numeric_part)

  def cohort_index(self, numeric_uid):
    matches = [index for index, cohort in enumerate(self.cohorts) if cohort.matches(numeric_uid)]
    if len(matches) != 1:
      raise ValueError('numeric uid must match exactly one scenario cohort')
    return matches[0]

  def placement(self, numeric_uid):
    cohort_index = self.cohort_index(numeric_uid)
    cohort = self.cohorts[cohort_index]
    _, variant_index, variant_ordinal = cohort.position(numeric_uid)
    return cohort_index, variant_index, variant_ordinal

  def clicked(self, numeric_uid):
    cohort_index, _, variant_ordinal = self.placement(numeric_uid)
    every_n = self.cohorts[cohort_index].click_every_n_per_variant
    return int(every_n > 0 and variant_ordinal % every_n == 0)

  def sample(self, numeric_uid, impression_timestamp):
    cohort_index, variant_index, _ = self.placement(numeric_uid)
    cohort = self.cohorts[cohort_index]
    visits = cohort.profile_variants[variant_index]
    navigations = []
    for visit in visits:
      navigations.extend(
        {
          'url': visit.url,
          'timestamp': impression_timestamp - age,
        }
        for age in visit.ages_seconds)
    navigations.sort(key=lambda item: (item['timestamp'], item['url']))
    return {
      'cohort_id': cohort_index,
      'cohort_name': cohort.name,
      'variant_id': variant_index,
      'clicked': self.clicked(numeric_uid),
      'navigations': navigations,
    }

  def timestamp(self, numeric_uid):
    return self.timestamp_start + numeric_uid * self.timestamp_step_seconds

  def to_dict(self):
    return {
      'rows': self.rows,
      'uid_prefix': self.uid_prefix,
      'timestamp_start': _format_timestamp(self.timestamp_start),
      'timestamp_step_seconds': self.timestamp_step_seconds,
      'validation_fraction': self.validation_fraction,
      'final_test_fraction': self.final_test_fraction,
      'cohorts': [
        {
          'name': cohort.name,
          'uid_mod': cohort.uid_mod,
          'uid_remainder': cohort.uid_remainder,
          'profile_variants': [
            [
              {
                'url': visit.url,
                'ages_seconds': list(visit.ages_seconds),
              }
              for visit in variant
            ]
            for variant in cohort.profile_variants
          ],
          'click_every_n_per_variant': cohort.click_every_n_per_variant,
        }
        for cohort in self.cohorts
      ],
      'expected_segments': [dataclasses.asdict(segment) for segment in self.expected_segments],
    }


def _make_cohort(value):
  value = dict(value)
  variants = tuple(
    tuple(
      ScenarioVisit(
        url=visit['url'],
        ages_seconds=tuple(visit['ages_seconds']))
      for visit in variant)
    for variant in value.pop('profile_variants'))
  return ScenarioCohort(profile_variants=variants, **value)


def _make_expected_segment(value):
  value = dict(value)
  value['urls'] = tuple(value['urls'])
  value.setdefault('weight', 1.0)
  return ScenarioExpectedSegment(**value)


def _parse_timestamp(value):
  if isinstance(value, int):
    return value
  parsed = datetime.datetime.fromisoformat(value.replace('Z', '+00:00'))
  if parsed.tzinfo is None:
    parsed = parsed.replace(tzinfo=datetime.timezone.utc)
  return int(parsed.timestamp())


def _format_timestamp(value):
  return datetime.datetime.fromtimestamp(value, datetime.timezone.utc).isoformat().replace(
    '+00:00', 'Z')


def _validation_period(cohorts):
  period = 1
  for cohort in cohorts:
    period = _least_common_multiple(period, cohort.uid_mod * len(cohort.profile_variants))
  return min(period, 100000)


def _least_common_multiple(first, second):
  greatest_common_divisor = _greatest_common_divisor(first, second)
  return first // greatest_common_divisor * second


def _greatest_common_divisor(first, second):
  while second:
    first, second = second, first % second
  return first
