#!/usr/bin/env python3

import sys
import csv
import argparse

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Default yolo11 model training script.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  field_filling = [  # < Order of fields in clickhouse table RImpression.
    ('request_id', 1),
    ('timestamp', 0),
    ('device', 3),
    ('ip', 4),
    ('uid', 6),
    ('url', 7),
    ('publisher_id', 8),
    ('tag_id', 9),
    ('etag', 10),
    ('campaign_id', 11),
    ('ccg_id', 12),
    ('ccid', 13),
    ('geo_ch', 14),
    ('user_ch', 15),
    ('imp_ch', 16),
    ('bid_price', 17),
    ('bid_floor', 18),
    ('alg_id', 19),
    ('size_id', 20),
    ('colo_id', 21),
    ('predicted_ctr', 22),
    ('campaign_freq', 23),
    ('cr_alg_id', 24),
    ('predicted_cr', 25),
    ('win_price', 27),
    ('viewability', 28),
  ]

  writer = csv.writer(sys.stdout)
  writer.writerow([field[0] for field in field_filling])

  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      it = iter(csv.reader(infile))
      next(it)  # skip header - it contains problem
      for row in it:
        writer.writerow([row[field_index] for _, field_index in field_filling])
