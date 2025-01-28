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
    ('timestamp', None),
    ('device', None),
    ('ip', None),
    ('uid', None),
    ('url', None),
    ('publisher_id', None),
    ('tag_id', None),
    ('etag', None),
    ('campaign_id', None),
    ('ccg_id', None),
    ('ccid', None),
    ('geo_ch', None),
    ('user_ch',  None),
    ('imp_ch', None),
    ('bid_price', None),
    ('bid_floor', None),
    ('alg_id', None),
    ('size_id', None),
    ('colo_id', None),
    ('predicted_ctr', None),
    ('campaign_freq', None),
    ('cr_alg_id', None),
    ('predicted_cr', None),
    ('win_price', None),
    ('viewability', None),
    ('click_timestamp', 0),
  ]

  writer = csv.writer(sys.stdout)
  writer.writerow([field[0] for field in field_filling])

  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      it = iter(csv.reader(infile))
      next(it)  # skip header - it contains problem
      for row in it:
        writer.writerow([(row[field_index] if field_index is not None else '') for _, field_index in field_filling])
