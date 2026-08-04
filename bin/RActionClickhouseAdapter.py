#!/usr/bin/env python3

import sys
import csv
import argparse


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='RAction ClickHouse adapter.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  field_filling = [  # < Order of fields in clickhouse table RAction.
    ('timestamp', 0),
    ('device', 1),
    ('ip', 2),
    ('uid', 3),
    ('url', 4),
    ('action_id', 5),
    ('order_id', 6),
    ('order_value', 7),
  ]

  writer = csv.writer(sys.stdout)

  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      it = iter(csv.reader(infile))
      next(it)  # skip header
      for row in it:
        writer.writerow([
          row[field_index] if field_index < len(row) else ''
          for _, field_index in field_filling
        ])
