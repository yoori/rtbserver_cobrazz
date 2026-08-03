#!/usr/bin/env python3
"""Convert AdvertiserAction TSV log files to CSV for ClickHouse INSERT."""

import argparse
import csv
import sys


FIELDS = (
  'time',
  'user_id',
  'request_id',
  'action_id',
  'device_channel_id',
  'action_request_id',
  'ccg_ids',
  'referrer',
  'order_id',
  'ip',
  'cur_value',
)


def strip_optional(value: str) -> str:
  if value is None:
    return ''
  value = value.strip()
  if value in ('', '-'):
    return ''
  if value.startswith('@'):
    return value[1:]
  return value


def to_uint(value: str) -> str:
  value = strip_optional(value)
  if not value:
    return '0'
  return value


def to_time(value: str) -> str:
  value = strip_optional(value)
  if not value:
    return ''
  # AdvertiserAction uses YYYY-MM-DD_HH:MM:SS
  return value.replace('_', ' ', 1)


def to_float(value: str) -> str:
  value = strip_optional(value)
  if not value:
    return '0'
  return value


def convert_row(row):
  if len(row) < 11:
    return None
  return [
    to_time(row[0]),
    strip_optional(row[1]),
    strip_optional(row[2]),
    to_uint(row[3]),
    to_uint(row[4]),
    strip_optional(row[5]),
    strip_optional(row[6]),
    strip_optional(row[7]),
    strip_optional(row[8]),
    strip_optional(row[9]),
    to_float(row[10]),
  ]


def main():
  parser = argparse.ArgumentParser(
    description='Adapt AdvertiserAction TSV logs for ClickHouse CSVWithNames insert.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  writer = csv.writer(sys.stdout, lineterminator='\n')
  writer.writerow(FIELDS)

  for read_file in args.filename:
    with open(read_file, 'r', encoding='utf-8', errors='replace') as infile:
      reader = csv.reader(infile, delimiter='\t')
      try:
        next(reader)  # skip "AdvertiserAction\t3.6"
      except StopIteration:
        continue
      for row in reader:
        if not row or (len(row) == 1 and not row[0].strip()):
          continue
        out = convert_row(row)
        if out is not None:
          writer.writerow(out)


if __name__ == '__main__':
  main()
