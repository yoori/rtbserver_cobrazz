import argparse
import csv
import json
import os
import sys

from .Boundaries import (
  BoundaryIndex,
  BoundaryResolver,
  load_bgp_prefixes,
  load_old_database_boundaries,
  load_rir_database)
from .Merger import (
  build_output,
  collect_unique_votes,
  load_mapping,
  merge_subnets,
  scan_candidate_parents,
  select_subnets)


def atomic_json(path, value):
  os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
  temporary_path = path + '.tmp'
  with open(temporary_path, 'w', encoding='utf-8') as target:
    json.dump(value, target, indent=2, sort_keys=True)
    target.write('\n')
  os.replace(temporary_path, path)


def parse_args(argv):
  parser = argparse.ArgumentParser(
    description=(
      'Overlay mapped Geo observations using conservative IPv4 prefix '
      'aggregation. The Geo CSV must be grouped by IP.'))
  parser.add_argument('--geo', required=True)
  parser.add_argument('--mapping', required=True)
  parser.add_argument('--existing', required=True)
  parser.add_argument('--output', required=True)
  parser.add_argument('--rir-database', action='append', default=[])
  parser.add_argument('--bgp-prefixes', action='append', default=[])
  parser.add_argument('--conflicts')
  parser.add_argument('--summary')
  parser.add_argument('--base-prefix', type=int, default=25)
  parser.add_argument('--max-merged-prefix', type=int, default=23)
  parser.add_argument('--min-unique-ips', type=int, default=2)
  parser.add_argument('--min-confidence', type=float, default=0.75)
  parser.add_argument('--progress-rows', type=int, default=1000000)
  args = parser.parse_args(argv)
  if args.progress_rows < 0:
    parser.error('--progress-rows must be non-negative')
  if args.base_prefix < 1 or args.base_prefix > 32:
    parser.error('--base-prefix must be between 1 and 32')
  if (args.max_merged_prefix < 23 or
      args.max_merged_prefix > args.base_prefix):
    parser.error(
      '--max-merged-prefix must be between 23 and --base-prefix')
  if args.min_unique_ips < 1:
    parser.error('--min-unique-ips must be positive')
  if args.min_confidence <= 0.5 or args.min_confidence > 1.0:
    parser.error('--min-confidence must be in (0.5, 1.0]')
  return args


def main(argv=None):
  args = parse_args(argv)
  mapping = load_mapping(args.mapping)
  candidate_parents, scan_stats = scan_candidate_parents(
    args.geo, mapping, args.progress_rows)

  boundary_stats = {}
  indexes = []

  old_index = BoundaryIndex('old database', candidate_parents)
  boundary_stats.update(load_old_database_boundaries(
    args.existing, old_index))
  indexes.append(old_index)

  if args.rir_database:
    rir_index = BoundaryIndex(
      'RIR database', candidate_parents, cap_prefix=args.max_merged_prefix)
    for path in args.rir_database:
      for key, value in load_rir_database(path, rir_index).items():
        boundary_stats[key] = boundary_stats.get(key, 0) + value
    rir_index.finalize()
    boundary_stats['rir_indexed_boundaries'] = rir_index.objects
    indexes.append(rir_index)

  if args.bgp_prefixes:
    bgp_index = BoundaryIndex('BGP prefixes', candidate_parents)
    for path in args.bgp_prefixes:
      for key, value in load_bgp_prefixes(path, bgp_index).items():
        boundary_stats[key] = boundary_stats.get(key, 0) + value
    bgp_index.finalize()
    boundary_stats['bgp_indexed_boundaries'] = bgp_index.objects
    indexes.append(bgp_index)

  resolver = BoundaryResolver(indexes, args.base_prefix)
  votes, cell_unique_ips, vote_stats = collect_unique_votes(
    args.geo, mapping, resolver, args.progress_rows)
  selected, selection_stats = select_subnets(
    votes,
    cell_unique_ips,
    args.min_unique_ips,
    args.min_confidence,
    args.conflicts)
  del votes
  del cell_unique_ips

  selected, merge_stats = merge_subnets(
    selected, args.max_merged_prefix)
  output_stats = build_output(args.existing, args.output, selected)

  summary = dict(scan_stats)
  summary.update(boundary_stats)
  summary.update(vote_stats)
  summary.update(selection_stats)
  summary.update(merge_stats)
  summary.update(output_stats)
  summary.update({
    'base_prefix': args.base_prefix,
    'max_merged_prefix': args.max_merged_prefix,
    'min_confidence': args.min_confidence,
    'min_unique_ips': args.min_unique_ips,
    'mapping_keys': len(mapping),
    'output': os.path.abspath(args.output),
  })
  if args.summary:
    atomic_json(args.summary, summary)
  print(json.dumps(summary, indent=2, sort_keys=True))
  return 0


def run():
  try:
    return main()
  except (OSError, ValueError, csv.Error) as error:
    print('GeoMerger: {0}'.format(error), file=sys.stderr)
    return 1
