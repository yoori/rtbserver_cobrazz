#!/usr/bin/env python3

import contextlib
import csv
import io
import ipaddress
import os
import sys
import tempfile
import unittest


GEO_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, GEO_DIR)

from GeoMerger import Application


class GeoMergerTest(unittest.TestCase):
  def setUp(self):
    self.temporary_directory = tempfile.TemporaryDirectory()
    self.root = self.temporary_directory.name
    self.mapping = os.path.join(self.root, 'mapping.csv')
    self.write_csv(self.mapping, (
      ('Source', 'Country', 'Region', 'City', 'TargetLocation'),
      ('otm', 'ru', 'raw', 'alpha', 'ru/region/alpha'),
      ('yandex', 'ru', 'raw', 'beta', 'ru/region/beta'),
      ('yandex', 'ru', 'raw', 'beta-alias', 'ru/region/beta'),
      ('yandex', 'ru', 'raw', 'other', 'ru/region/other'),
    ))

  def tearDown(self):
    self.temporary_directory.cleanup()

  @staticmethod
  def write_csv(path, rows):
    with open(path, 'w', encoding='utf-8', newline='') as target:
      csv.writer(target).writerows(rows)

  @staticmethod
  def read_csv(path):
    with open(path, 'r', encoding='utf-8', newline='') as source:
      return list(csv.reader(source))

  def run_merger(self, geo_rows, existing_rows, extra_arguments=()):
    geo = os.path.join(self.root, 'geo.csv')
    existing = os.path.join(self.root, 'existing.csv')
    output = os.path.join(self.root, 'output.csv')
    conflicts = os.path.join(self.root, 'conflicts.csv')
    self.write_csv(geo, (
      ('IP', 'Source', 'Country', 'Region', 'City'),) + tuple(geo_rows))
    self.write_csv(existing, existing_rows)
    arguments = [
      '--geo', geo,
      '--mapping', self.mapping,
      '--existing', existing,
      '--output', output,
      '--conflicts', conflicts,
      '--progress-rows', '0',
    ]
    arguments.extend(extra_arguments)
    with contextlib.redirect_stdout(io.StringIO()):
      self.assertEqual(Application.main(arguments), 0)
    return self.read_csv(output), self.read_csv(conflicts)

  def test_old_specific_boundaries_are_not_merged(self):
    rows, _ = self.run_merger((
      ('10.0.0.1', 'yandex', 'RUS', 'raw', 'beta'),
      ('10.0.0.2', 'yandex', 'RUS', 'raw', 'beta'),
      ('10.0.0.129', 'yandex', 'RUS', 'raw', 'beta'),
      ('10.0.0.130', 'yandex', 'RUS', 'raw', 'beta'),
    ), (
      ('10.0.0.0/25', 'ru/medium/left'),
      ('10.0.0.128/25', 'ru/medium/right'),
    ))

    self.assertIn(['10.0.0.0/25', 'ru/region/beta'], rows)
    self.assertIn(['10.0.0.128/25', 'ru/region/beta'], rows)
    self.assertNotIn(['10.0.0.0/24', 'ru/region/beta'], rows)
    self.assertNotIn(['10.0.0.0/25', 'ru/medium/left'], rows)

  def test_independently_supported_children_merge_to_23(self):
    geo_rows = []
    for last_octet in (1, 2, 129, 130):
      geo_rows.append((
        '10.0.2.{0}'.format(last_octet),
        'yandex', 'RUS', 'raw', 'beta'))
    for last_octet in (1, 2, 129, 130):
      geo_rows.append((
        '10.0.3.{0}'.format(last_octet),
        'yandex', 'RUS', 'raw', 'beta'))

    rows, _ = self.run_merger(geo_rows, (
      ('10.0.9.0/24', 'ru/medium/unchanged'),
    ))

    self.assertIn(['10.0.2.0/23', 'ru/region/beta'], rows)
    self.assertIn(['10.0.9.0/24', 'ru/medium/unchanged'], rows)

  def test_rir_boundaries_prevent_23_merge(self):
    rir = os.path.join(self.root, 'ripe.db.inetnum')
    with open(rir, 'w', encoding='utf-8') as target:
      target.write(
        'inetnum: 10.0.4.0 - 10.0.4.255\n\n'
        'inetnum: 10.0.5.0 - 10.0.5.255\n\n')

    geo_rows = []
    for third_octet in (4, 5):
      for last_octet in (1, 2, 129, 130):
        geo_rows.append((
          '10.0.{0}.{1}'.format(third_octet, last_octet),
          'yandex', 'RUS', 'raw', 'beta'))
    rows, _ = self.run_merger(
      geo_rows,
      (),
      ('--rir-database', rir))

    self.assertIn(['10.0.4.0/24', 'ru/region/beta'], rows)
    self.assertIn(['10.0.5.0/24', 'ru/region/beta'], rows)
    self.assertNotIn(['10.0.4.0/23', 'ru/region/beta'], rows)

  def test_nested_rir_boundary_splits_parent_cell(self):
    rir = os.path.join(self.root, 'ripe-nested.db.inetnum')
    with open(rir, 'w', encoding='utf-8') as target:
      target.write(
        'inetnum: 10.0.11.0 - 10.0.11.255\n\n'
        'inetnum: 10.0.11.0 - 10.0.11.63\n\n')

    rows, _ = self.run_merger((
      ('10.0.11.1', 'yandex', 'RUS', 'raw', 'other'),
      ('10.0.11.2', 'yandex', 'RUS', 'raw', 'other'),
      ('10.0.11.65', 'yandex', 'RUS', 'raw', 'beta'),
      ('10.0.11.66', 'yandex', 'RUS', 'raw', 'beta'),
    ), (), ('--rir-database', rir))

    self.assertIn(['10.0.11.0/26', 'ru/region/other'], rows)
    self.assertIn(['10.0.11.64/26', 'ru/region/beta'], rows)
    self.assertNotIn(['10.0.11.0/25', 'ru/region/beta'], rows)

  def test_boundary_parts_require_independent_support(self):
    rir = os.path.join(self.root, 'ripe-middle.db.inetnum')
    with open(rir, 'w', encoding='utf-8') as target:
      target.write(
        'inetnum: 10.0.12.0 - 10.0.12.255\n\n'
        'inetnum: 10.0.12.64 - 10.0.12.127\n\n')

    rows, conflicts = self.run_merger((
      ('10.0.12.1', 'yandex', 'RUS', 'raw', 'beta'),
      ('10.0.12.65', 'yandex', 'RUS', 'raw', 'other'),
      ('10.0.12.66', 'yandex', 'RUS', 'raw', 'other'),
      ('10.0.12.129', 'yandex', 'RUS', 'raw', 'beta'),
    ), (), ('--rir-database', rir))

    self.assertEqual(rows, [['10.0.12.64/26', 'ru/region/other']])
    rejected = {
      row[1] for row in conflicts[1:]
      if row[-1] == 'insufficient_unique_ips'}
    self.assertEqual(rejected, {'10.0.12.0/26', '10.0.12.128/25'})

  def test_bgp_origins_prevent_23_merge(self):
    bgp = os.path.join(self.root, 'bgp.csv')
    self.write_csv(bgp, (
      ('Prefix', 'OriginASN'),
      ('10.0.6.0/24', 'AS1'),
      ('10.0.7.0/24', 'AS2'),
    ))
    geo_rows = []
    for third_octet in (6, 7):
      for last_octet in (1, 2, 129, 130):
        geo_rows.append((
          '10.0.{0}.{1}'.format(third_octet, last_octet),
          'yandex', 'RUS', 'raw', 'beta'))
    rows, _ = self.run_merger(
      geo_rows,
      (),
      ('--bgp-prefixes', bgp))

    self.assertIn(['10.0.6.0/24', 'ru/region/beta'], rows)
    self.assertIn(['10.0.7.0/24', 'ru/region/beta'], rows)
    self.assertNotIn(['10.0.6.0/23', 'ru/region/beta'], rows)

  def test_repeated_ip_is_one_vote_and_low_does_not_override_medium(self):
    rows, conflicts = self.run_merger((
      ('10.0.8.1', 'yandex', 'RUS', 'raw', 'beta'),
      ('10.0.8.1', 'yandex', 'RUS', 'raw', 'beta-alias'),
      ('10.0.9.1', 'otm', 'RUS', 'raw', 'alpha'),
      ('10.0.9.2', 'otm', 'RUS', 'raw', 'alpha'),
      ('2001:db8::1', 'yandex', 'RUS', 'raw', 'beta'),
    ), (
      ('10.0.8.0/24', 'ru/medium/eight'),
      ('10.0.9.0/24', 'ru/medium/nine'),
    ))

    self.assertIn(['10.0.8.0/24', 'ru/medium/eight'], rows)
    self.assertNotIn(['10.0.8.0/25', 'ru/region/beta'], rows)
    self.assertIn(['10.0.9.0/24', 'ru/medium/nine'], rows)
    self.assertNotIn(['10.0.9.0/25', 'ru/region/alpha'], rows)
    self.assertTrue(any(
      row[-1] == 'insufficient_unique_ips' for row in conflicts[1:]))

    parsed = [ipaddress.ip_network(row[0]) for row in rows]
    self.assertTrue(all(network.version == 4 for network in parsed))

  def test_single_ip_can_replace_existing_32_without_expansion(self):
    rows, _ = self.run_merger((
      ('10.0.10.7', 'yandex', 'RUS', 'raw', 'beta'),
    ), (
      ('10.0.10.7/32', 'ru/medium/old'),
    ))

    self.assertEqual(rows, [['10.0.10.7/32', 'ru/region/beta']])


if __name__ == '__main__':
  unittest.main()
