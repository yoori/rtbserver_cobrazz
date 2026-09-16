#!/usr/bin/env python3

"""Generate a topology-size-independent CPU-affinity map.

Input pools are expanded into workers and assigned unique abstract CPU positions per NUMA node.
At runtime position modulo the available CPU count selects the CPU. The objective is the sum of
squared coefficients of variation for every meaningful CPU count, so no hardware size is baked
into the generated map.
"""

import argparse
import collections
import csv
import math
import os
import sys


Pool = collections.namedtuple("Pool", "numa_node name threads points")
MAX_REFINEMENT_SWAPS = 32


class InputError(Exception):
  pass


def read_pools(stream):
  lines = [line for line in stream if line.strip() and not line.lstrip().startswith("#")]
  if not lines:
    raise InputError("input is empty")

  reader = csv.DictReader(lines, delimiter="\t")
  required = {"numa_node", "pool", "threads", "points"}
  if reader.fieldnames is None or set(reader.fieldnames) != required:
    fields = ", ".join(sorted(required))
    raise InputError(f"expected a tab-separated header with fields: {fields}")

  pools = []
  seen = set()
  for line_number, row in enumerate(reader, 2):
    try:
      numa_node = int(row["numa_node"])
      name = row["pool"].strip()
      threads = int(row["threads"])
      points = float(row["points"])
    except (TypeError, ValueError):
      raise InputError(f"line {line_number}: invalid numeric value")

    if numa_node < 0:
      raise InputError(f"line {line_number}: numa_node must be non-negative")
    if not name:
      raise InputError(f"line {line_number}: pool must not be empty")
    if threads <= 0:
      raise InputError(f"line {line_number}: threads must be positive")
    if not math.isfinite(points) or points < 0:
      raise InputError(f"line {line_number}: points must be finite and non-negative")

    key = (numa_node, name)
    if key in seen:
      raise InputError(f"line {line_number}: duplicate pool {name!r} on NUMA {numa_node}")
    seen.add(key)
    pools.append(Pool(numa_node, name, threads, points))

  if not pools:
    raise InputError("input contains no pools")
  return sorted(pools, key=lambda pool: (pool.numa_node, pool.name))


def collision_penalties(size):
  penalties = [0.0] * size
  # Positions collide after modulo C exactly when C divides the distance between them.
  for cpu_count in range(2, size):
    for distance in range(cpu_count, size, cpu_count):
      penalties[distance] += cpu_count
  return penalties


def worker_collision_score(workers, penalties):
  score = 0.0
  for left in range(len(workers)):
    for right in range(left + 1, len(workers)):
      score += workers[left][0] * workers[right][0] * penalties[right - left]
  return score


def greedy_worker_order(workers, penalties):
  size = len(workers)
  collision_cost = [0.0] * size
  available = [True] * size
  result = [None] * size

  for worker in workers:
    points = worker[0]
    position = min(
      (candidate for candidate in range(size) if available[candidate]),
      key=lambda candidate: (collision_cost[candidate], candidate))
    available[position] = False
    result[position] = worker

    if points == 0:
      continue
    for candidate in range(size):
      if available[candidate]:
        collision_cost[candidate] += points * penalties[abs(candidate - position)]
  return result


def refine_worker_order(workers, penalties):
  size = len(workers)
  fields = [
    sum(workers[other][0] * penalties[abs(position - other)] for other in range(size))
    for position in range(size)
  ]

  for _ in range(min(MAX_REFINEMENT_SWAPS, size)):
    best_delta = 0.0
    best_swap = None
    for left in range(size):
      left_points = workers[left][0]
      for right in range(left + 1, size):
        right_points = workers[right][0]
        if left_points == right_points:
          continue
        delta = (right_points - left_points) * (
          fields[left] - fields[right] +
          (left_points - right_points) * penalties[right - left])
        if delta < best_delta:
          best_delta = delta
          best_swap = (left, right)

    if best_swap is None:
      break

    left, right = best_swap
    left_points = workers[left][0]
    right_points = workers[right][0]
    workers[left], workers[right] = workers[right], workers[left]
    for position in range(size):
      fields[position] += (
        (right_points - left_points) * penalties[abs(position - left)] +
        (left_points - right_points) * penalties[abs(position - right)])
  return workers


def arrange_node(pools):
  pools = sorted(pools, key=lambda pool: pool.name)
  workers = []
  for pool in pools:
    workers.extend((pool.points, pool.name, index) for index in range(pool.threads))
  workers.sort(key=lambda worker: (-worker[0], worker[1], worker[2]))

  size = len(workers)
  penalties = collision_penalties(size)
  greedy = greedy_worker_order(workers, penalties)
  sequential = sorted(workers, key=lambda worker: (worker[1], worker[2]))
  if worker_collision_score(sequential, penalties) < worker_collision_score(greedy, penalties):
    selected = sequential
  else:
    selected = greedy
  selected = refine_worker_order(selected, penalties)

  result = {pool.name: [None] * pool.threads for pool in pools}
  for position, worker in enumerate(selected):
    _points, name, index = worker
    result[name][index] = position
  return result


def generate_layout(pools):
  nodes = {}
  for pool in pools:
    nodes.setdefault(pool.numa_node, []).append(pool)

  layout = {}
  for numa_node, node_pools in sorted(nodes.items()):
    node_layout = arrange_node(node_pools)
    for pool in node_pools:
      layout[(numa_node, pool.name)] = node_layout[pool.name]
  return layout


def position_weights(pools, layout):
  nodes = {}
  for pool in pools:
    node = nodes.setdefault(pool.numa_node, [0.0] * sum(
      item.threads for item in pools if item.numa_node == pool.numa_node))
    for position in layout[(pool.numa_node, pool.name)]:
      if position < 0 or position >= len(node):
        raise ValueError(f"position {position} is outside NUMA {pool.numa_node} layout")
      node[position] = pool.points
  return nodes


def quality_rows(pools, layout):
  rows = []
  for numa_node, weights in sorted(position_weights(pools, layout).items()):
    total = sum(weights)
    for cpu_count in range(1, len(weights) + 1):
      loads = [0.0] * cpu_count
      counts = [0] * cpu_count
      for position, weight in enumerate(weights):
        cpu = position % cpu_count
        loads[cpu] += weight
        counts[cpu] += 1

      mean_load = total / cpu_count
      if mean_load:
        max_overload = 100.0 * (max(loads) / mean_load - 1.0)
        variance = sum((load - mean_load) ** 2 for load in loads) / cpu_count
        coefficient_of_variation = 100.0 * math.sqrt(variance) / mean_load
      else:
        max_overload = 0.0
        coefficient_of_variation = 0.0
      rows.append({
        "numa_node": numa_node,
        "cpu_count": cpu_count,
        "threads_min": min(counts),
        "threads_max": max(counts),
        "mean_load": mean_load,
        "max_load": max(loads),
        "max_overload_pct": max_overload,
        "coefficient_of_variation_pct": coefficient_of_variation,
      })
  return rows


def collision_score(pools, layout):
  score = 0.0
  for weights in position_weights(pools, layout).values():
    penalties = collision_penalties(len(weights))
    for left in range(len(weights)):
      for right in range(left + 1, len(weights)):
        score += weights[left] * weights[right] * penalties[right - left]
  return score


def split_pool_name(name):
  parts = name.split("/", 1)
  if len(parts) != 2 or not parts[0] or not parts[1]:
    raise InputError(f"pool {name!r} must have Service/pool form")
  return parts


def sequential_layout(pools):
  layout = {}
  next_position = {}
  for pool in pools:
    begin = next_position.get(pool.numa_node, 0)
    layout[(pool.numa_node, pool.name)] = list(range(begin, begin + pool.threads))
    next_position[pool.numa_node] = begin + pool.threads
  return layout


def write_layout(stream, pools, layout, service=None):
  stream.write("# pool\tnuma_node\tabstract_cpu_by_thread_index\n")
  for pool in pools:
    pool_service, pool_name = split_pool_name(pool.name)
    if service is not None and pool_service != service:
      continue
    positions = ",".join(str(position) for position in layout[(pool.numa_node, pool.name)])
    stream.write(f"{pool_name}\t{pool.numa_node}\t{positions}\n")


def write_service_layouts(output_dir, pools, layout):
  services = sorted({split_pool_name(pool.name)[0] for pool in pools})
  os.makedirs(output_dir, exist_ok=True)
  for service in services:
    path = os.path.join(output_dir, f"{service}.cpu_aff")
    with open(path, "w", encoding="utf-8") as stream:
      write_layout(stream, pools, layout, service)


def write_quality_report(path, rows):
  fields = (
    "numa_node",
    "cpu_count",
    "threads_min",
    "threads_max",
    "mean_load",
    "max_load",
    "max_overload_pct",
    "coefficient_of_variation_pct",
  )
  with open(path, "w", newline="", encoding="utf-8") as stream:
    writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
    writer.writeheader()
    writer.writerows(rows)


def parse_args():
  parser = argparse.ArgumentParser(
    description="Generate a CPU-count-independent thread-pool affinity layout")
  parser.add_argument("input", help="input TSV path, or - for stdin")
  parser.add_argument(
    "--quality-report",
    help="write per-NUMA quality metrics for every possible CPU count to this TSV")
  parser.add_argument(
    "--output-dir",
    help="write one <service>.cpu_aff file per service instead of a combined stdout map")
  return parser.parse_args()


def main():
  args = parse_args()
  try:
    if args.input == "-":
      pools = read_pools(sys.stdin)
    else:
      with open(args.input, "r", encoding="utf-8") as stream:
        pools = read_pools(stream)
    layout = generate_layout(pools)
    if args.output_dir:
      write_service_layouts(args.output_dir, pools, layout)
    else:
      write_layout(sys.stdout, pools, layout)
    if args.quality_report:
      write_quality_report(args.quality_report, quality_rows(pools, layout))
  except (InputError, OSError) as error:
    print(f"GenerateCpuAffinity.py: {error}", file=sys.stderr)
    return 1
  return 0


if __name__ == "__main__":
  sys.exit(main())
