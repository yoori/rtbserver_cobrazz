#!/usr/bin/env python3

import argparse
import json
from pathlib import Path
import socket
import subprocess
import tempfile
import time
from xml.sax.saxutils import quoteattr


ROOT = Path(__file__).resolve().parents[3]
NAMESPACE = "http://www.adintelligence.net/xsd/AdServer/Configuration"


def require(condition, message):
  if not condition:
    raise RuntimeError(message)


def run(command, code=0, data=None):
  result = subprocess.run(command, input=data, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    universal_newlines=True, timeout=15)
  if code is not None:
    require(result.returncode == code, "{}: {}".format(command, result.stdout))
  return result


def test(binary_dir):
  with tempfile.TemporaryDirectory(prefix="dbaccess-services-") as tmp:
    work = Path(tmp)
    for mode in ("campaign-db", "campaign-proxy", "channel"):
      with socket.socket() as corba_socket, socket.socket() as grpc_socket:
        corba_socket.bind(("127.0.0.1", 0))
        grpc_socket.bind(("127.0.0.1", 0))
        corba_port = corba_socket.getsockname()[1]
        grpc_port = grpc_socket.getsockname()[1]
      endpoint = "127.0.0.1:{}".format(grpc_port)
      mode_dir = work / mode
      mode_dir.mkdir()
      pidfile = mode_dir / "service.pid"
      logfile = mode_dir / "service.log"
      grpc_config = '''<cfg:GrpcConfig process_threads="2" cq_threads="1" threads_per_cq="1">
        <cfg:Endpoint host="127.0.0.1" port="{}"/>
      </cfg:GrpcConfig>'''.format(grpc_port)
      logger = '''<cfg:Logger filename={} log_level="7">
        <cfg:Suffix name=".trace" max_log_level="8"/>
      </cfg:Logger>'''.format(quoteattr(str(logfile)))
      if mode == "channel":
        binary = "ChannelServer"
        schema = "ChannelSvcs/ChannelServerConfig.xsd"
        service = '''<cfg:ChannelServerConfig log_root={root} pid_file={pid}
          count_chunks="1" merge_size="8" update_period="1" service_index="0">
          {grpc}<cfg:MatchOptions nonstrict="false"/>{logger}
        </cfg:ChannelServerConfig>'''.format(
          root=quoteattr(str(mode_dir)), pid=quoteattr(str(pidfile)),
          grpc=grpc_config, logger=logger)
      else:
        binary = "CampaignServer"
        schema = "CampaignSvcs/CampaignServerConfig.xsd"
        source = '''<cfg:ProxyMode><cfg:CampaignServerGrpcRefs>
          <cfg:Ref endpoint="127.0.0.1:1"/>
        </cfg:CampaignServerGrpcRefs></cfg:ProxyMode>'''
        if mode == "campaign-db":
          source = '''<cfg:ServerMode stat_stamp_sync_period="3600">
            <cfg:PGConnection
              connection_string="host=127.0.0.1 port=1 dbname=test connect_timeout=1"/>
            <cfg:LogGeneralizerGrpcRefs>
              <cfg:Ref endpoint="127.0.0.1:1"/>
            </cfg:LogGeneralizerGrpcRefs></cfg:ServerMode>'''
        service = '''<cfg:CampaignServer log_root={root} pid_file={pid} server_id="1"
          config_update_period="3600" ecpm_update_period="3600" bill_stat_update_period="0">
          {grpc}{logger}{source}
        </cfg:CampaignServer>'''.format(root=quoteattr(str(mode_dir)),
          pid=quoteattr(str(pidfile)), port=corba_port,
          grpc=grpc_config, logger=logger, source=source)
      config = mode_dir / "config.xml"
      config.write_text('''<cfg:AdConfiguration xmlns:cfg="{namespace}"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation={schema}>
        {service}</cfg:AdConfiguration>'''.format(namespace=NAMESPACE,
          schema=quoteattr(NAMESPACE + " " + str(ROOT / "xsd" / schema)), service=service))
      run(["xmllint", "--noout", "--schema", str(ROOT / "xsd" / schema), str(config)])

      def probe(state=None, code=0):
        command = [str(binary_dir / "GrpcProbeObj"), endpoint, "2000"]
        if state is not None:
          command += ["-control", "DB", str(state)]
        return run(command, code)

      with (mode_dir / "process.out").open("w") as output:
        process = subprocess.Popen([str(binary_dir / binary), str(config)],
          stdout=output, stderr=subprocess.STDOUT)
        try:
          deadline = time.monotonic() + 15
          while probe(code=None).returncode == 2:
            require(process.poll() is None, "{} failed: {}".format(
              mode, (mode_dir / "process.out").read_text()))
            require(time.monotonic() < deadline, mode + " startup timeout")
            time.sleep(0.1)
          require(pidfile.exists(), mode + " has no pidfile")
          if mode != "channel":
            run([str(binary_dir / "CampaignServerGrpcTest"), endpoint])
            with socket.socket() as old_listener:
              old_listener.settimeout(0.2)
              require(old_listener.connect_ex(("127.0.0.1", corba_port)) != 0,
                "CampaignServer opened the retired CORBA port")
          if mode == "channel":
            require("Not ready" in probe(-1, 2).stdout, "unconfigured DB reported disabled")
            request = {"campaign_refs": ["127.0.0.1:1"],
              "pg_connection": "host=127.0.0.1 port=1 dbname=test connect_timeout=1",
              "count_chunks": 1, "colo": 1, "version": "test", "check_sum": 1, "sources": [0]}
            run([str(binary_dir / "ChannelServerGrpcTestClient"), endpoint, "set_sources"],
              data=json.dumps(request))
            deadline = time.monotonic() + 10
            while probe(-1, None).returncode == 2:
              require(process.poll() is None, "ChannelServer exited while installing sources")
              require(time.monotonic() < deadline, "ChannelServer did not install sources")
              time.sleep(0.1)
          if mode == "campaign-proxy":
            for state in (-1, 0, 1):
              require("not applicable" in probe(state).stdout, "proxy DB is not N/A")
          else:
            for state in (0, 0, 1, 1):
              probe(state)
              probe(-1, 0 if state else 1)
          process.terminate()
          require(process.wait(timeout=15) == 0, mode + " shutdown failed")
          require(not pidfile.exists(), mode + " left its pidfile")
          probe(-1, 2)
          print("PASS: " + mode, flush=True)
        except Exception as error:
          trace = logfile.with_name(logfile.name + ".trace")
          raise RuntimeError("{}: {}\n{}\n{}".format(mode, error,
            (mode_dir / "process.out").read_text(), trace.read_text() if trace.exists() else ""))
        finally:
          if process.poll() is None:
            process.kill()
            process.wait()
  print("PASS: CampaignServer DB/proxy, ChannelServer DB, repeated switches and pidfile shutdown")


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--bin-dir", type=Path, default=ROOT / "build/bin")
  test(parser.parse_args().bin_dir.resolve())
