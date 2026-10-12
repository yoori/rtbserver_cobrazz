#!/usr/bin/env python3

import argparse
import contextlib
import json
import os
from pathlib import Path
import select
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[3]


def require(condition, message):
  if not condition:
    raise RuntimeError(message)


def run(command, env, code=0):
  result = subprocess.run(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    universal_newlines=True, timeout=15)
  require(result.returncode == code, "{} returned {}: {}".format(
    command, result.returncode, result.stdout))
  return result.stdout


@contextlib.contextmanager
def server(binary_dir, mode):
  process = subprocess.Popen([str(binary_dir / "DbAccessTest"), "--serve", mode],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True)
  try:
    require(select.select([process.stdout], [], [], 10)[0], "test server startup timed out")
    endpoint = process.stdout.readline().strip()
    require(endpoint.startswith("127.0.0.1:"), "test server did not start")
    yield endpoint
  finally:
    process.stdin.close()
    try:
      process.wait(timeout=5)
    except subprocess.TimeoutExpired:
      process.kill()
      process.wait()


def test(binary_dir):
  env = os.environ.copy()
  env["PATH"] = str(binary_dir) + os.pathsep + str(ROOT / "bin") + os.pathsep + env["PATH"]
  env["PERL5LIB"] = str(ROOT / "DACS")
  env["DBACCESS_TIMEOUT_MS"] = "2000"
  with tempfile.TemporaryDirectory(prefix="dbaccess-cli-") as tmp, \
      server(binary_dir, "normal") as normal, server(binary_dir, "unsupported") as unsupported, \
      server(binary_dir, "failure") as failure, server(binary_dir, "slow") as slow:
    work = Path(tmp)
    env["config_root"] = tmp
    env["workspace_root"] = tmp
    (work / "localhost").mkdir()
    config = work / "localhost/DBAccess.json"

    def probe(endpoint, state, code=0, timeout="2000"):
      return run(["GrpcProbeObj", endpoint, timeout, "-control", "DB", str(state)], env, code)

    def group(state, code=0, service=None):
      command = [str(ROOT / "bin/DBAccess.py"), str(config), str(state)]
      return run(command + ([service] if service else []), env, code)

    def targets(entries):
      config.write_text(json.dumps({"services": [
        {"name": name, "endpoint": endpoint} for name, endpoint in entries]}))

    require("enabled" in probe(normal, -1), "initial status")
    probe(normal, 0)
    probe(normal, 0)
    require("disabled" in probe(normal, -1, 1), "disabled status")
    probe(normal, 1)
    for state in (-1, 0, 1):
      require("not applicable" in probe(unsupported, state), "unsupported state was not explicit")
    require("Test DB failure" in probe(failure, 0, 2), "server failure was hidden")
    require("unconfirmed" in probe(slow, 0, 2, "100"), "timeout did not report unknown state")
    time.sleep(0.4)
    require("disabled" in probe(slow, -1, 1), "timed out operation did not complete")
    for invalid in ("0", "-1", "999999999999999999999", "12junk"):
      run(["GrpcProbeObj", normal, invalid, "-control", "DB", "0"], env, 2)

    targets([("LogGeneralizer", normal), ("CampaignServer", unsupported)])
    require("not applicable" in group(0), "group lost not-applicable result")
    group(-1, 1)
    group(-1, 0, "CampaignServer")
    group(1)
    targets([("LogGeneralizer", normal), ("CampaignServer", failure)])
    result = group(0, 2)
    require("LogGeneralizer" in result and "CampaignServer" in result, "partial results missing")
    require("disabled" in probe(normal, -1, 1), "successful member of failed group was rolled back")

    def dacs_command(module, method):
      script = (
        'use ' + module + '; no warnings "redefine"; '
        '*AdServer::Functions::execute_command = sub { print $_[2]; return 1; }; '
        '*' + module + '::is_alive = sub { return 1; }; ' +
        module + '::' + method + '("localhost", "test");')
      return run(["perl", "-Mthreads", "-Mthreads::shared", "-e", script], env)

    stop = dacs_command("AdServer::DBAccess", "stop")
    run(["bash", "-c", stop], env, 2)
    require("Test DB failure" in (work / "log/DBAccess/DBAccess.log").read_text(),
      "DACS did not log partial failure")
    targets([(name, normal) for name in ("LogGeneralizer", "CampaignServer", "ChannelServer")])
    for module in ("LogProcessing::LogGeneralizer", "CampaignSvcs::CampaignServer",
        "ChannelSvcs::ChannelServer"):
      status = dacs_command("AdServer::" + module, "db_status")
      run(["bash", "-c", status], env, 1)
    run(["bash", "-c", dacs_command("AdServer::DBAccess", "start")], env)
    group(-1)

  require("GrpcProbeObj:" in probe(normal, -1, 2), "unavailable endpoint reported success")
  print("PASS: gRPC CLI, group partial failure, DACS start/stop/status, N/A, timeout, unavailable")


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--bin-dir", type=Path, default=ROOT / "build/bin")
  test(parser.parse_args().bin_dir.resolve())
