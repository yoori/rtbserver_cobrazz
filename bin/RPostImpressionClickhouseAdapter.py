#!/usr/bin/python3.12

import argparse
import csv
import sys


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='RPostImpression ClickHouse adapter.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()

  action_fields = (
    ('vstart', 'video_start_timestamp'),
    ('vview', 'video_view_timestamp'),
    ('vq1', 'video_q1_timestamp'),
    ('vmid', 'video_mid_timestamp'),
    ('vq3', 'video_q3_timestamp'),
    ('vcomplete', 'video_complete_timestamp'),
    ('vskip', 'video_skip_timestamp'),
    ('vpause', 'video_pause_timestamp'),
    ('vmute', 'video_mute_timestamp'),
    ('vunmute', 'video_unmute_timestamp'),
    ('vresume', 'video_resume_timestamp'),
    ('vfullscreen', 'video_fullscreen_timestamp'),
    ('verror', 'video_error_timestamp'),
  )
  action_indexes = {
    action_name: index
    for index, (action_name, _) in enumerate(action_fields)
  }

  writer = csv.writer(sys.stdout)
  writer.writerow(['request_id'] + [field_name for _, field_name in action_fields])

  for read_file in args.filename:
    with open(read_file, 'r') as infile:
      rows = iter(csv.reader(infile))
      next(rows)
      for row in rows:
        action_index = action_indexes.get(row[2])
        if action_index is None:
          continue

        timestamps = [''] * len(action_fields)
        timestamps[action_index] = row[0]
        writer.writerow([row[1]] + timestamps)
