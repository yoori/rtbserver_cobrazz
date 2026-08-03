#!/usr/bin/env python3
"""Convert RAction research CSV logs to ClickHouse CSVWithNames."""

import argparse
import csv
import sys

# ResearchActionTraits::csv_header()
SRC_FIELDS = [
  'Timestamp',
  'Device',
  'IP Address',
  'UID',
  'URL',
  'Action ID',
  'Order ID',
  'Order Value',
]

OUT_FIELDS = [
  'time',
  'device_channel_id',
  'ip',
  'user_id',
  'referrer',
  'action_id',
  'order_id',
  'cur_value',
]


def norm(value: str) -> str:
  if value is None:
    return ''
  value = value.strip()
  if value in ('', '-'):
    return ''
  if value.startswith('@'):
    return value[1:]
  return value


def to_uint(value: str) -> str:
  value = norm(value)
  return value if value else '0'


def to_float(value: str) -> str:
  value = norm(value)
  return value if value else '0'


def to_time(value: str) -> str:
  value = norm(value)
  if not value:
    return ''
  # accept both "YYYY-MM-DD HH:MM:SS" and "YYYY-MM-DD_HH:MM:SS"
  return value.replace('_', ' ', 1)


def convert_row(row_map):
  return [
    to_time(row_map.get('Timestamp', '')),
    to_uint(row_map.get('Device', '')),
    norm(row_map.get('IP Address', '')),
    norm(row_map.get('UID', '')),
    norm(row_map.get('URL', '')),
    to_uint(row_map.get('Action ID', '')),
    norm(row_map.get('Order ID', '')),
    to_float(row_map.get('Order Value', '')),
  ]


def main():
  parser = argparse.ArgumentParser(
    description='Adapt RAction CSV logs for ClickHouse CSVWithNames insert.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  writer = csv.writer(sys.stdout, lineterminator='\n')
  writer.writerow(OUT_FIELDS)

  for read_file in args.filename:
    with open(read_file, 'r', encoding='utf-8', errors='replace') as infile:
      reader = csv.DictReader(infile)
      # tolerate header drift / missing names
      if reader.fieldnames is None:
        continue
      for row in reader:
        if not row:
          continue
        writer.writerow(convert_row(row))


if __name__ == '__main__':
  main()
