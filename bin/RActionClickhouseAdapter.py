#!/usr/bin/env python3

import sys
import csv
import argparse


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='RAction ClickHouse adapter.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  field_filling = [  # < Order of fields in clickhouse table RAction.
    ('timestamp', 0, False),
    ('device', 1, True),
    ('ip', 2, False),
    ('uid', 3, False),
    ('url', 4, False),
    ('action_id', 5, True),
    ('order_id', 6, False),
    ('order_value', 7, False),
  ]

  writer = csv.writer(sys.stdout)

  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      it = iter(csv.reader(infile))
      next(it)  # skip header
      for row in it:
        writer.writerow([
          ('\\N' if nullable and (field_index >= len(row) or not row[field_index]) else
           row[field_index] if field_index < len(row) else '')
          for _, field_index, nullable in field_filling
        ])
