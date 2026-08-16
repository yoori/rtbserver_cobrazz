#!/usr/bin/env python3

import contextlib
import csv
import io
import json
import os
import sys
import tempfile
import unittest


GEO_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, GEO_DIR)

import GeoMapper


class GeoMapperTest(unittest.TestCase):
  def setUp(self):
    self.temporary_directory = tempfile.TemporaryDirectory()
    self.root = self.temporary_directory.name
    self.points = os.path.join(self.root, 'points.csv')
    self.locations = os.path.join(self.root, 'locations.csv')
    self.geo = os.path.join(self.root, 'geo.csv')

    self.write_csv(self.points, (
      ('channel_id', 'region', 'city', 'latitude', 'longitude', 'status'),
      (1, 'alpha', 'alpha city', 55.0, 37.0, 'A'),
      (1, 'alpha', 'alpha alias', 55.0, 37.0, 'A'),
      (2, 'beta', 'beta city', 60.0, 30.0, 'A'),
      (3, 'alpha', 'neighbor', 55.01, 37.01, 'A'),
      (4, 'alpha', 'deleted', 55.0, 37.0, 'D'),
    ))
    self.write_csv(self.locations, (
      ('ru/alpha/',),
      ('ru/alpha/alpha city',),
      ('ru/alpha/alpha alias',),
      ('ru/alpha/neighbor',),
      ('ru/beta/',),
      ('ru/beta/beta city',),
    ))

    geo_rows = [[
      'IP', 'Source', 'Latitude', 'Longitude', 'Type', 'Country',
      'Region', 'City']]
    for index in range(25):
      geo_rows.append([
        '10.0.0.{0}'.format(index), 'ssp', 55.0, 37.0, 2, 'RUS',
        'raw-alpha', 'Alpha City'])
      geo_rows.append([
        '10.1.0.{0}'.format(index), 'ssp', 55.0, 37.0, 2, 'RU',
        'raw-split', 'Alpha City'])
      geo_rows.append([
        '10.2.0.{0}'.format(index), 'ssp', 60.0, 30.0, 2, 'RUS',
        'raw-split', 'Beta City'])
      geo_rows.append([
        '10.3.0.{0}'.format(index), 'ssp', 0.0, 0.0, 2, 'RUS',
        'raw-fallback', 'Alpha Alias'])
    geo_rows.append([
      '', 'ssp', 60.0, 30.0, 2, 'RUS', 'raw-alpha', 'Beta City'])
    geo_rows.append([
      '<nil>', 'ssp', 60.0, 30.0, 2, 'RUS', 'raw-alpha', 'Beta City'])
    geo_rows.append([
      '10.4.0.1', 'ssp', 60.0, 30.0, 2, 'RUS',
      'raw-alpha', 'Alpha City'])
    self.write_csv(self.geo, geo_rows)

  def tearDown(self):
    self.temporary_directory.cleanup()

  @staticmethod
  def write_csv(path, rows):
    with open(path, 'w', encoding='utf-8', newline='') as target:
      csv.writer(target).writerows(rows)

  @staticmethod
  def read_dicts(path):
    with open(path, 'r', encoding='utf-8', newline='') as source:
      return list(csv.DictReader(source))

  def run_mapper(self, arguments):
    with contextlib.redirect_stdout(io.StringIO()):
      self.assertEqual(GeoMapper.main(arguments), 0)

  def test_region_split_and_reusable_override(self):
    first_output = os.path.join(self.root, 'first')
    self.run_mapper([
      '--geo', self.geo,
      '--canonical-points', self.points,
      '--locations', self.locations,
      '--output-dir', first_output,
      '--min-region-support', '1',
      '--min-city-support', '5',
      '--disable-fuzzy',
      '--progress-rows', '0',
    ])

    mappings = self.read_dicts(
      os.path.join(first_output, 'region_mapping.csv'))
    self.assertIn(
      ('raw-alpha', 'alpha'),
      set((row['Region'], row['CanonicalRegion']) for row in mappings))
    self.assertIn(
      ('raw-fallback', 'alpha'),
      set((row['Region'], row['CanonicalRegion']) for row in mappings))
    split_targets = set(
      row['CanonicalRegion'] for row in mappings
      if row['Region'] == 'raw-split')
    self.assertEqual(split_targets, {'alpha', 'beta'})

    conflicts = self.read_dicts(
      os.path.join(first_output, 'region_conflicts.csv'))
    split_targets = set(
      row['CanonicalRegion'] for row in conflicts
      if row['Region'] == 'raw-split')
    self.assertEqual(split_targets, {'alpha', 'beta'})

    city_mappings = self.read_dicts(
      os.path.join(first_output, 'city_mapping.csv'))
    split_locations = set(
      row['TargetLocation'] for row in city_mappings
      if row['Region'] == 'raw-split')
    self.assertEqual(
      split_locations, {'ru/alpha/alpha city', 'ru/beta/beta city'})

    with open(
        os.path.join(first_output, 'summary.json'), encoding='utf-8') as source:
      summary = json.load(source)
    self.assertEqual(summary['empty_ip_rows'], 2)
    self.assertEqual(summary['canonical_channels'], 3)

    overrides = os.path.join(self.root, 'overrides.csv')
    self.write_csv(overrides, (
      ('Source', 'Country', 'Region', 'CanonicalRegion'),
      ('ssp', 'ru', 'raw-split', 'alpha'),
      ('ssp', 'ru', 'raw-split', 'beta'),
    ))
    second_output = os.path.join(self.root, 'second')
    self.run_mapper([
      '--load-evidence', os.path.join(first_output, 'evidence.sqlite3'),
      '--canonical-points', self.points,
      '--locations', self.locations,
      '--output-dir', second_output,
      '--region-overrides', overrides,
      '--min-region-support', '1',
      '--min-city-support', '5',
      '--disable-fuzzy',
    ])

    mappings = self.read_dicts(
      os.path.join(second_output, 'region_mapping.csv'))
    split_targets = set(
      row['CanonicalRegion'] for row in mappings
      if row['Region'] == 'raw-split')
    self.assertEqual(split_targets, {'alpha', 'beta'})
    city_mappings = self.read_dicts(
      os.path.join(second_output, 'city_mapping.csv'))
    split_locations = set(
      row['TargetLocation'] for row in city_mappings
      if row['Region'] == 'raw-split')
    self.assertEqual(
      split_locations, {'ru/alpha/alpha city', 'ru/beta/beta city'})

    alpha_mapping = next(
      row for row in city_mappings
      if row['Region'] == 'raw-alpha' and row['City'] == 'alpha city')
    self.assertEqual(alpha_mapping['TargetLocation'], 'ru/alpha/alpha city')

  def test_split_with_more_than_two_regions_requires_override(self):
    evidence = GeoMapper.Evidence()
    key = ('ssp', 'ru', 'raw-region')
    evidence.region_totals[key] = 75
    for region in ('alpha', 'beta', 'gamma'):
      vote = GeoMapper.Vote()
      vote.add(rows=25, distance=1.0, method='coordinate')
      evidence.region_votes[key][region] = vote

    resolution = GeoMapper.resolve_regions(
      evidence, {}, minimum_support=20, minimum_confidence=0.95)[key]
    self.assertEqual(resolution.status, 'split')
    self.assertEqual(resolution.regions, frozenset())

  def test_target_location_tie_break_is_deterministic(self):
    canonical = GeoMapper.CanonicalData()
    canonical.locations = {'ru/alpha/ab', 'ru/alpha/ac'}
    point = GeoMapper.CanonicalPoint(1, 'alpha', 55.0, 37.0)
    point.aliases = {'ac', 'ab'}

    self.assertEqual(canonical.target_location(point, 'ad'), 'ru/alpha/ab')

  def test_exact_candidates_use_text_then_shared_target(self):
    canonical = GeoMapper.CanonicalData()
    canonical.locations = {
      'ru/alpha/kinel', "ru/alpha/kinel'", 'ru/alpha/usinsk'}

    kinel = GeoMapper.CanonicalPoint(1, 'alpha', 55.0, 37.0)
    kinel.aliases = {'kinel'}
    kinel_quote = GeoMapper.CanonicalPoint(2, 'alpha', 55.0, 37.0)
    kinel_quote.aliases = {"kinel'"}
    usinsk_alias = GeoMapper.CanonicalPoint(3, 'alpha', 55.0, 37.0)
    usinsk_alias.aliases = {'parma', 'usinsk'}
    usinsk = GeoMapper.CanonicalPoint(4, 'alpha', 55.0, 37.0)
    usinsk.aliases = {'usinsk'}
    canonical.points = {
      point.channel_id: point
      for point in (kinel, kinel_quote, usinsk_alias, usinsk)}

    self.assertEqual(
      GeoMapper.select_exact_candidate(canonical, {1, 2}, "kinel'", None),
      (2, 'exact_text'))
    self.assertEqual(
      GeoMapper.select_exact_candidate(canonical, {3, 4}, 'usinsk', None),
      (3, 'exact_target'))


if __name__ == '__main__':
  unittest.main()
