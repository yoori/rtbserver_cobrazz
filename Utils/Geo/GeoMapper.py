#!/usr/bin/env python3

"""Build reusable RU region and city mappings from aggregated Geo logs.

The first run scans --geo and writes evidence.sqlite3. After reviewing
region_conflicts.csv, edit region_overrides_template.csv and run again with
--load-evidence and --region-overrides; the large Geo CSV is not rescanned.
"""

import argparse
import collections
import csv
import difflib
import functools
import hashlib
import json
import math
import os
import sqlite3
import sys
import time
import unicodedata


EARTH_RADIUS_KM = 6371.0088
NORMALIZATION_VERSION = 1
EVIDENCE_VERSION = 2
EMPTY_IP_VALUES = frozenset(('', '<nil>', 'nil', 'null', 'none'))
REQUIRED_GEO_COLUMNS = (
  'IP', 'Source', 'Latitude', 'Longitude', 'Country', 'Region', 'City')


CYRILLIC_TRANSLITERATION = {
  ord('а'): 'a', ord('б'): 'b', ord('в'): 'v', ord('г'): 'g',
  ord('д'): 'd', ord('е'): 'e', ord('ё'): 'e', ord('ж'): 'zh',
  ord('з'): 'z', ord('и'): 'i', ord('й'): 'y', ord('к'): 'k',
  ord('л'): 'l', ord('м'): 'm', ord('н'): 'n', ord('о'): 'o',
  ord('п'): 'p', ord('р'): 'r', ord('с'): 's', ord('т'): 't',
  ord('у'): 'u', ord('ф'): 'f', ord('х'): 'kh', ord('ц'): 'ts',
  ord('ч'): 'ch', ord('ш'): 'sh', ord('щ'): 'shch', ord('ъ'): '',
  ord('ы'): 'y', ord('ь'): '', ord('э'): 'e', ord('ю'): 'yu',
  ord('я'): 'ya',
}


def normalize_text(value):
  value = unicodedata.normalize('NFKC', value or '').casefold().strip()
  value = value.replace('\u2018', "'").replace('\u2019', "'")
  value = value.replace('\u02bc', "'").replace('`', "'")
  for character in ('\u2010', '\u2011', '\u2012', '\u2013', '\u2014', '\u2212'):
    value = value.replace(character, '-')
  return ' '.join(value.split())


def normalize_name(value):
  value = normalize_text(value).translate(CYRILLIC_TRANSLITERATION)
  normalized = []
  for character in value:
    normalized.append(character if character.isalnum() else ' ')
  return ' '.join(''.join(normalized).split())


def normalize_country(value):
  value = normalize_text(value)
  return 'ru' if value in ('ru', 'rus') else value


normalize_geo_text = functools.lru_cache(maxsize=200000)(normalize_text)
normalize_geo_name = functools.lru_cache(maxsize=200000)(normalize_name)
normalize_geo_country = functools.lru_cache(maxsize=1024)(normalize_country)


def format_number(value, digits=6):
  if value is None:
    return ''
  return ('{0:.' + str(digits) + 'f}').format(value)


def sha256_file(path):
  digest = hashlib.sha256()
  with open(path, 'rb') as source:
    while True:
      chunk = source.read(1024 * 1024)
      if not chunk:
        return digest.hexdigest()
      digest.update(chunk)


class CanonicalPoint(object):
  def __init__(self, channel_id, region, latitude, longitude):
    self.channel_id = channel_id
    self.region = region
    self.latitude = latitude
    self.longitude = longitude
    self.aliases = set()
    self.normalized_aliases = set()


class CanonicalData(object):
  def __init__(self):
    self.locations = set()
    self.regions = set()
    self.points = {}
    self.aliases_by_region = collections.defaultdict(
      lambda: collections.defaultdict(set))
    self.global_aliases = collections.defaultdict(set)
    self.point_ids_by_region = collections.defaultdict(set)

  def target_location(self, point, raw_city):
    aliases = [
      alias for alias in point.aliases
      if 'ru/{0}/{1}'.format(point.region, alias) in self.locations]
    if not aliases:
      return None

    normalized_city = normalize_name(raw_city)
    exact_aliases = [
      alias for alias in aliases if normalize_name(alias) == normalized_city]
    if exact_aliases:
      alias = sorted(exact_aliases)[0]
    else:
      alias = min(
        aliases,
        key=lambda item: (
          -difflib.SequenceMatcher(
            None,
            normalized_city,
            normalize_name(item),
            autojunk=False).ratio(),
          normalize_text(item),
          item))
    return 'ru/{0}/{1}'.format(point.region, alias)


def load_locations(path):
  locations = set()
  with open(path, 'r', encoding='utf-8', newline='') as source:
    for row in csv.reader(source):
      if not row:
        continue
      location = normalize_text(row[0])
      parts = location.split('/', 2)
      if len(parts) == 3 and parts[0] == 'ru':
        locations.add(location)
  if not locations:
    raise ValueError('No RU locations found in {0}'.format(path))
  return locations


def load_canonical(points_path, locations_path):
  canonical = CanonicalData()
  canonical.locations = load_locations(locations_path)

  with open(points_path, 'r', encoding='utf-8', newline='') as source:
    reader = csv.DictReader(source)
    required = {
      'channel_id', 'region', 'city', 'latitude', 'longitude', 'status'}
    if reader.fieldnames is None or not required.issubset(reader.fieldnames):
      raise ValueError(
        'Canonical points must contain columns: {0}'.format(
          ','.join(sorted(required))))

    for row in reader:
      if normalize_text(row['status']) == 'd':
        continue
      channel_id = int(row['channel_id'])
      region = normalize_text(row['region'])
      city = normalize_text(row['city'])
      latitude = float(row['latitude'])
      longitude = float(row['longitude'])
      point = canonical.points.get(channel_id)
      if point is None:
        point = CanonicalPoint(channel_id, region, latitude, longitude)
        canonical.points[channel_id] = point
      elif (point.region != region or
          abs(point.latitude - latitude) > 1e-7 or
          abs(point.longitude - longitude) > 1e-7):
        raise ValueError(
          'Conflicting canonical data for channel {0}'.format(channel_id))
      if city:
        point.aliases.add(city)
        point.normalized_aliases.add(normalize_name(city))

  for point in canonical.points.values():
    if not point.aliases:
      raise ValueError(
        'Canonical channel {0} has no city aliases'.format(point.channel_id))
    canonical.regions.add(point.region)
    canonical.point_ids_by_region[point.region].add(point.channel_id)
    for alias in point.aliases:
      normalized_alias = normalize_name(alias)
      canonical.aliases_by_region[point.region][normalized_alias].add(
        point.channel_id)
      canonical.global_aliases[normalized_alias].add(point.channel_id)

  unknown_locations = []
  for point in canonical.points.values():
    if not any(
        'ru/{0}/{1}'.format(point.region, alias) in canonical.locations
        for alias in point.aliases):
      unknown_locations.append(point.channel_id)
  if unknown_locations:
    raise ValueError(
      '{0} canonical channels have no target location, first channel: {1}'.format(
        len(unknown_locations), unknown_locations[0]))

  return canonical


def distance_km(latitude1, longitude1, latitude2, longitude2):
  latitude1 = math.radians(latitude1)
  longitude1 = math.radians(longitude1)
  latitude2 = math.radians(latitude2)
  longitude2 = math.radians(longitude2)
  delta_latitude = latitude2 - latitude1
  delta_longitude = longitude2 - longitude1
  value = (
    math.sin(delta_latitude / 2.0) ** 2 +
    math.cos(latitude1) * math.cos(latitude2) *
    math.sin(delta_longitude / 2.0) ** 2)
  return EARTH_RADIUS_KM * 2.0 * math.asin(math.sqrt(min(1.0, value)))


class CoordinateMatch(object):
  def __init__(self, region=None, region_distance=None,
      region_channel_id=None, city_channel_id=None, city_distance=None,
      region_status='not_found', city_status='not_found'):
    self.region = region
    self.region_distance = region_distance
    self.region_channel_id = region_channel_id
    self.city_channel_id = city_channel_id
    self.city_distance = city_distance
    self.region_status = region_status
    self.city_status = city_status


class SpatialIndex(object):
  def __init__(self, canonical, max_region_distance, max_city_distance,
      region_margin, city_margin, cache_size):
    self.canonical = canonical
    self.max_region_distance = max_region_distance
    self.max_city_distance = max_city_distance
    self.region_margin = region_margin
    self.city_margin = city_margin
    self.cell_size = 0.5
    self.grid = collections.defaultdict(list)
    for point in canonical.points.values():
      self.grid[self._cell(point.latitude, point.longitude)].append(
        point.channel_id)
    self.cached_candidates = functools.lru_cache(maxsize=cache_size)(
      self._find_candidates)

  def _cell(self, latitude, longitude):
    return (
      int(math.floor(latitude / self.cell_size)),
      int(math.floor(longitude / self.cell_size)))

  def _find_candidates(self, latitude, longitude):
    center_latitude, center_longitude = self._cell(latitude, longitude)
    latitude_cells = int(math.ceil(
      self.max_region_distance / 110.574 / self.cell_size)) + 1
    longitude_scale = max(
      111.320 * abs(math.cos(math.radians(latitude))), 1.0)
    longitude_cells = int(math.ceil(
      self.max_region_distance / longitude_scale / self.cell_size)) + 1
    result = []
    for latitude_delta in range(-latitude_cells, latitude_cells + 1):
      for longitude_delta in range(-longitude_cells, longitude_cells + 1):
        for channel_id in self.grid.get((
            center_latitude + latitude_delta,
            center_longitude + longitude_delta), ()):
          point = self.canonical.points[channel_id]
          distance = distance_km(
            latitude, longitude, point.latitude, point.longitude)
          if distance <= self.max_region_distance:
            result.append((distance, channel_id))
    result.sort()
    return tuple(result)

  def match(self, latitude, longitude, raw_city):
    candidates = self.cached_candidates(
      round(latitude, 7), round(longitude, 7))
    if not candidates:
      return CoordinateMatch()

    best_by_region = {}
    for distance, channel_id in candidates:
      point = self.canonical.points[channel_id]
      previous = best_by_region.get(point.region)
      if previous is None or distance < previous:
        best_by_region[point.region] = distance
    regions = sorted(
      (distance, region) for region, distance in best_by_region.items())
    best_region_distance, best_region = regions[0]
    if (len(regions) > 1 and
        regions[1][0] - best_region_distance < self.region_margin):
      region = None
      region_status = 'ambiguous'
    else:
      region = best_region
      region_status = 'matched'
    region_channel_id = None
    if region is not None:
      region_channel_id = next(
        channel_id for distance, channel_id in candidates
        if self.canonical.points[channel_id].region == region)

    nearest_distance = candidates[0][0]
    city_channel_id = None
    city_distance = None
    city_status = 'too_far'
    if nearest_distance <= self.max_city_distance:
      close_candidates = [
        (distance, channel_id)
        for distance, channel_id in candidates
        if distance <= self.max_city_distance and
          distance - nearest_distance < self.city_margin]
      normalized_city = normalize_geo_name(raw_city)
      exact_candidates = [
        (distance, channel_id)
        for distance, channel_id in close_candidates
        if normalized_city and normalized_city in
          self.canonical.points[channel_id].normalized_aliases]
      exact_ids = set(item[1] for item in exact_candidates)
      close_ids = set(item[1] for item in close_candidates)
      if len(exact_ids) == 1:
        city_channel_id = next(iter(exact_ids))
        city_distance = next(
          item[0] for item in exact_candidates if item[1] == city_channel_id)
        city_status = 'matched_by_coordinate_and_name'
      elif len(close_ids) == 1:
        city_channel_id = next(iter(close_ids))
        city_distance = close_candidates[0][0]
        city_status = 'matched'
      else:
        city_status = 'ambiguous'

    if city_channel_id is not None:
      city_region = self.canonical.points[city_channel_id].region
      if region is None or city_region != region:
        city_channel_id = None
        city_distance = None
        city_status = 'region_ambiguous'

    return CoordinateMatch(
      region,
      best_region_distance,
      region_channel_id,
      city_channel_id,
      city_distance,
      region_status,
      city_status)


class Vote(object):
  def __init__(self):
    self.rows = 0
    self.coordinate_rows = 0
    self.exact_city_rows = 0
    self.distance_sum = 0.0
    self.distance_count = 0
    self.max_distance = 0.0
    self.channel_ids = set()

  def add(self, rows=1, distance=None, channel_ids=(), method='coordinate'):
    self.rows += rows
    if method == 'coordinate':
      self.coordinate_rows += rows
    elif method == 'exact_city':
      self.exact_city_rows += rows
    if distance is not None:
      self.distance_sum += distance * rows
      self.distance_count += rows
      self.max_distance = max(self.max_distance, distance)
    self.channel_ids.update(channel_ids)

  def mean_distance(self):
    if not self.distance_count:
      return None
    return self.distance_sum / self.distance_count


class Evidence(object):
  def __init__(self):
    self.region_totals = collections.Counter()
    self.triple_totals = collections.Counter()
    self.region_votes = collections.defaultdict(dict)
    self.city_votes = collections.defaultdict(dict)
    self.stats = collections.Counter()

  def add_region_vote(self, key, target_region, distance, channel_ids, method):
    vote = self.region_votes[key].get(target_region)
    if vote is None:
      vote = Vote()
      self.region_votes[key][target_region] = vote
    vote.add(distance=distance, channel_ids=channel_ids, method=method)

  def add_city_vote(self, key, channel_id, distance):
    vote = self.city_votes[key].get(channel_id)
    if vote is None:
      vote = Vote()
      self.city_votes[key][channel_id] = vote
    vote.add(distance=distance, channel_ids=(channel_id,))


def geo_column_indexes(header):
  indexes = {}
  for column in REQUIRED_GEO_COLUMNS:
    if column not in header:
      raise ValueError('Geo CSV has no {0} column'.format(column))
    indexes[column] = header.index(column)
  return indexes


def scan_geo(path, canonical, spatial_index, progress_rows):
  evidence = Evidence()
  started = time.time()
  with open(path, 'r', encoding='utf-8', newline='') as source:
    reader = csv.reader(source)
    try:
      indexes = geo_column_indexes(next(reader))
    except StopIteration:
      raise ValueError('Empty Geo CSV: {0}'.format(path))
    minimum_columns = max(indexes.values()) + 1

    for row_number, row in enumerate(reader, 1):
      evidence.stats['input_rows'] += 1
      if len(row) < minimum_columns:
        evidence.stats['malformed_rows'] += 1
        continue

      ip = row[indexes['IP']].strip().casefold()
      if ip in EMPTY_IP_VALUES:
        evidence.stats['empty_ip_rows'] += 1
        continue
      country = normalize_geo_country(row[indexes['Country']])
      if country != 'ru':
        evidence.stats['non_ru_rows'] += 1
        continue

      source_name = normalize_geo_text(row[indexes['Source']])
      raw_region = normalize_geo_text(row[indexes['Region']])
      raw_city = normalize_geo_text(row[indexes['City']])
      region_key = (source_name, country, raw_region)
      triple_key = (source_name, country, raw_region, raw_city)
      evidence.region_totals[region_key] += 1
      evidence.triple_totals[triple_key] += 1
      evidence.stats['accepted_rows'] += 1

      try:
        latitude = float(row[indexes['Latitude']])
        longitude = float(row[indexes['Longitude']])
      except (TypeError, ValueError):
        evidence.stats['invalid_coordinate_rows'] += 1
        latitude = None
        longitude = None

      coordinate_match = None
      if latitude is not None:
        if (not math.isfinite(latitude) or not math.isfinite(longitude) or
            latitude < -90.0 or latitude > 90.0 or
            longitude < -180.0 or longitude > 180.0):
          evidence.stats['invalid_coordinate_rows'] += 1
        elif latitude == 0.0 and longitude == 0.0:
          evidence.stats['zero_coordinate_rows'] += 1
        else:
          coordinate_match = spatial_index.match(
            latitude, longitude, raw_city)
          evidence.stats[
            'coordinate_region_' + coordinate_match.region_status] += 1
          evidence.stats[
            'coordinate_city_' + coordinate_match.city_status] += 1

      if coordinate_match is not None and coordinate_match.region is not None:
        evidence.add_region_vote(
          region_key,
          coordinate_match.region,
          coordinate_match.region_distance,
          (coordinate_match.region_channel_id,),
          'coordinate')
        if coordinate_match.city_channel_id is not None:
          evidence.add_city_vote(
            triple_key,
            coordinate_match.city_channel_id,
            coordinate_match.city_distance)
      else:
        city_ids = canonical.global_aliases.get(
          normalize_geo_name(raw_city), set())
        city_regions = set(
          canonical.points[channel_id].region for channel_id in city_ids)
        if raw_city and len(city_regions) == 1:
          evidence.add_region_vote(
            region_key,
            next(iter(city_regions)),
            None,
            city_ids,
            'exact_city')
          evidence.stats['region_exact_city_fallback_rows'] += 1

      if progress_rows and row_number % progress_rows == 0:
        elapsed = max(time.time() - started, 0.001)
        print(
          'rows={0} accepted={1} rate={2:.0f}/sec'.format(
            row_number, evidence.stats['accepted_rows'], row_number / elapsed),
          file=sys.stderr,
          flush=True)

  evidence.stats['scan_seconds'] = int(time.time() - started)
  return evidence


def save_evidence(path, evidence, metadata):
  parent = os.path.dirname(os.path.abspath(path))
  os.makedirs(parent, exist_ok=True)
  temporary_path = path + '.tmp'
  if os.path.exists(temporary_path):
    os.unlink(temporary_path)
  database = sqlite3.connect(temporary_path)
  try:
    database.executescript('''
      CREATE TABLE metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL);
      CREATE TABLE stats (key TEXT PRIMARY KEY, value INTEGER NOT NULL);
      CREATE TABLE region_totals (
        source TEXT, country TEXT, region TEXT, rows INTEGER,
        PRIMARY KEY (source, country, region));
      CREATE TABLE triple_totals (
        source TEXT, country TEXT, region TEXT, city TEXT, rows INTEGER,
        PRIMARY KEY (source, country, region, city));
      CREATE TABLE region_votes (
        source TEXT, country TEXT, region TEXT, canonical_region TEXT,
        rows INTEGER, coordinate_rows INTEGER, exact_city_rows INTEGER,
        distance_sum REAL, distance_count INTEGER, max_distance REAL,
        channel_ids TEXT,
        PRIMARY KEY (source, country, region, canonical_region));
      CREATE TABLE city_votes (
        source TEXT, country TEXT, region TEXT, city TEXT,
        channel_id INTEGER, rows INTEGER, distance_sum REAL,
        distance_count INTEGER, max_distance REAL,
        PRIMARY KEY (source, country, region, city, channel_id));
    ''')
    database.executemany(
      'INSERT INTO metadata VALUES (?, ?)',
      sorted((key, str(value)) for key, value in metadata.items()))
    database.executemany(
      'INSERT INTO stats VALUES (?, ?)', sorted(evidence.stats.items()))
    database.executemany(
      'INSERT INTO region_totals VALUES (?, ?, ?, ?)',
      (key + (rows,) for key, rows in evidence.region_totals.items()))
    database.executemany(
      'INSERT INTO triple_totals VALUES (?, ?, ?, ?, ?)',
      (key + (rows,) for key, rows in evidence.triple_totals.items()))

    region_rows = []
    for key, targets in evidence.region_votes.items():
      for canonical_region, vote in targets.items():
        region_rows.append(key + (
          canonical_region,
          vote.rows,
          vote.coordinate_rows,
          vote.exact_city_rows,
          vote.distance_sum,
          vote.distance_count,
          vote.max_distance,
          ','.join(str(item) for item in sorted(vote.channel_ids))))
    database.executemany(
      'INSERT INTO region_votes VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)',
      region_rows)

    city_rows = []
    for key, targets in evidence.city_votes.items():
      for channel_id, vote in targets.items():
        city_rows.append(key + (
          channel_id,
          vote.rows,
          vote.distance_sum,
          vote.distance_count,
          vote.max_distance))
    database.executemany(
      'INSERT INTO city_votes VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)', city_rows)
    database.commit()
  finally:
    database.close()
  os.replace(temporary_path, path)


def load_evidence(path, expected_metadata):
  evidence = Evidence()
  database = sqlite3.connect(path)
  try:
    metadata = dict(database.execute('SELECT key, value FROM metadata'))
    for key, expected_value in expected_metadata.items():
      if metadata.get(key) != str(expected_value):
        raise ValueError(
          'Evidence metadata mismatch for {0}: {1!r} != {2!r}'.format(
            key, metadata.get(key), str(expected_value)))
    evidence.stats.update(dict(database.execute('SELECT key, value FROM stats')))
    for source, country, region, rows in database.execute(
        'SELECT source, country, region, rows FROM region_totals'):
      evidence.region_totals[(source, country, region)] = rows
    for source, country, region, city, rows in database.execute(
        'SELECT source, country, region, city, rows FROM triple_totals'):
      evidence.triple_totals[(source, country, region, city)] = rows
    for row in database.execute('''
        SELECT source, country, region, canonical_region, rows,
          coordinate_rows, exact_city_rows, distance_sum, distance_count,
          max_distance, channel_ids
        FROM region_votes'''):
      key = tuple(row[:3])
      vote = Vote()
      vote.rows = row[4]
      vote.coordinate_rows = row[5]
      vote.exact_city_rows = row[6]
      vote.distance_sum = row[7]
      vote.distance_count = row[8]
      vote.max_distance = row[9]
      vote.channel_ids = set(
        int(item) for item in row[10].split(',') if item)
      evidence.region_votes[key][row[3]] = vote
    for row in database.execute('''
        SELECT source, country, region, city, channel_id, rows,
          distance_sum, distance_count, max_distance
        FROM city_votes'''):
      key = tuple(row[:4])
      vote = Vote()
      vote.rows = row[5]
      vote.coordinate_rows = row[5]
      vote.distance_sum = row[6]
      vote.distance_count = row[7]
      vote.max_distance = row[8]
      vote.channel_ids.add(row[4])
      evidence.city_votes[key][row[4]] = vote
  finally:
    database.close()
  return evidence


class RegionResolution(object):
  def __init__(self, status, method, regions):
    self.status = status
    self.method = method
    self.regions = frozenset(regions)


def load_region_overrides(path, canonical):
  result = collections.defaultdict(set)
  if not path:
    return result
  with open(path, 'r', encoding='utf-8', newline='') as source:
    reader = csv.DictReader(source)
    required = {'Source', 'Country', 'Region', 'CanonicalRegion'}
    if reader.fieldnames is None or not required.issubset(reader.fieldnames):
      raise ValueError(
        'Region overrides must contain columns: {0}'.format(
          ','.join(sorted(required))))
    for row in reader:
      canonical_region = normalize_text(row['CanonicalRegion'])
      if canonical_region not in canonical.regions:
        raise ValueError(
          'Unknown canonical region in override: {0}'.format(
            row['CanonicalRegion']))
      key = (
        normalize_text(row['Source']),
        normalize_country(row['Country']),
        normalize_text(row['Region']))
      if key[1] != 'ru':
        raise ValueError(
          'Only RU region overrides are supported: {0}'.format(
            row['Country']))
      result[key].add(canonical_region)
  return result


def resolve_regions(evidence, overrides, minimum_support, minimum_confidence):
  resolutions = {}
  keys = set(evidence.region_totals) | set(evidence.region_votes) | set(overrides)
  for key in keys:
    if key in overrides:
      regions = overrides[key]
      status = 'resolved_override' if len(regions) == 1 else 'split_override'
      resolutions[key] = RegionResolution(status, 'override', regions)
      continue

    votes = evidence.region_votes.get(key, {})
    total_votes = sum(vote.rows for vote in votes.values())
    eligible = sorted(
      (
        (vote.rows, region)
        for region, vote in votes.items()
        if vote.rows >= minimum_support),
      reverse=True)
    if eligible:
      top_rows, target_region = eligible[0]
      confidence = top_rows / float(total_votes)
      if confidence >= minimum_confidence:
        resolutions[key] = RegionResolution(
          'resolved', 'coordinate', (target_region,))
      elif len(eligible) == 2:
        resolutions[key] = RegionResolution(
          'split', 'coordinate', tuple(region for _, region in eligible))
      elif len(eligible) > 2:
        resolutions[key] = RegionResolution('split', 'coordinate', ())
      else:
        resolutions[key] = RegionResolution(
          'low_confidence', 'coordinate', ())
    elif votes:
      resolutions[key] = RegionResolution(
        'insufficient_support', 'coordinate', ())
    else:
      resolutions[key] = RegionResolution('unknown', 'none', ())
  return resolutions


def atomic_csv(path, header, rows):
  temporary_path = path + '.tmp'
  with open(temporary_path, 'w', encoding='utf-8', newline='') as target:
    writer = csv.writer(target)
    writer.writerow(header)
    writer.writerows(rows)
  os.replace(temporary_path, path)


def write_region_outputs(
    output_dir, evidence, resolutions, minimum_support):
  evidence_rows = []
  mapping_rows = []
  conflict_rows = []
  template_rows = []

  for key in sorted(resolutions):
    source, country, raw_region = key
    resolution = resolutions[key]
    votes = evidence.region_votes.get(key, {})
    total_votes = sum(vote.rows for vote in votes.values())
    for canonical_region, vote in sorted(votes.items()):
      share = vote.rows / float(total_votes) if total_votes else 0.0
      row = (
        source, country, raw_region, canonical_region,
        evidence.region_totals.get(key, 0), vote.rows,
        vote.coordinate_rows, vote.exact_city_rows,
        format_number(share), len(vote.channel_ids),
        format_number(vote.mean_distance(), 3),
        format_number(vote.max_distance, 3), resolution.status)
      evidence_rows.append(row)
      if resolution.status == 'split' or not resolution.regions:
        conflict_rows.append(row)

    for canonical_region in sorted(resolution.regions):
      vote = votes.get(canonical_region, Vote())
      share = vote.rows / float(total_votes) if total_votes else 0.0
      mapping_rows.append((
        source, country, raw_region, canonical_region,
        resolution.method, resolution.status,
        evidence.region_totals.get(key, 0), vote.rows,
        format_number(share), len(vote.channel_ids),
        format_number(vote.mean_distance(), 3),
        format_number(vote.max_distance, 3)))

    if resolution.status == 'split':
      for canonical_region, vote in sorted(
          votes.items(), key=lambda item: (-item[1].rows, item[0])):
        if vote.rows >= minimum_support:
          template_rows.append(
            (source, country, raw_region, canonical_region))

    if not votes and not resolution.regions:
      conflict_rows.append((
        source, country, raw_region, '',
        evidence.region_totals.get(key, 0), 0, 0, 0, '', 0, '', '',
        resolution.status))

  evidence_header = (
    'Source', 'Country', 'Region', 'CanonicalRegion', 'Rows',
    'EvidenceRows', 'CoordinateRows', 'ExactCityRows', 'Share',
    'DistinctCanonicalCities', 'MeanDistanceKm', 'MaxDistanceKm', 'Status')
  atomic_csv(
    os.path.join(output_dir, 'region_evidence.csv'),
    evidence_header,
    evidence_rows)
  atomic_csv(
    os.path.join(output_dir, 'region_conflicts.csv'),
    evidence_header,
    conflict_rows)
  atomic_csv(
    os.path.join(output_dir, 'region_mapping.csv'),
    ('Source', 'Country', 'Region', 'CanonicalRegion', 'Method', 'Status',
      'Rows', 'EvidenceRows', 'Confidence', 'DistinctCanonicalCities',
      'MeanDistanceKm', 'MaxDistanceKm'),
    mapping_rows)
  atomic_csv(
    os.path.join(output_dir, 'region_overrides_template.csv'),
    ('Source', 'Country', 'Region', 'CanonicalRegion'),
    template_rows)


def point_alias_candidates(canonical, regions, normalized_city):
  result = set()
  for region in regions:
    result.update(canonical.aliases_by_region[region].get(normalized_city, ()))
  return result


def select_exact_candidate(
    canonical, candidate_ids, raw_city, coordinate_id):
  candidate_ids = set(candidate_ids)
  text_disambiguated = False
  if len(candidate_ids) > 1:
    normalized_text = normalize_text(raw_city)
    text_ids = {
      channel_id for channel_id in candidate_ids
      if any(
        normalize_text(alias) == normalized_text
        for alias in canonical.points[channel_id].aliases)}
    if text_ids and text_ids != candidate_ids:
      candidate_ids = text_ids
      text_disambiguated = True

  if len(candidate_ids) == 1:
    channel_id = next(iter(candidate_ids))
    if text_disambiguated:
      method = 'exact_text_coordinate' \
        if coordinate_id == channel_id else 'exact_text'
    else:
      method = 'exact_coordinate' \
        if coordinate_id == channel_id else 'exact'
    return channel_id, method

  targets = {
    canonical.target_location(canonical.points[channel_id], raw_city)
    for channel_id in candidate_ids}
  if len(targets) == 1 and None not in targets:
    channel_id = coordinate_id \
      if coordinate_id in candidate_ids else min(candidate_ids)
    method = 'coordinate_exact_target' \
      if coordinate_id == channel_id else 'exact_target'
    return channel_id, method

  if coordinate_id in candidate_ids:
    return coordinate_id, 'coordinate_exact'
  return None, None


def fuzzy_candidate(canonical, regions, raw_city, threshold, margin, cache):
  key = (tuple(sorted(regions)), normalize_name(raw_city))
  if key in cache:
    return cache[key]
  normalized_city = key[1]
  scores = []
  for region in regions:
    for channel_id in canonical.point_ids_by_region[region]:
      point = canonical.points[channel_id]
      score = max(
        difflib.SequenceMatcher(
          None, normalized_city, alias, autojunk=False).ratio()
        for alias in point.normalized_aliases)
      scores.append((score, channel_id))
  scores.sort(reverse=True)
  if not scores or scores[0][0] < threshold:
    result = (None, scores[0][0] if scores else 0.0, 'fuzzy_low_score')
  elif len(scores) > 1 and scores[0][0] - scores[1][0] < margin:
    result = (None, scores[0][0], 'fuzzy_ambiguous')
  else:
    result = (scores[0][1], scores[0][0], 'fuzzy')
  cache[key] = result
  return result


def write_city_outputs(output_dir, evidence, resolutions, canonical,
    minimum_support, minimum_confidence, fuzzy_threshold, fuzzy_margin,
    enable_fuzzy):
  mapping_rows = []
  conflict_rows = []
  unresolved_rows = []
  fuzzy_cache = {}
  mapped_keys = 0
  mapped_rows = 0

  for triple_key, rows in sorted(evidence.triple_totals.items()):
    source, country, raw_region, raw_city = triple_key
    region_key = triple_key[:3]
    resolution = resolutions.get(
      region_key, RegionResolution('unknown', 'none', ()))
    regions = resolution.regions
    if not regions:
      unresolved_rows.append(
        triple_key + (rows, 'region_' + resolution.status))
      continue

    if not raw_city:
      if len(regions) != 1:
        unresolved_rows.append(
          triple_key + (rows, 'empty_city_split_region'))
        continue
      target_region = next(iter(regions))
      target_location = 'ru/{0}/'.format(target_region)
      if target_location not in canonical.locations:
        unresolved_rows.append(
          triple_key + (rows, 'missing_region_target'))
        continue
      mapping_rows.append(triple_key + (
        target_region, '', '', target_location, 'region_only', rows, '', '', ''))
      mapped_keys += 1
      mapped_rows += rows
      continue

    normalized_city = normalize_name(raw_city)
    exact_ids = point_alias_candidates(canonical, regions, normalized_city)
    coordinate_votes = {
      channel_id: vote
      for channel_id, vote in evidence.city_votes.get(triple_key, {}).items()
      if canonical.points[channel_id].region in regions}
    coordinate_total = sum(vote.rows for vote in coordinate_votes.values())
    sorted_coordinate_votes = sorted(
      coordinate_votes.items(), key=lambda item: (-item[1].rows, item[0]))
    coordinate_id = None
    coordinate_confidence = 0.0
    if sorted_coordinate_votes:
      coordinate_id, coordinate_vote = sorted_coordinate_votes[0]
      coordinate_confidence = coordinate_vote.rows / float(coordinate_total)
      if (coordinate_vote.rows < minimum_support or
          coordinate_confidence < minimum_confidence):
        coordinate_id = None

    selected_id = None
    method = None
    confidence = None
    reason = None
    if exact_ids:
      selected_id, method = select_exact_candidate(
        canonical, exact_ids, raw_city, coordinate_id)
      if selected_id is not None:
        confidence = coordinate_confidence \
          if coordinate_id == selected_id else 1.0
      else:
        reason = 'exact_ambiguous'
    elif coordinate_id is not None:
      selected_id = coordinate_id
      method = 'coordinate'
      confidence = coordinate_confidence
    elif enable_fuzzy:
      selected_id, confidence, reason = fuzzy_candidate(
        canonical,
        regions,
        raw_city,
        fuzzy_threshold,
        fuzzy_margin,
        fuzzy_cache)
      if selected_id is not None:
        method = 'fuzzy'
        reason = None
    else:
      reason = 'no_match'

    if selected_id is not None:
      point = canonical.points[selected_id]
      target_location = canonical.target_location(point, raw_city)
      if target_location is None:
        reason = 'missing_city_target'
        selected_id = None
      else:
        selected_vote = coordinate_votes.get(selected_id)
        mapping_rows.append(triple_key + (
          point.region,
          target_location.split('/', 2)[2],
          selected_id,
          target_location,
          method,
          rows,
          selected_vote.rows if selected_vote else 0,
          format_number(confidence),
          format_number(
            selected_vote.mean_distance(), 3) if selected_vote else ''))
        mapped_keys += 1
        mapped_rows += rows

    if selected_id is None:
      candidate_ids = set(exact_ids) | set(coordinate_votes)
      if candidate_ids:
        for channel_id in sorted(candidate_ids):
          point = canonical.points[channel_id]
          vote = coordinate_votes.get(channel_id, Vote())
          share = vote.rows / float(coordinate_total) if coordinate_total else 0.0
          conflict_rows.append(triple_key + (
            rows, reason or 'ambiguous', point.region,
            sorted(point.aliases)[0], channel_id, vote.rows,
            format_number(share), format_number(vote.mean_distance(), 3)))
      else:
        unresolved_rows.append(triple_key + (rows, reason or 'no_match'))

  atomic_csv(
    os.path.join(output_dir, 'city_mapping.csv'),
    ('Source', 'Country', 'Region', 'City', 'CanonicalRegion',
      'CanonicalCity', 'CityChannelId', 'TargetLocation', 'Method', 'Rows',
      'EvidenceRows', 'Confidence', 'MeanDistanceKm'),
    mapping_rows)
  atomic_csv(
    os.path.join(output_dir, 'city_conflicts.csv'),
    ('Source', 'Country', 'Region', 'City', 'Rows', 'Reason',
      'CandidateRegion', 'CandidateCity', 'CityChannelId', 'EvidenceRows',
      'Share', 'MeanDistanceKm'),
    conflict_rows)
  atomic_csv(
    os.path.join(output_dir, 'unresolved.csv'),
    ('Source', 'Country', 'Region', 'City', 'Rows', 'Reason'),
    unresolved_rows)
  return {
    'mapped_city_keys': mapped_keys,
    'mapped_rows': mapped_rows,
    'total_city_keys': len(evidence.triple_totals),
    'total_rows': sum(evidence.triple_totals.values()),
    'city_conflict_rows': len(conflict_rows),
    'unresolved_keys': len(unresolved_rows),
  }


def atomic_json(path, value):
  temporary_path = path + '.tmp'
  with open(temporary_path, 'w', encoding='utf-8') as target:
    json.dump(value, target, indent=2, sort_keys=True)
    target.write('\n')
  os.replace(temporary_path, path)


def parse_args(argv):
  parser = argparse.ArgumentParser(
    description='Build RU region and city mappings from aggregated Geo logs.')
  source = parser.add_mutually_exclusive_group(required=True)
  source.add_argument('--geo', help='Aggregated Geo CSV to scan.')
  source.add_argument(
    '--load-evidence', help='Previously generated evidence.sqlite3.')
  parser.add_argument('--canonical-points', required=True)
  parser.add_argument('--locations', required=True)
  parser.add_argument('--output-dir', required=True)
  parser.add_argument('--region-overrides')
  parser.add_argument('--save-evidence')
  parser.add_argument('--max-region-distance-km', type=float, default=50.0)
  parser.add_argument('--max-city-distance-km', type=float, default=25.0)
  parser.add_argument('--region-margin-km', type=float, default=5.0)
  parser.add_argument('--city-margin-km', type=float, default=2.0)
  parser.add_argument('--min-region-support', type=int, default=20)
  parser.add_argument('--min-region-confidence', type=float, default=0.95)
  parser.add_argument('--min-city-support', type=int, default=5)
  parser.add_argument('--min-city-confidence', type=float, default=0.95)
  parser.add_argument('--fuzzy-threshold', type=float, default=0.90)
  parser.add_argument('--fuzzy-margin', type=float, default=0.05)
  parser.add_argument('--disable-fuzzy', action='store_true')
  parser.add_argument('--coordinate-cache-size', type=int, default=500000)
  parser.add_argument('--progress-rows', type=int, default=1000000)
  args = parser.parse_args(argv)

  if args.max_city_distance_km > args.max_region_distance_km:
    parser.error('--max-city-distance-km must not exceed region distance')
  if (args.max_city_distance_km <= 0.0 or
      args.max_region_distance_km <= 0.0 or
      args.region_margin_km < 0.0 or args.city_margin_km < 0.0):
    parser.error('distance limits must be positive and margins non-negative')
  if args.coordinate_cache_size < 1 or args.progress_rows < 0:
    parser.error('cache size must be positive and progress rows non-negative')
  for name in ('min_region_confidence', 'min_city_confidence',
      'fuzzy_threshold', 'fuzzy_margin'):
    value = getattr(args, name)
    if value < 0.0 or value > 1.0:
      parser.error('--{0} must be in [0, 1]'.format(name.replace('_', '-')))
  if args.min_region_support < 1 or args.min_city_support < 1:
    parser.error('minimum support must be positive')
  return args


def main(argv=None):
  args = parse_args(argv)
  os.makedirs(args.output_dir, exist_ok=True)
  canonical = load_canonical(args.canonical_points, args.locations)
  metadata = {
    'evidence_version': EVIDENCE_VERSION,
    'normalization_version': NORMALIZATION_VERSION,
    'canonical_points_sha256': sha256_file(args.canonical_points),
    'locations_sha256': sha256_file(args.locations),
    'max_region_distance_km': args.max_region_distance_km,
    'max_city_distance_km': args.max_city_distance_km,
    'region_margin_km': args.region_margin_km,
    'city_margin_km': args.city_margin_km,
  }

  if args.load_evidence:
    evidence = load_evidence(args.load_evidence, metadata)
    evidence_path = args.load_evidence
  else:
    spatial_index = SpatialIndex(
      canonical,
      args.max_region_distance_km,
      args.max_city_distance_km,
      args.region_margin_km,
      args.city_margin_km,
      args.coordinate_cache_size)
    evidence = scan_geo(
      args.geo, canonical, spatial_index, args.progress_rows)
    evidence_path = args.save_evidence or os.path.join(
      args.output_dir, 'evidence.sqlite3')
    save_evidence(evidence_path, evidence, metadata)

  overrides = load_region_overrides(args.region_overrides, canonical)
  resolutions = resolve_regions(
    evidence,
    overrides,
    args.min_region_support,
    args.min_region_confidence)
  write_region_outputs(
    args.output_dir, evidence, resolutions, args.min_region_support)
  summary = dict(evidence.stats)
  summary.update(write_city_outputs(
    args.output_dir,
    evidence,
    resolutions,
    canonical,
    args.min_city_support,
    args.min_city_confidence,
    args.fuzzy_threshold,
    args.fuzzy_margin,
    not args.disable_fuzzy))
  summary.update({
    'canonical_channels': len(canonical.points),
    'canonical_regions': len(canonical.regions),
    'region_keys': len(resolutions),
    'resolved_region_keys': sum(
      1 for resolution in resolutions.values() if resolution.regions),
    'split_region_keys': sum(
      1 for resolution in resolutions.values()
      if resolution.status == 'split'),
    'override_region_keys': len(overrides),
    'evidence_path': os.path.abspath(evidence_path),
  })
  atomic_json(os.path.join(args.output_dir, 'summary.json'), summary)
  print(json.dumps(summary, indent=2, sort_keys=True))
  return 0


if __name__ == '__main__':
  try:
    sys.exit(main())
  except (OSError, ValueError, sqlite3.Error) as error:
    print('GeoMapper: {0}'.format(error), file=sys.stderr)
    sys.exit(1)
