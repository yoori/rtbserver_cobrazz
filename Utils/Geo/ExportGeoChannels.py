#!/usr/bin/env python3

"""Export non-deleted RU CITY channels for GeoMapper."""

import argparse
import csv
import io
import os
import subprocess
import sys

from GeoMapper import normalize_name


class Channel(object):
  def __init__(self, row):
    self.channel_id = int(row['channel_id'])
    self.parent_name = row['parent_name']
    self.name = row['name']
    self.city_list = row['city_list']
    self.latitude = float(row['latitude'])
    self.longitude = float(row['longitude'])
    self.status = row['status']
    self.aliases = channel_aliases(self.name, self.city_list)


def channel_aliases(name, city_list):
  aliases = {}
  for value in [name] + (city_list or '').splitlines():
    value = value.strip()
    if '/' in value:
      value = value.rsplit('/', 1)[1].strip()
    normalized = normalize_name(value)
    if normalized and normalized not in aliases:
      aliases[normalized] = value
  return aliases


def load_channels(host, port, database, user):
  query = """
COPY (
  SELECT
    city.channel_id,
    parent.name AS parent_name,
    city.name,
    COALESCE(city.city_list, '') AS city_list,
    city.latitude,
    city.longitude,
    city.status
  FROM channel city
  JOIN channel parent ON parent.channel_id = city.parent_channel_id
  WHERE city.country_code = 'RU'
    AND city.channel_type = 'G'
    AND city.geo_type = 'CITY'
    AND city.status <> 'D'
    AND parent.status <> 'D'
    AND city.latitude IS NOT NULL
    AND city.longitude IS NOT NULL
  ORDER BY city.channel_id
) TO STDOUT WITH (FORMAT CSV, HEADER TRUE)
"""
  command = [
    'psql', '-X', '--no-psqlrc', '-v', 'ON_ERROR_STOP=1',
    '-h', host, '-p', str(port), '-U', user, '-d', database, '-c', query]
  process = subprocess.run(
    command,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    universal_newlines=True)
  if process.returncode:
    raise RuntimeError(process.stderr.strip() or 'psql failed')
  return [Channel(row) for row in csv.DictReader(io.StringIO(process.stdout))]


def write_channels(path, channels):
  parent = os.path.dirname(os.path.abspath(path))
  os.makedirs(parent, exist_ok=True)
  temporary_path = path + '.tmp'
  rows = 0
  with open(temporary_path, 'w', encoding='utf-8', newline='') as target:
    writer = csv.writer(target, lineterminator='\n')
    writer.writerow((
      'channel_id', 'region', 'city', 'latitude', 'longitude', 'status'))
    for channel in channels:
      for _, city in sorted(channel.aliases.items()):
        writer.writerow((
          channel.channel_id,
          channel.parent_name,
          city,
          '{0:.6f}'.format(channel.latitude),
          '{0:.6f}'.format(channel.longitude),
          channel.status))
        rows += 1
  os.replace(temporary_path, path)
  return rows


def parse_args(argv):
  parser = argparse.ArgumentParser(
    description='Export non-deleted RU CITY channels for GeoMapper.')
  parser.add_argument('--host', default=os.environ.get('PGHOST', 'localhost'))
  parser.add_argument('--port', type=int, default=int(os.environ.get(
    'PGPORT', '5432')))
  parser.add_argument('--database', default=os.environ.get(
    'PGDATABASE', 'stat'))
  parser.add_argument('--user', default=os.environ.get('PGUSER', 'ro'))
  parser.add_argument('--output', required=True)
  return parser.parse_args(argv)


def main(argv=None):
  args = parse_args(argv)
  channels = load_channels(
    args.host, args.port, args.database, args.user)
  rows = write_channels(args.output, channels)
  print('channels: {0}'.format(len(channels)))
  print('rows: {0}'.format(rows))
  print('output: {0}'.format(os.path.abspath(args.output)))
  return 0


if __name__ == '__main__':
  try:
    sys.exit(main())
  except (OSError, RuntimeError, ValueError) as error:
    print('ExportGeoChannels: {0}'.format(error), file=sys.stderr)
    sys.exit(1)
