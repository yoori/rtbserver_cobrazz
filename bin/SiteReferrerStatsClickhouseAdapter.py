#!/usr/bin/python3.12

import argparse
import csv
import sys


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='SiteReferrerStats ClickHouse adapter.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  writer = csv.writer(sys.stdout)
  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      rows = iter(csv.reader(infile))
      next(rows)
      for row in rows:
        writer.writerow(row)
