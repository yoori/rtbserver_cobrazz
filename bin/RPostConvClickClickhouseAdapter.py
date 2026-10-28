#!/usr/bin/python3.12

import argparse
import csv
import sys


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='RPostConvClick ClickHouse adapter.')
  parser.add_argument('filename', nargs='+')
  args = parser.parse_args()
  writer = csv.writer(sys.stdout)
  writer.writerow((
    'request_id', 'timestamp', 'conversion_name', 'action_name', 'visits', 'goals',
    'external_metrics_connection_type_id', 'source_date'))
  for filename in args.filename:
    with open(filename, newline='') as stream:
      rows = csv.reader(stream)
      header = next(rows)
      legacy = header[6] == 'ExternalMetricsConnectionID'
      if header[6] not in ('ExternalMetricsConnectionID', 'ExternalMetricsConnectionTypeID'):
        raise ValueError('Unknown RPostConvClick source column')
      for row in rows:
        if row:
          if legacy:
            row[6] = '1' if int(row[6]) else '0'
          writer.writerow(row)
