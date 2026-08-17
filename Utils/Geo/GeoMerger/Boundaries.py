import bisect
import collections
import csv
import gzip
import ipaddress

from .Networks import network_address, parse_ipv4_network, prefix_size


PARENT_PREFIX = 23


class Boundary(object):
  __slots__ = ('first', 'last', 'token')

  def __init__(self, first, last, token):
    self.first = first
    self.last = last
    self.token = token

  def size(self):
    return self.last - self.first + 1


class BoundarySegment(object):
  __slots__ = ('first', 'last', 'boundary')

  def __init__(self, first, last, boundary):
    self.first = first
    self.last = last
    self.boundary = boundary


class BoundaryIndex(object):
  def __init__(self, name, candidate_parents, cap_prefix=None):
    self.name = name
    self.cap_prefix = cap_prefix
    self.candidate_parents = tuple(sorted(candidate_parents))
    self.boundaries = collections.defaultdict(list)
    self.segments = {}
    self.objects = 0

  def add(self, first, last, token):
    if first < 0 or last > 0xFFFFFFFF or first > last:
      raise ValueError(
        'Invalid {0} boundary: {1}-{2}'.format(self.name, first, last))
    first_parent = network_address(first, PARENT_PREFIX)
    last_parent = network_address(last, PARENT_PREFIX)
    left = bisect.bisect_left(self.candidate_parents, first_parent)
    right = bisect.bisect_right(self.candidate_parents, last_parent)
    if left == right:
      return
    boundary = Boundary(first, last, token)
    for parent in self.candidate_parents[left:right]:
      self.boundaries[parent].append(boundary)
    self.objects += 1

  def finalize(self):
    for parent, boundaries in self.boundaries.items():
      boundaries.sort(key=lambda item: (item.size(), item.first, item.last))
      parent_last = parent + prefix_size(PARENT_PREFIX) - 1
      points = {parent, parent_last + 1}
      for boundary in boundaries:
        points.add(max(parent, boundary.first))
        points.add(min(parent_last, boundary.last) + 1)
      points = sorted(points)

      segments = []
      for index in range(len(points) - 1):
        first = points[index]
        last = points[index + 1] - 1
        selected = next((
          boundary for boundary in boundaries
          if boundary.first <= first and boundary.last >= last), None)
        if segments and segments[-1].boundary is selected:
          segments[-1].last = last
        else:
          segments.append(BoundarySegment(first, last, selected))
      self.segments[parent] = (
        tuple(segment.first for segment in segments), tuple(segments))
    self.boundaries.clear()

  def lookup(self, address):
    parent = network_address(address, PARENT_PREFIX)
    indexed = self.segments.get(parent)
    if indexed is None:
      return None
    starts, segments = indexed
    index = bisect.bisect_right(starts, address) - 1
    return segments[index]

  def merge_token(self, segment, address):
    if segment is None or segment.boundary is None:
      return None
    boundary = segment.boundary
    token = boundary.token
    if (self.cap_prefix is not None and
        boundary.size() > prefix_size(self.cap_prefix)):
      token = (token, network_address(address, self.cap_prefix))
    return token


class BoundaryResolver(object):
  def __init__(self, indexes, base_prefix):
    self.indexes = tuple(indexes)
    self.base_prefix = base_prefix

  def resolve(self, address):
    first = network_address(address, self.base_prefix)
    last = first + prefix_size(self.base_prefix) - 1
    tokens = []
    for index in self.indexes:
      segment = index.lookup(address)
      tokens.append(index.merge_token(segment, address))
      if segment is not None:
        first = max(first, segment.first)
        last = min(last, segment.last)

    prefix = self.base_prefix
    while prefix <= 32:
      candidate = network_address(address, prefix)
      if candidate >= first and candidate + prefix_size(prefix) - 1 <= last:
        return candidate, prefix, tuple(tokens)
      prefix += 1
    raise ValueError('Unable to resolve boundaries for IPv4 address')


def load_old_database_boundaries(path, index):
  stats = collections.Counter()
  with open(path, 'r', encoding='utf-8', newline='') as source:
    reader = csv.reader(source)
    for line_number, row in enumerate(reader, 1):
      if not row:
        continue
      if len(row) != 2:
        raise ValueError(
          'Existing IPv4 CSV line {0} has {1} columns'.format(
            line_number, len(row)))
      try:
        first, prefix = parse_ipv4_network(row[0])
      except ValueError as error:
        raise ValueError(
          'Existing IPv4 CSV line {0}: {1}'.format(line_number, error))
      stats['old_records'] += 1
      if prefix <= 24:
        continue
      last = first + prefix_size(prefix) - 1
      index.add(first, last, ('old', first, prefix))
      stats['old_specific_boundaries'] += 1
  index.finalize()
  stats['old_indexed_boundaries'] = index.objects
  return stats


def open_text(path):
  if path.endswith('.gz'):
    return gzip.open(path, 'rt', encoding='utf-8', errors='replace')
  return open(path, 'r', encoding='utf-8', errors='replace')


def load_rir_database(path, index):
  stats = collections.Counter()
  with open_text(path) as source:
    for line_number, line in enumerate(source, 1):
      if not line.lower().startswith('inetnum:'):
        continue
      value = line.split(':', 1)[1].strip()
      parts = value.split('-', 1)
      if len(parts) != 2:
        stats['rir_invalid_inetnum'] += 1
        continue
      try:
        first = int(ipaddress.IPv4Address(parts[0].strip()))
        last = int(ipaddress.IPv4Address(parts[1].strip()))
      except ipaddress.AddressValueError:
        stats['rir_invalid_inetnum'] += 1
        continue
      if first > last:
        raise ValueError(
          'RIR database line {0} has a reversed inetnum'.format(line_number))
      index.add(first, last, ('rir', first, last))
      stats['rir_inetnum_objects'] += 1
  return stats


def load_bgp_prefixes(path, index):
  stats = collections.Counter()
  prefixes = collections.defaultdict(set)
  with open(path, 'r', encoding='utf-8', newline='') as source:
    sample = source.read(8192)
    source.seek(0)
    try:
      dialect = csv.Sniffer().sniff(sample, delimiters=',\t')
    except csv.Error:
      dialect = csv.excel
    reader = csv.DictReader(source, dialect=dialect)
    if reader.fieldnames is None:
      raise ValueError('BGP prefix file has no header')
    names = {name.strip().casefold(): name for name in reader.fieldnames}
    prefix_column = names.get('prefix') or names.get('network')
    origin_column = (
      names.get('originasn') or names.get('origin_asn') or
      names.get('origin') or names.get('asn'))
    if prefix_column is None:
      raise ValueError('BGP prefix file must contain Prefix or Network')

    for line_number, row in enumerate(reader, 2):
      try:
        first, prefix = parse_ipv4_network(row[prefix_column].strip())
      except (AttributeError, ValueError) as error:
        raise ValueError(
          'BGP prefix file line {0}: {1}'.format(line_number, error))
      origin = '' if origin_column is None else row[origin_column].strip()
      prefixes[(first, prefix)].add(origin)
      stats['bgp_rows'] += 1

  for (first, prefix), origins in prefixes.items():
    last = first + prefix_size(prefix) - 1
    index.add(
      first,
      last,
      ('bgp', first, prefix, tuple(sorted(origins))))
  stats['bgp_prefixes'] = len(prefixes)
  return stats
