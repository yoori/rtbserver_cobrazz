import bisect
import collections
import csv
import os
import sys
import time

from GeoMapper import (
  normalize_geo_country as normalize_country,
  normalize_geo_text as normalize_text)

from .Boundaries import PARENT_PREFIX
from .Networks import (
  format_network,
  ipv4_to_int,
  network_address,
  prefix_size,
  read_ipv4_database,
  uncovered_networks)


REQUIRED_GEO_COLUMNS = ('IP', 'Source', 'Country', 'Region', 'City')
REQUIRED_MAPPING_COLUMNS = (
  'Source', 'Country', 'Region', 'City', 'TargetLocation')


class SubnetRecord(object):
  __slots__ = (
    'address', 'prefix', 'location', 'tokens', 'unique_ips',
    'selected_votes', 'evidence_votes')

  def __init__(self, address, prefix, location, tokens, unique_ips,
      selected_votes, evidence_votes):
    self.address = address
    self.prefix = prefix
    self.location = location
    self.tokens = tokens
    self.unique_ips = unique_ips
    self.selected_votes = selected_votes
    self.evidence_votes = evidence_votes

  def confidence(self):
    if not self.evidence_votes:
      return 0.0
    return float(self.selected_votes) / self.evidence_votes

  def interval(self):
    return (
      self.address,
      self.address + prefix_size(self.prefix) - 1)


class IntervalIndex(object):
  def __init__(self, records):
    intervals = list(
      (record.interval()[0], record.interval()[1], record)
      for record in records)
    intervals.sort(key=lambda item: (item[0], item[1]))
    previous_last = -1
    for first, last, _ in intervals:
      if first <= previous_last:
        raise ValueError('Selected Geo subnets overlap')
      previous_last = last
    self.intervals = intervals
    self.starts = [item[0] for item in intervals]

  def overlaps(self, first, last):
    if not self.intervals:
      return ()
    index = bisect.bisect_right(self.starts, first) - 1
    if index < 0 or self.intervals[index][1] < first:
      index += 1
    result = []
    while index < len(self.intervals):
      interval_first, interval_last, record = self.intervals[index]
      if interval_first > last:
        break
      if interval_last >= first:
        result.append((interval_first, interval_last, record))
      index += 1
    return result


def mapping_key(source, country, region, city):
  return (
    normalize_text(source),
    normalize_country(country),
    normalize_text(region),
    normalize_text(city))


def load_mapping(path):
  result = {}
  with open(path, 'r', encoding='utf-8', newline='') as source:
    reader = csv.DictReader(source)
    required = set(REQUIRED_MAPPING_COLUMNS)
    if reader.fieldnames is None or not required.issubset(reader.fieldnames):
      raise ValueError(
        'Mapping must contain columns: {0}'.format(','.join(sorted(required))))
    for row in reader:
      key = mapping_key(
        row['Source'], row['Country'], row['Region'], row['City'])
      location = normalize_text(row['TargetLocation'])
      previous = result.get(key)
      if previous is not None and previous != location:
        raise ValueError('Conflicting mapping for {0!r}'.format(key))
      result[key] = location
  return result


def validate_geo_reader(reader):
  required = set(REQUIRED_GEO_COLUMNS)
  if reader.fieldnames is None or not required.issubset(reader.fieldnames):
    raise ValueError(
      'Geo CSV must contain columns: {0}'.format(','.join(sorted(required))))


def mapped_row(row, mapping):
  location = mapping.get(mapping_key(
    row['Source'], row['Country'], row['Region'], row['City']))
  if location is None:
    return None
  ip_value = row['IP'].strip()
  address = ipv4_to_int(ip_value)
  if address is None or address == 0:
    return location, ip_value, None, None
  layer = 'high' if normalize_text(row['Source']) == 'yandex' else 'low'
  return location, ip_value, address, layer


def print_progress(prefix, row_number, started):
  elapsed = max(time.time() - started, 0.001)
  print(
    '{0} rows={1} rate={2:.0f}/sec'.format(
      prefix, row_number, row_number / elapsed),
    file=sys.stderr,
    flush=True)


def scan_candidate_parents(path, mapping, progress_rows):
  parents = set()
  stats = collections.Counter()
  started = time.time()
  with open(path, 'r', encoding='utf-8', newline='') as source:
    reader = csv.DictReader(source)
    validate_geo_reader(reader)
    for row_number, row in enumerate(reader, 1):
      stats['input_rows'] += 1
      if progress_rows and row_number % progress_rows == 0:
        print_progress('boundary-scan', row_number, started)

      result = mapped_row(row, mapping)
      if result is None:
        stats['unmapped_rows'] += 1
        continue
      _, ip_value, address, layer = result
      if address is None:
        if ':' in ip_value:
          stats['ipv6_rows'] += 1
        elif ip_value:
          stats['invalid_ip_rows'] += 1
        else:
          stats['empty_ip_rows'] += 1
        continue
      parents.add(network_address(address, PARENT_PREFIX))
      stats[layer + '_mapped_rows'] += 1

  stats['candidate_parent_subnets'] = len(parents)
  stats['boundary_scan_seconds'] = int(time.time() - started)
  return parents, stats


def collect_unique_votes(path, mapping, resolver, progress_rows):
  votes = collections.Counter()
  cell_unique_ips = collections.Counter()
  stats = collections.Counter()
  started = time.time()
  current_address = None
  locations = {'low': set(), 'high': set()}

  def flush():
    if current_address is None:
      return
    address, prefix, tokens = resolver.resolve(current_address)
    for layer in ('low', 'high'):
      if not locations[layer]:
        continue
      cell = (layer, address, prefix, tokens)
      cell_unique_ips[cell] += 1
      stats[layer + '_mapped_unique_ips'] += 1
      for location in locations[layer]:
        votes[cell + (location,)] += 1

  with open(path, 'r', encoding='utf-8', newline='') as source:
    reader = csv.DictReader(source)
    validate_geo_reader(reader)
    for row_number, row in enumerate(reader, 1):
      if progress_rows and row_number % progress_rows == 0:
        print_progress('vote-scan', row_number, started)
      result = mapped_row(row, mapping)
      if result is None:
        continue
      location, _, address, layer = result
      if address is None:
        continue
      if current_address != address:
        flush()
        current_address = address
        locations = {'low': set(), 'high': set()}
      locations[layer].add(location)
  flush()

  stats['vote_scan_seconds'] = int(time.time() - started)
  stats['vote_cells'] = len(cell_unique_ips)
  return votes, cell_unique_ips, stats


def select_subnets(votes, cell_unique_ips, minimum_unique_ips,
    minimum_confidence, conflicts_path):
  candidates = collections.defaultdict(list)
  for key, unique_ips in votes.items():
    candidates[key[:-1]].append((key[-1], unique_ips))

  selected = {'low': {}, 'high': {}}
  reports = []
  stats = collections.Counter()
  for cell, locations in candidates.items():
    layer, address, prefix, tokens = cell
    locations.sort(key=lambda item: (-item[1], item[0]))
    unique_ips = cell_unique_ips[cell]
    evidence_votes = sum(item[1] for item in locations)
    top_location, top_votes = locations[0]
    tied = len(locations) > 1 and locations[1][1] == top_votes
    confidence = float(top_votes) / evidence_votes
    required_unique_ips = min(minimum_unique_ips, prefix_size(prefix))
    reason = 'selected'
    if len(locations) > 1:
      stats[layer + '_conflicting_subnets'] += 1
    if tied:
      reason = 'tie'
      stats[layer + '_tied_subnets'] += 1
    elif unique_ips < required_unique_ips:
      reason = 'insufficient_unique_ips'
      stats[layer + '_insufficient_unique_ip_subnets'] += 1
    elif confidence < minimum_confidence:
      reason = 'low_confidence'
      stats[layer + '_low_confidence_subnets'] += 1

    if reason == 'selected':
      record = SubnetRecord(
        address,
        prefix,
        top_location,
        tokens,
        unique_ips,
        top_votes,
        evidence_votes)
      network_key = (address, prefix)
      if network_key in selected[layer]:
        raise ValueError(
          'Multiple boundary identities produced {0}'.format(
            format_network(address, prefix)))
      selected[layer][network_key] = record
      stats[layer + '_selected_subnets'] += 1
      stats[layer + '_selected_unique_ips'] += unique_ips

    if len(locations) > 1 or reason != 'selected':
      for location, location_votes in locations:
        reports.append((
          layer,
          address,
          prefix,
          location,
          location_votes,
          unique_ips,
          confidence,
          'yes' if reason == 'selected' and location == top_location else 'no',
          reason))

  write_conflicts(conflicts_path, reports)
  return selected, stats


def write_conflicts(path, reports):
  if not path:
    return
  os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
  temporary_path = path + '.tmp'
  with open(temporary_path, 'w', encoding='utf-8', newline='') as target:
    writer = csv.writer(target, lineterminator='\n')
    writer.writerow((
      'Priority', 'Subnet', 'TargetLocation', 'UniqueIPVotes',
      'CellUniqueIPs', 'WinnerConfidence', 'Selected', 'Reason'))
    for item in sorted(reports, key=lambda value: (
        value[0], value[1], value[2], -value[4], value[3])):
      writer.writerow((
        item[0],
        format_network(item[1], item[2]),
        item[3],
        item[4],
        item[5],
        '{0:.6f}'.format(item[6]),
        item[7],
        item[8]))
  os.replace(temporary_path, path)


def merge_subnets(selected, maximum_merged_prefix):
  stats = collections.Counter()
  result = {'low': {}, 'high': {}}
  for layer in ('low', 'high'):
    records = dict(selected[layer])
    if records:
      largest_prefix = max(prefix for _, prefix in records)
    else:
      largest_prefix = maximum_merged_prefix
    for prefix in range(largest_prefix, maximum_merged_prefix, -1):
      child_size = prefix_size(prefix)
      parent_prefix = prefix - 1
      parent_size = prefix_size(parent_prefix)
      addresses = sorted(
        address for address, item_prefix in records if item_prefix == prefix)
      for address in addresses:
        if address % parent_size != 0:
          continue
        left_key = (address, prefix)
        right_key = (address + child_size, prefix)
        left = records.get(left_key)
        right = records.get(right_key)
        if left is None or right is None:
          continue
        if left.location != right.location or left.tokens != right.tokens:
          continue
        parent_key = (address, parent_prefix)
        if parent_key in records:
          raise ValueError(
            'Overlapping selected subnet {0}'.format(
              format_network(address, parent_prefix)))
        del records[left_key]
        del records[right_key]
        records[parent_key] = SubnetRecord(
          address,
          parent_prefix,
          left.location,
          left.tokens,
          left.unique_ips + right.unique_ips,
          left.selected_votes + right.selected_votes,
          left.evidence_votes + right.evidence_votes)
        stats[layer + '_merged_to_' + str(parent_prefix)] += 1
    stats[layer + '_output_subnets_before_overlay'] = len(records)
    result[layer] = records
  return result, stats


def build_output(existing_path, output_path, selected):
  low_records = list(selected['low'].values())
  high_records = list(selected['high'].values())
  low_index = IntervalIndex(low_records)
  high_index = IntervalIndex(high_records)
  low_blockers = collections.defaultdict(list)
  stats = collections.Counter()

  for first, prefix, _ in read_ipv4_database(existing_path):
    last = first + prefix_size(prefix) - 1
    stats['medium_records'] += 1
    for _, _, low_record in low_index.overlaps(first, last):
      low_first, low_last = low_record.interval()
      low_blockers[(low_record.address, low_record.prefix)].append((
        max(first, low_first), min(last, low_last)))

  for high_record in high_records:
    first, last = high_record.interval()
    for _, _, low_record in low_index.overlaps(first, last):
      low_first, low_last = low_record.interval()
      low_blockers[(low_record.address, low_record.prefix)].append((
        max(first, low_first), min(last, low_last)))

  os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
  temporary_path = output_path + '.tmp'
  with open(temporary_path, 'w', encoding='utf-8', newline='') as target:
    writer = csv.writer(target, lineterminator='\n')

    for record in sorted(low_records, key=lambda item: item.interval()):
      first, last = record.interval()
      output_count = 0
      for address, prefix in uncovered_networks(
          first,
          last,
          low_blockers.get((record.address, record.prefix), ())):
        writer.writerow((format_network(address, prefix), record.location))
        output_count += 1
        stats['low_output_records'] += 1
      if output_count == 0:
        stats['low_fully_shadowed'] += 1
      elif output_count > 1 or low_blockers.get(
          (record.address, record.prefix)):
        stats['low_partially_shadowed'] += 1

    for first, prefix, location in read_ipv4_database(existing_path):
      last = first + prefix_size(prefix) - 1
      covered = any(
        high_first <= first and high_last >= last
        for high_first, high_last, _ in high_index.overlaps(first, last))
      if covered:
        stats['medium_records_shadowed_by_high'] += 1
        continue
      writer.writerow((format_network(first, prefix), location))
      stats['medium_output_records'] += 1

    for record in sorted(high_records, key=lambda item: item.interval()):
      writer.writerow((
        format_network(record.address, record.prefix), record.location))
      stats['high_output_records'] += 1

  os.replace(temporary_path, output_path)
  stats['output_records'] = (
    stats['low_output_records'] + stats['medium_output_records'] +
    stats['high_output_records'])
  return stats
