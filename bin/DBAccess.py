#!/usr/bin/env python3

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
import re
import subprocess
import sys


def timeout_value(value):
  if not re.fullmatch(r"[0-9]+", value) or not 1 <= int(value) <= 3600000:
    raise argparse.ArgumentTypeError("timeout must be between 1 and 3600000 milliseconds")
  return int(value)


def load_services(filename, service_name):
  with open(filename, encoding="utf-8") as config:
    document = json.load(config)
  if not isinstance(document, dict) or not isinstance(document.get("services"), list):
    raise ValueError("config must contain a services array")
  services = document["services"]
  for service in services:
    if not isinstance(service, dict):
      raise ValueError("each service must be an object with name and endpoint")
    name, endpoint = service.get("name"), service.get("endpoint")
    if not isinstance(name, str) or not name.strip() or any(ord(char) < 32 for char in name):
      raise ValueError("service name must be a nonempty string without control characters")
    match = re.fullmatch(r"(?:\[[0-9a-fA-F:.%]+\]|[A-Za-z0-9_.-]+):([0-9]+)", endpoint) \
      if isinstance(endpoint, str) else None
    if match is None or not 1 <= int(match.group(1)) <= 65535:
      raise ValueError("service {!r} requires a host:port gRPC endpoint".format(name))
  targets = [service for service in services
    if service_name is None or service["name"] == service_name]
  if not targets:
    raise ValueError("no matching DBAccess services")
  return targets


def run_for(service, state, timeout_ms):
  label = "{} ({})".format(service["name"], service["endpoint"])
  command = ["GrpcProbeObj", service["endpoint"], str(timeout_ms), "-control", "DB", str(state)]
  try:
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
      universal_newlines=True, timeout=timeout_ms / 1000 + 1)
    code = result.returncode if result.returncode in (0, 1, 2) else 2
    message = result.stdout.rstrip()
    if result.returncode not in (0, 1, 2):
      message += " (GrpcProbeObj exited with {})".format(result.returncode)
    return code, "{}: {}".format(label, message)
  except subprocess.TimeoutExpired:
    return 2, "{}: GrpcProbeObj timed out; DB state is unconfirmed".format(label)
  except OSError as error:
    return 2, "{}: cannot run GrpcProbeObj: {}".format(label, error)


def main():
  parser = argparse.ArgumentParser(description="Control service DB access through GrpcProbeObj")
  parser.add_argument("config", help="generated DBAccess.json")
  parser.add_argument("state", type=int, choices=(-1, 0, 1),
    help="-1: status, 0: disable, 1: enable")
  parser.add_argument("service", nargs="?", help="optional service name filter")
  parser.add_argument("--timeout-ms", type=timeout_value,
    default=os.environ.get("DBACCESS_TIMEOUT_MS", "120000"))
  args = parser.parse_args()
  try:
    targets = load_services(args.config, args.service)
  except (OSError, ValueError) as error:
    parser.error(str(error))
  with ThreadPoolExecutor(max_workers=min(32, len(targets))) as workers:
    results = workers.map(lambda service: run_for(service, args.state, args.timeout_ms), targets)
    code = 0
    for status, message in results:
      print(message)
      code = max(code, status)
  return code


if __name__ == "__main__":
  sys.exit(main())
