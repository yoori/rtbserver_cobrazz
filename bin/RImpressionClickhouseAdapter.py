#!/usr/bin/python3.12

import sys
import csv
import argparse
import json

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
    ('ssp_tag_id', None),
    ('ssp_ctr', None),
    ('ssp_viewability', None),
    ('ssp_vtr', None),
    ('page_keywords', None),
    ('expected_post_actions', None),
  ]
  additional_info_field_map = {
    'ssp_tag_id': 'ssp_tag_id',
    'ssp_ctr': 'ctr',
    'ssp_viewability': 'viewability',
    'ssp_vtr': 'vtr',
  }

  writer = csv.writer(sys.stdout)
  writer.writerow([field[0] for field in field_filling])

  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      it = iter(csv.reader(infile))
      next(it)  # skip header - it contains problem
      for row in it:
        additional_info = {}
        page_keywords = row[29] if len(row) >= 31 else ''
        expected_post_actions = row[30] if len(row) >= 32 else ''
        if row:
          additional_info_raw = row[-1].strip()
          if additional_info_raw:
            try:
              additional_info = json.loads(additional_info_raw)
            except json.JSONDecodeError:
              additional_info = {}

        values = []
        for field_name, field_index in field_filling:
          if field_index is not None:
            values.append(row[field_index])
          elif field_name == 'page_keywords':
            values.append(page_keywords)
          elif field_name == 'expected_post_actions':
            actions = expected_post_actions.split(',') if expected_post_actions else []
            values.append("['" + "','".join(actions) + "']" if actions else '[]')
          else:
            values.append(additional_info.get(additional_info_field_map[field_name], ''))
        writer.writerow(values)
