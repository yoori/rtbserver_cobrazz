#!/usr/bin/python3.12

import argparse
import csv
import os
import sys


HEADERS = {
  'ChannelTriggerStatsCH': [
    'sdate', 'colo_id', 'channel_trigger_id', 'trigger_type', 'hits', 'channel_id'],
  'ChannelTriggerImpStatsCH': [
    'sdate', 'colo_id', 'channel_trigger_id', 'trigger_type',
    'approximated_imps', 'approximated_clicks', 'channel_id'],
}


def convert(filename, writer):
  basename = os.path.basename(filename)
  source_type = basename.split('_', 1)[0]
  expected_header = HEADERS[source_type]
  with open(filename, newline = '') as input_file:
    reader = csv.reader(input_file)
    header = next(reader)
    if header != expected_header:
      raise ValueError('Unexpected header in ' + basename)
    for row_number, row in enumerate(reader, 1):
      if len(row) != len(expected_header):
        raise ValueError('Unexpected row width in ' + basename)
      if source_type == 'ChannelTriggerStatsCH':
        date, colo, trigger, kind, hits, channel = row
        writer.writerow([date, colo, trigger, kind, channel, hits, 0, 0,
                         basename, row_number])
      else:
        date, colo, trigger, kind, imps, clicks, channel = row
        writer.writerow([date, colo, trigger, kind, channel, 0, imps, clicks,
                         basename, row_number])


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description = 'Channel trigger ClickHouse adapter.')
  parser.add_argument('filename', nargs = '+')
  args = parser.parse_args()
  output = csv.writer(sys.stdout)
  for source_file in args.filename:
    convert(source_file, output)
