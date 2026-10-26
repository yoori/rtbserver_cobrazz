#!/usr/bin/env python3

import argparse
import http.client
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import time
from xml.sax.saxutils import quoteattr


ROOT = Path(__file__).resolve().parents[2]


def run(command, env, check=True, timeout=15):
  result = subprocess.run(
    command, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    universal_newlines=True, timeout=timeout)
  if check and result.returncode:
    raise RuntimeError("{}: {}{}".format(command, result.stdout, result.stderr))
  return result


def dacs_command(method, env):
  script = (
    'use AdServer::LogProcessing::LogGeneralizer; '
    'no warnings "redefine"; '
    '*AdServer::Functions::execute_command = sub { print $_[2]; return 1; }; '
    'AdServer::LogProcessing::LogGeneralizer::' + method + '("localhost", "test");')
  return run(["perl", "-Mthreads", "-Mthreads::shared", "-e", script], env).stdout


def require(condition, message):
  if not condition:
    raise RuntimeError(message)


def test(binary_dir):
  with tempfile.TemporaryDirectory(prefix="loggeneralizer-pidfile-") as tmp:
    work = Path(tmp)
    (work / "In/WebStat/Intermediate/Intermediate").mkdir(parents=True)
    (work / "Out/WebStat").mkdir(parents=True)
    (work / "Out/WebStat_").mkdir(parents=True)
    env = os.environ.copy()
    env["workspace_root"] = tmp
    env["PATH"] = str(binary_dir) + os.pathsep + env.get("PATH", "")
    env["PERL5LIB"] = str(ROOT / "DACS") + os.pathsep + env.get("PERL5LIB", "")
    pidfile = work / "run/LogGeneralizer.pid"
    status_command = dacs_command("is_alive", env)
    stop_command = dacs_command("stop", env)
    start_command = dacs_command("start", env)
    require("ProcessControl" not in status_command and "ProbeObj" not in status_command,
      "status still depends on ProcessControl")
    require("kill -TERM" in stop_command, "DACS stop does not send SIGTERM")
    require("kill -9" not in stop_command, "DACS stop force-kills after timeout")
    require("pid file retained" in stop_command, "DACS stop hides a timeout")
    require("run/LogGeneralizer.pid" in start_command, "DACS start has no pidfile guard")
    guard_script = (
      'use AdServer::Functions; '
      'print AdServer::Functions::pidfile_start_guard('
      '"$ENV{workspace_root}/run/LogGeneralizer.pid", "LogGeneralizer");')
    guard = run(["perl", "-Mthreads", "-Mthreads::shared", "-e", guard_script], env).stdout

    def shell(command, check=True):
      return run(["bash", "-c", command], env, check, timeout=75)

    require(shell(status_command, False).returncode != 0, "missing pidfile reported alive")
    shell(stop_command)
    pidfile.parent.mkdir(parents=True, exist_ok=True)
    pidfile.write_text(str(os.getpid()) + "\n")
    require(shell(status_command, False).returncode != 0, "foreign PID reported alive")
    shell(stop_command)
    require(not pidfile.exists(), "stale pidfile was not removed")

    hanging_process = subprocess.Popen(
      ["bash", "-c", "trap '' TERM; while true; do sleep 1; done"], env=env)
    try:
      time.sleep(0.1)
      pidfile.write_text(str(hanging_process.pid) + "\n")
      command_script = (
        'use AdServer::Functions; '
        'print AdServer::Functions::stop_pidfile_command($ARGV[0], $ARGV[1]);')
      timeout_stop_command = run(
        ["perl", "-Mthreads", "-Mthreads::shared", "-e", command_script,
          str(pidfile), "bash"], env).stdout.replace("seq 1 60", "seq 1 1")
      stop_result = shell(timeout_stop_command, False)
      require(stop_result.returncode != 0, "stop succeeded while process stayed alive")
      require("pid file retained" in stop_result.stderr, "stop timeout was not reported")
      require(pidfile.exists(), "pidfile was removed while process stayed alive")
      require(hanging_process.poll() is None, "stop force-killed the process")
    finally:
      hanging_process.kill()
      hanging_process.wait()
      if pidfile.exists():
        pidfile.unlink()

    for mode in ("pidfile", "SIGINT", "without-db", "without-http"):
      with socket.socket() as listener, socket.socket() as http_listener, \
          socket.socket() as grpc_listener:
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]
        http_listener.bind(("127.0.0.1", 0))
        http_port = http_listener.getsockname()[1]
        grpc_listener.bind(("127.0.0.1", 0))
        grpc_port = grpc_listener.getsockname()[1]
      db_config = '' if mode == "without-db" else '''
    <cfg:DBConnection>
      <cfg:Postgres connection_string="host=127.0.0.1 port=1 dbname=test connect_timeout=1"/>
    </cfg:DBConnection>'''

      def probe(state=None, check=True):
        command = [str(binary_dir / "GrpcProbeObj"), "127.0.0.1:{}".format(grpc_port), "2000"]
        if state is not None:
          command += ["-control", "DB", str(state)]
        return run(command, env, check)

      http_config = '' if mode == "without-http" else '''
    <cfg:HttpConfig process_threads="2">
      <cfg:Endpoint host="127.0.0.1" port="{}"/>
    </cfg:HttpConfig>'''.format(http_port)

      def request(path="/stats", method="GET"):
        connection = http.client.HTTPConnection("127.0.0.1", http_port, timeout=2)
        try:
          connection.request(method, path)
          response = connection.getresponse()
          return response.status, response.getheader("Content-Type"), response.read()
        finally:
          connection.close()

      def stats():
        status, content_type, body = request()
        require(status == 200 and content_type == "application/json", "invalid /stats response")
        values = json.loads(body.decode("utf-8"))
        require(isinstance(values, dict) and values, "empty /stats")
        require(all(type(value) in (int, float) for value in values.values()),
          "counters are not JSON numbers")
        require("webStat.inputErrorCount" in values, "missing WebStat counter")
        return values

      config = work / "LogGeneralizer.xml"
      config.write_text('''<?xml version="1.0"?>
<cfg:AdConfiguration xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:schemaLocation={schema}>
  <cfg:LogGeneralizerConfig pid_file={pidfile}
    input_logs_dir={input_dir} output_logs_dir={output_dir}>
    <cfg:GrpcConfig process_threads="2">
      <cfg:Endpoint host="127.0.0.1" port="{grpc_port}"/>
    </cfg:GrpcConfig>
    {http_config}
    <cfg:Logger filename={logfile} log_level="7">
      <cfg:Suffix name=".trace" max_log_level="8"/>
    </cfg:Logger>
    {db_config}
    <cfg:LogProcessing>
      <cfg:WebStat check_logs_period="1" max_time="1" max_size="1000"
        max_upload_task_count="1" backup_mode="false"/>
    </cfg:LogProcessing>
  </cfg:LogGeneralizerConfig>
</cfg:AdConfiguration>
'''.format(
        schema=quoteattr("http://www.adintelligence.net/xsd/AdServer/Configuration " +
          str(ROOT / "xsd/LogProcessing/LogGeneralizerConfig.xsd")),
        pidfile=quoteattr(str(pidfile)), port=port, http_config=http_config,
        grpc_port=grpc_port, db_config=db_config,
        input_dir=quoteattr(str(work / "In")), output_dir=quoteattr(str(work / "Out")),
        logfile=quoteattr(str(work / (mode + ".log")))))
      output_path = work / (mode + ".out")
      with output_path.open("w") as output:
        process = subprocess.Popen(
          [str(binary_dir / "LogGeneralizer"), str(config)], env=env,
          stdout=output, stderr=subprocess.STDOUT)
        try:
          deadline = time.monotonic() + 15
          while True:
            require(process.poll() is None, "startup failed: " + output_path.read_text())
            if pidfile.exists() and shell(status_command, False).returncode == 0:
              try:
                probe()
                if mode != "without-http":
                  stats()
                break
              except (RuntimeError, OSError):
                pass
            require(time.monotonic() < deadline, "startup timed out")
            time.sleep(0.1)
          require(int(pidfile.read_text()) == process.pid, "incorrect PID")
          require(shell(guard, False).returncode != 0, "duplicate start was not rejected")
          with socket.socket() as legacy:
            require(legacy.connect_ex(("127.0.0.1", port)) != 0, "legacy CORBA port is listening")

          def admin(command, client_id=1, clear=False, check=True):
            args = [str(binary_dir / "LogGeneralizerAdmin"), command,
              "-r", "127.0.0.1:{}".format(grpc_port), "--client-id={}".format(client_id)]
            if clear:
              args.append("--clear")
            return run(args, env, check)

          if mode == "without-db":
            for command in ("stop-stat-upload", "start-stat-upload", "get-stat-info"):
              result = admin(command, check=False)
              require(result.returncode != 0 and "not supported" in result.stderr,
                "missing NotSupported for " + command)
          else:
            for command in ("start-stat-upload", "get-stat-info"):
              result = admin(command, check=False)
              require(result.returncode != 0 and "collection is not started" in result.stderr,
                "missing CollectionNotStarted for " + command)
            admin("stop-stat-upload")
            admin("stop-stat-upload", client_id=2)
            for clear in (False, True):
              require("Number of records: 0" in admin("get-stat-info", clear=clear).stdout,
                "invalid empty stats")
              admin("start-stat-upload", clear=clear)
            admin("start-stat-upload", client_id=2)
            result = admin("get-stat-info", client_id=3, check=False)
            require(result.returncode != 0 and "collection is not started" in result.stderr,
              "client collections are not isolated")

          if mode != "without-http":
            require(request(method="POST")[0] == 405, "POST /stats was not rejected")
            require(request(path="/missing")[0] == 404, "unknown route was not rejected")
            require(stats() == stats(), "reading /stats changed counters")

          if mode == "pidfile":
            require("DB enabled" in probe(-1).stdout,
              "initial DB state is not active")
            probe(0)
            probe(0)
            disabled = probe(-1, False)
            require(disabled.returncode == 1 and "DB disabled" in disabled.stdout,
              "DBAccess disable failed")
            stats()
            probe(1)
            probe(1)
            require("DB enabled" in probe(-1).stdout,
              "DBAccess enable failed")
            stats()
            shell(stop_command)
          elif mode == "SIGINT":
            process.send_signal(signal.SIGINT)
          elif mode == "without-db":
            for state in (-1, 0, 1):
              require("not applicable" in probe(state).stdout, "missing unsupported DB state")
            shell(stop_command)
          else:
            shell(stop_command)

          require(process.wait(timeout=15) == 0, "service did not exit cleanly")
          require(not pidfile.exists(), "pidfile remains after shutdown")
          require("service stopped." in (work / (mode + ".log.trace")).read_text(),
            "shutdown did not complete")
          require(shell(status_command, False).returncode != 0, "stopped service reported alive")
          shell(stop_command)
        finally:
          if process.poll() is None:
            process.terminate()
            try:
              process.wait(timeout=15)
            except subprocess.TimeoutExpired:
              process.kill()
              process.wait()

    print("PASS: /stats JSON, HTTP errors, optional HTTP, pidfile lifecycle, DBAccess, business gRPC, shutdown")


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--bin-dir", type=Path, default=ROOT / "build/bin")
  test(parser.parse_args().bin_dir.resolve())
