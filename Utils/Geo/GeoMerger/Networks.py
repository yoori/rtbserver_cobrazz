import csv
import ipaddress
import socket
import struct


IPV4_BITS = 32


def prefix_size(prefix):
  return 1 << (IPV4_BITS - prefix)


def network_address(address, prefix):
  return address & ~(prefix_size(prefix) - 1)


def ipv4_to_int(value):
  try:
    return struct.unpack('!I', socket.inet_pton(socket.AF_INET, value))[0]
  except OSError:
    return None


def format_network(address, prefix):
  return '{0}/{1}'.format(
    socket.inet_ntoa(struct.pack('!I', address)), prefix)


def parse_ipv4_network(value):
  network = ipaddress.ip_network(value, strict=False)
  if network.version != 4:
    raise ValueError('{0} is not an IPv4 network'.format(value))
  return int(network.network_address), network.prefixlen


def read_ipv4_database(path):
  with open(path, 'r', encoding='utf-8', newline='') as source:
    reader = csv.reader(source)
    for line_number, row in enumerate(reader, 1):
      if not row:
        continue
      if len(row) != 2:
        raise ValueError(
          'IPv4 CSV line {0} has {1} columns'.format(
            line_number, len(row)))
      try:
        address, prefix = parse_ipv4_network(row[0])
      except ValueError as error:
        raise ValueError(
          'IPv4 CSV line {0}: {1}'.format(line_number, error))
      yield address, prefix, row[1]


def summarize_range(first, last):
  while first <= last:
    alignment = first & -first
    if alignment == 0:
      alignment = 1 << IPV4_BITS
    remaining = last - first + 1
    size = min(alignment, 1 << (remaining.bit_length() - 1))
    prefix = IPV4_BITS - (size.bit_length() - 1)
    yield first, prefix
    first += size


def largest_containing_network(address, first, last, minimum_prefix):
  for prefix in range(minimum_prefix, IPV4_BITS + 1):
    candidate = network_address(address, prefix)
    if candidate >= first and candidate + prefix_size(prefix) - 1 <= last:
      return candidate, prefix
  raise ValueError('Address is outside the supplied range')


def uncovered_networks(first, last, intervals):
  cursor = first
  merged_first = None
  merged_last = None
  for interval_first, interval_last in sorted(intervals):
    interval_first = max(interval_first, first)
    interval_last = min(interval_last, last)
    if interval_first > interval_last:
      continue
    if merged_last is None:
      merged_first = interval_first
      merged_last = interval_last
    elif interval_first <= merged_last + 1:
      merged_last = max(merged_last, interval_last)
    else:
      if cursor < merged_first:
        yield from summarize_range(cursor, merged_first - 1)
      cursor = max(cursor, merged_last + 1)
      merged_first = interval_first
      merged_last = interval_last

  if merged_last is not None:
    if cursor < merged_first:
      yield from summarize_range(cursor, merged_first - 1)
    cursor = max(cursor, merged_last + 1)
  if cursor <= last:
    yield from summarize_range(cursor, last)
