#!/usr/bin/env python3

import csv
import datetime
import glob
import math
import os


ROOT = os.path.dirname(os.path.abspath(__file__))

POOL_ALIASES = {
  "event_engin": "event_engine",
  "fast-schedu": "fast-scheduler",
  "fast-schedul": "fast-scheduler",
  "grpc_global_": "grpc_global",
  "jemalloc_bg_": "jemalloc_bg",
}


def percentile(values, fraction):
  if not values:
    return 0.0
  ordered = sorted(values)
  position = (len(ordered) - 1) * fraction
  lower = math.floor(position)
  upper = math.ceil(position)
  if lower == upper:
    return ordered[lower]
  return ordered[lower] * (upper - position) + ordered[upper] * (position - lower)


def mean(values):
  return sum(values) / len(values) if values else 0.0


def load_rows():
  rows = []
  for path in sorted(glob.glob(os.path.join(ROOT, "raw_adfe*.tsv"))):
    host = os.path.basename(path)[4:-4]
    with open(path, newline="", encoding="utf-8") as stream:
      for row in csv.DictReader(stream, delimiter="\t"):
        row["host"] = host
        row["pool"] = POOL_ALIASES.get(row["pool"], row["pool"])
        for field in (
          "sample", "wall_ns", "pid", "tid", "runtime_ns", "wait_ns", "timeslices"):
          row[field] = int(row[field])
        rows.append(row)
  return rows


def calculate(rows):
  previous = {}
  thread_totals = {}
  active = {}
  started_ns = None
  finished_ns = None

  for row in rows:
    key = (row["host"], row["pid"], row["tid"])
    group = (row["host"], row["service"], row["pool"])
    sample_key = group + (row["sample"],)
    active.setdefault(sample_key, set()).add((row["pid"], row["tid"]))
    started_ns = row["wall_ns"] if started_ns is None else min(started_ns, row["wall_ns"])
    finished_ns = row["wall_ns"] if finished_ns is None else max(finished_ns, row["wall_ns"])

    old = previous.get(key)
    previous[key] = row
    if old is None:
      continue

    elapsed = row["wall_ns"] - old["wall_ns"]
    runtime = row["runtime_ns"] - old["runtime_ns"]
    wait = row["wait_ns"] - old["wait_ns"]
    if elapsed <= 0 or runtime < 0 or wait < 0:
      continue

    total = thread_totals.setdefault(key, {
      "host": row["host"],
      "service": row["service"],
      "pool": row["pool"],
      "pid": row["pid"],
      "tid": row["tid"],
      "thread_name": row["thread_name"],
      "affinity": row["affinity"],
      "current_cpu": row["current_cpu"],
      "runtime_ns": 0,
      "wait_ns": 0,
      "elapsed_ns": 0,
    })
    total["runtime_ns"] += runtime
    total["wait_ns"] += wait
    total["elapsed_ns"] += elapsed
    total["affinity"] = row["affinity"]
    total["current_cpu"] = row["current_cpu"]

  counts = {}
  for sample_key, tids in active.items():
    group = sample_key[:3]
    counts.setdefault(group, []).append(len(tids))

  duration = 0.0
  if started_ns is not None and finished_ns is not None:
    duration = (finished_ns - started_ns) / 1_000_000_000
  return list(thread_totals.values()), counts, duration, started_ns, finished_ns


def build_thread_rows(threads):
  rows = []
  for thread in sorted(
      threads,
      key=lambda row: (row["host"], row["service"], row["pool"], row["tid"])):
    elapsed = thread["elapsed_ns"]
    runtime = thread["runtime_ns"]
    wait = thread["wait_ns"]
    rows.append({
      "host": thread["host"],
      "service": thread["service"],
      "pool": thread["pool"],
      "pid": thread["pid"],
      "tid": thread["tid"],
      "thread_name": thread["thread_name"],
      "affinity": thread["affinity"],
      "current_cpu": thread["current_cpu"],
      "runtime_sec": runtime / 1_000_000_000,
      "wait_sec": wait / 1_000_000_000,
      "elapsed_sec": elapsed / 1_000_000_000,
      "cpu_pct": 100.0 * runtime / elapsed if elapsed else 0.0,
      "runqueue_wait_pct": 100.0 * wait / elapsed if elapsed else 0.0,
      "runnable_pct": 100.0 * min(1.0, (runtime + wait) / elapsed) if elapsed else 0.0,
    })
  return rows


def build_integrity(rows):
  hosts = {}
  for row in rows:
    host = hosts.setdefault(row["host"], {"rows": 0, "samples": {}})
    host["rows"] += 1
    sample = host["samples"].setdefault(row["sample"], {
      "wall_ns": row["wall_ns"],
      "threads": 0,
    })
    sample["threads"] += 1

  result = []
  for host_name, host in sorted(hosts.items()):
    samples = host["samples"]
    first = samples[min(samples)]
    last = samples[max(samples)]
    counts = [sample["threads"] for sample in samples.values()]
    result.append({
      "host": host_name,
      "rows": host["rows"],
      "samples": len(samples),
      "threads_min": min(counts),
      "threads_max": max(counts),
      "started_utc": format_time(first["wall_ns"]),
      "finished_utc": format_time(last["wall_ns"]),
      "duration_sec": (last["wall_ns"] - first["wall_ns"]) / 1_000_000_000,
    })
  return result


def build_statistics(threads, counts):
  host_pools = {}
  for thread in threads:
    key = (thread["host"], thread["service"], thread["pool"])
    host_pools.setdefault(key, []).append(thread)

  host_rows = []
  for key, values in sorted(host_pools.items()):
    elapsed = sum(value["elapsed_ns"] for value in values)
    runtime = sum(value["runtime_ns"] for value in values)
    wait = sum(value["wait_ns"] for value in values)
    thread_shares = [
      100.0 * min(1.0, (value["runtime_ns"] + value["wait_ns"]) / value["elapsed_ns"])
      for value in values if value["elapsed_ns"]]
    active_counts = counts.get(key, [])
    host_rows.append({
      "host": key[0],
      "service": key[1],
      "pool": key[2],
      "threads_max": max(active_counts, default=0),
      "threads_observed": len(values),
      "cpu_pct": 100.0 * runtime / elapsed if elapsed else 0.0,
      "runqueue_wait_pct": 100.0 * wait / elapsed if elapsed else 0.0,
      "runnable_pct": 100.0 * min(1.0, (runtime + wait) / elapsed) if elapsed else 0.0,
      "thread_runnable_p75": percentile(thread_shares, 0.75),
      "thread_runnable_p95": percentile(thread_shares, 0.95),
    })

  pools = {}
  for row in host_rows:
    pools.setdefault((row["service"], row["pool"]), []).append(row)

  pool_rows = []
  for key, values in sorted(pools.items()):
    runnable = [value["runnable_pct"] for value in values]
    recommended = max(1, round(percentile(runnable, 0.75)))
    pool_rows.append({
      "service": key[0],
      "pool": key[1],
      "hosts": len(values),
      "threads_min": min(value["threads_max"] for value in values),
      "threads_max": max(value["threads_max"] for value in values),
      "cpu_pct_mean": mean([value["cpu_pct"] for value in values]),
      "runqueue_wait_pct_mean": mean([
        value["runqueue_wait_pct"] for value in values]),
      "runnable_pct_mean": mean(runnable),
      "runnable_pct_host_p75": percentile(runnable, 0.75),
      "runnable_pct_host_max": max(runnable),
      "recommended_points": recommended,
    })
  return host_rows, pool_rows


def write_csv(name, rows):
  path = os.path.join(ROOT, name)
  if not rows:
    return
  with open(path, "w", newline="", encoding="utf-8") as stream:
    writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)


def format_time(timestamp_ns):
  timestamp = timestamp_ns / 1_000_000_000
  return datetime.datetime.utcfromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M:%S UTC")


def write_report(duration, started_ns, finished_ns, pool_rows):
  path = os.path.join(ROOT, "README.md")
  ordered = sorted(pool_rows, key=lambda row: row["recommended_points"], reverse=True)
  with open(path, "w", encoding="utf-8") as stream:
    stream.write("# Production CPU-affinity sample\n\n")
    stream.write(f"Observed interval: {format_time(started_ns)} — {format_time(finished_ns)}.\n\n")
    stream.write(f"Observed duration: {duration:.1f} seconds on adfe100–102 and adfe200–201.\n\n")
    stream.write("`points` is p75 of per-host mean runnable share for a worker in the pool. ")
    stream.write("Runnable share includes CPU runtime and runqueue wait and is capped at 100%.\n\n")
    stream.write("| Service/pool | Threads/host | CPU % | Wait % | Runnable % | Points |\n")
    stream.write("|---|---:|---:|---:|---:|---:|\n")
    for row in ordered:
      pool = f"{row['service']}/{row['pool']}"
      thread_range = str(row["threads_min"])
      if row["threads_min"] != row["threads_max"]:
        thread_range += f"-{row['threads_max']}"
      stream.write(
        f"| `{pool}` | {thread_range} | {row['cpu_pct_mean']:.1f} | "
        f"{row['runqueue_wait_pct_mean']:.1f} | {row['runnable_pct_mean']:.1f} | "
        f"{row['recommended_points']} |\n")

    stream.write("\nThe full per-host breakdown is in `host_pool_stats.csv`, per-thread data ")
    stream.write("is in `thread_stats.csv`, and collection checks are in ")
    stream.write("`collection_integrity.csv`. Raw scheduler snapshots are in `raw_adfe*.tsv`. ")
    stream.write("Linux `comm` truncation aliases are merged by `POOL_ALIASES` in `analyze.py`.\n")


def main():
  rows = load_rows()
  threads, counts, duration, started_ns, finished_ns = calculate(rows)
  thread_rows = build_thread_rows(threads)
  integrity_rows = build_integrity(rows)
  host_rows, pool_rows = build_statistics(threads, counts)
  write_csv("collection_integrity.csv", integrity_rows)
  write_csv("thread_stats.csv", thread_rows)
  write_csv("host_pool_stats.csv", host_rows)
  write_csv("pool_points.csv", pool_rows)
  write_report(duration, started_ns, finished_ns, pool_rows)
  print(f"rows={len(rows)}, threads={len(threads)}, pools={len(pool_rows)}")


if __name__ == "__main__":
  main()
