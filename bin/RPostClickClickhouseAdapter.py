#!/usr/bin/python3.12

import argparse
import csv
import sys


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='RPostClick ClickHouse adapter.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  writer = csv.writer(sys.stdout)
  writer.writerow([
    'request_id',
    'landing_bounced',
    'landing_session_time',
    'landing_page_views',
    'landing_is_new_user',
  ])

  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      rows = iter(csv.reader(infile))
      next(rows)
      for row in rows:
        writer.writerow(row)
