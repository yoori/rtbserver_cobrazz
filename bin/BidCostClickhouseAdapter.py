#!/usr/bin/python3.12

import argparse
import csv
import sys


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='BidCost ClickHouse adapter.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  writer = csv.writer(sys.stdout)

  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      it = iter(csv.reader(infile))
      next(it)  # skip header
      for row in it:
        writer.writerow(row)
