#!/usr/bin/env python3

import argparse
import concurrent.futures
import os
import shlex
import subprocess
import sys


REMOTE_COLLECTOR = r'''
import os
import pwd
import re
import sys
import time


duration = int(sys.argv[1])
interval = int(sys.argv[2])
aduser_uid = pwd.getpwnam("aduser").pw_uid
cpu_suffix = re.compile(r":c[0-9]+$")


def read_text(path):
  with open(path, "r", encoding="utf-8", errors="replace") as stream:
    return stream.read().strip()


def process_service(pid, process_name):
  try:
    command = read_text(f"/proc/{pid}/cmdline").split("\0")
  except OSError:
    return process_name, ""

  config = next((arg for arg in command if arg.endswith(".xml")), "")
  if process_name != "FCGIServer" or not config:
    return process_name, config

  service = os.path.basename(config)
  service = re.sub(r"Config[.]xml$", "", service)
  service = re.sub(r"[.]xml$", "", service)
  return service, config


def allowed_cpus(status_path):
  with open(status_path, "r", encoding="utf-8", errors="replace") as stream:
    for line in stream:
      if line.startswith("Cpus_allowed_list:"):
        return line.split(":", 1)[1].strip()
  return ""


def current_cpu(stat_path):
  stat = read_text(stat_path)
  fields = stat.rsplit(") ", 1)[1].split()
  return fields[36]


def print_metadata():
  node_root = "/sys/devices/system/node"
  try:
    nodes = sorted(name for name in os.listdir(node_root) if re.fullmatch(r"node[0-9]+", name))
  except OSError:
    nodes = []

  for node in nodes:
    try:
      cpulist = read_text(f"{node_root}/{node}/cpulist")
      print("#META", "node", node[4:], cpulist, sep="\t")
    except OSError:
      pass

  cpu_root = "/sys/devices/system/cpu"
  try:
    cpus = sorted(
      (name for name in os.listdir(cpu_root) if re.fullmatch(r"cpu[0-9]+", name)),
      key=lambda name: int(name[3:]))
  except OSError:
    cpus = []

  for cpu in cpus:
    topology = f"{cpu_root}/{cpu}/topology"
    try:
      package = read_text(f"{topology}/physical_package_id")
      core = read_text(f"{topology}/core_id")
      siblings = read_text(f"{topology}/thread_siblings_list")
      print("#TOPO", cpu[3:], package, core, siblings, sep="\t")
    except OSError:
      pass


def collect_sample(sample):
  wall_ns = int(time.time() * 1_000_000_000)
  rows = 0
  for pid in os.listdir("/proc"):
    if not pid.isdigit():
      continue

    process_root = f"/proc/{pid}"
    try:
      if os.stat(process_root).st_uid != aduser_uid:
        continue
      process_name = read_text(f"{process_root}/comm")
      service, config = process_service(pid, process_name)
      tids = os.listdir(f"{process_root}/task")
    except OSError:
      continue

    for tid in tids:
      task_root = f"{process_root}/task/{tid}"
      try:
        thread_name = read_text(f"{task_root}/comm")
        if not cpu_suffix.search(thread_name):
          continue
        runtime_ns, wait_ns, timeslices = read_text(
          f"{task_root}/schedstat").split()[:3]
        affinity = allowed_cpus(f"{task_root}/status")
        cpu = current_cpu(f"{task_root}/stat")
      except (OSError, IndexError, ValueError):
        continue

      pool = cpu_suffix.sub("", thread_name)
      values = (
        sample,
        wall_ns,
        service,
        config,
        pid,
        tid,
        thread_name,
        pool,
        runtime_ns,
        wait_ns,
        timeslices,
        affinity,
        cpu,
      )
      print(*values, sep="\t")
      rows += 1

  print("#PROGRESS", sample, wall_ns, rows, sep="\t")
  sys.stdout.flush()


print_metadata()
started = time.monotonic()
sample = 0
while True:
  collect_sample(sample)
  sample += 1
  deadline = started + sample * interval
  if deadline > started + duration:
    break
  time.sleep(max(0.0, deadline - time.monotonic()))
'''


HEADER = (
  "sample\twall_ns\tservice\tconfig\tpid\ttid\tthread_name\tpool\t"
  "runtime_ns\twait_ns\ttimeslices\taffinity\tcurrent_cpu\n"
)


def collect_host(host, args):
  target = f"{args.user}@{host}"
  remote_command = (
    f"python3 -u -c {shlex.quote(REMOTE_COLLECTOR)} "
    f"{args.duration} {args.interval}"
  )
  command = [
    "ssh",
    "-F",
    "/dev/null",
    "-i",
    args.key,
    "-o",
    "BatchMode=yes",
    "-o",
    "ConnectTimeout=10",
    target,
    remote_command,
  ]
  raw_path = os.path.join(args.output_dir, f"raw_{host}.tsv")
  metadata_path = os.path.join(args.output_dir, f"topology_{host}.tsv")
  error_path = os.path.join(args.output_dir, f"ssh_{host}.err")

  with open(raw_path, "w", encoding="utf-8") as raw_stream, open(
    metadata_path, "w", encoding="utf-8") as metadata_stream, open(
      error_path, "w", encoding="utf-8") as error_stream:
    raw_stream.write(HEADER)
    process = subprocess.Popen(
      command,
      stdout=subprocess.PIPE,
      stderr=error_stream,
      universal_newlines=True,
      bufsize=1)
    assert process.stdout is not None
    for line in process.stdout:
      if line.startswith("#PROGRESS\t"):
        fields = line.rstrip().split("\t")
        if int(fields[1]) % 12 == 0:
          print(f"{host}: sample={fields[1]}, pinned_threads={fields[3]}", flush=True)
      elif line.startswith("#META\t") or line.startswith("#TOPO\t"):
        metadata_stream.write(line)
      else:
        raw_stream.write(line)
    return_code = process.wait()

  if return_code:
    raise RuntimeError(f"collector on {host} exited with code {return_code}")
  return host


def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("--duration", type=int, default=600)
  parser.add_argument("--interval", type=int, default=5)
  parser.add_argument("--output-dir", default=os.path.dirname(os.path.abspath(__file__)))
  parser.add_argument("--key", default="/home/jurij_kuznecov/.ssh/adkey")
  parser.add_argument("--user", default="jurij_kuznecov")
  parser.add_argument(
    "--hosts",
    nargs="+",
    default=["adfe100", "adfe101", "adfe102", "adfe200", "adfe201"])
  return parser.parse_args()


def main():
  args = parse_args()
  os.makedirs(args.output_dir, exist_ok=True)
  with concurrent.futures.ThreadPoolExecutor(max_workers=len(args.hosts)) as executor:
    futures = [executor.submit(collect_host, host, args) for host in args.hosts]
    for future in concurrent.futures.as_completed(futures):
      print(f"{future.result()}: complete", flush=True)
  return 0


if __name__ == "__main__":
  sys.exit(main())
