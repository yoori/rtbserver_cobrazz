#!/usr/bin/env python3

import argparse
import concurrent.futures
from http.client import HTTPConnection
import json
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET
from xml.sax.saxutils import quoteattr


ROOT = Path(__file__).resolve().parents[3]
BIN = ROOT / "build/bin"
NS = "http://www.adintelligence.net/xsd/AdServer/Configuration"
DESCRIPTOR = "AdCluster/BackendSubCluster/UrlInfoStorage"
SCHEMA = ROOT / "xsd/RequestInfoSvcs/UrlInfoStorageConfig.xsd"
STYLE = ROOT / "CMS/Plugin/xslt/RequestInfoSvcs/UrlInfoStorage.xsl"


def require(condition, message):
  if not condition:
    raise RuntimeError(message)


def run(command, cwd=None, check=True):
  result = subprocess.run([str(arg) for arg in command], cwd=cwd, stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT, universal_newlines=True, timeout=40)
  if check:
    require(result.returncode == 0, result.stdout)
  return result


def free_ports():
  with socket.socket() as first, socket.socket() as second:
    first.bind(("127.0.0.1", 0))
    second.bind(("127.0.0.1", 0))
    return first.getsockname()[1], second.getsockname()[1]


def http(port, path, body, method="POST", raw=False):
  connection = HTTPConnection("127.0.0.1", port, timeout=10)
  try:
    connection.request(method, path, body if raw else json.dumps(body),
      {"Content-Type": "application/json"})
    response = connection.getresponse()
    return response.status, json.loads(response.read())
  finally:
    connection.close()


def generate_config(work, grpc_port, http_port):
  fixture = work / "cluster.xml"
  config = work / "service.xml"
  service = '''<service descriptor="{descriptor}" host="node">
    <configuration><cfg:urlInfoStorage rocksdb_batch_size="64">
      <cfg:networkParams grpc_port="{grpc}" http_port="{http}"/>
    </cfg:urlInfoStorage></configuration></service>'''.format(
      descriptor=DESCRIPTOR, grpc=grpc_port, http=http_port)
  cluster = '''<colo:colocation xmlns:colo="http://www.foros.com/cms/colocation"
    xmlns:cfg="{ns}"><host name="node" hostName="127.0.0.1"/>
    <host name="other" hostName="127.0.0.2"/><application>
    <serviceGroup descriptor="AdCluster">
      <configuration><cfg:cluster><cfg:environment workspace_root={work}
        cache_root={cache} server_root={root}/></cfg:cluster></configuration>
      <serviceGroup descriptor="AdCluster/BackendSubCluster">{service}</serviceGroup>
    </serviceGroup></application></colo:colocation>'''.format(
      ns=NS, work=quoteattr(str(work)), cache=quoteattr(str(work / "cache")),
      root=quoteattr(str(ROOT)), service=service)
  fixture.write_text(cluster)
  xpath = "/colo:colocation/application/serviceGroup/serviceGroup/service"
  command = ["xsltproc", "--stringparam", "XPATH", xpath, "-o", config, STYLE, fixture]
  run(command, cwd=work)
  run(["xmllint", "--noout", "--schema", SCHEMA, config])
  tree = ET.parse(config)
  storage = tree.getroot().find("{" + NS + "}UrlInfoStorageConfig")
  require(storage.get("rocksdb_batch_size") == "64", "CMS ignored storage settings")
  require(storage.get("rocksdb_workers") == "2", "CMS defaults missing")
  ports = (work / "urlInfoStorage.port").read_text()
  require("['urlInfoStorageGrpc', {}]".format(grpc_port) in ports, "CMS gRPC port missing")
  require("['urlInfoStorageHttp', {}]".format(http_port) in ports, "CMS HTTP port missing")

  fixture.write_text(cluster.replace(service, service + service))
  require(run(command, cwd=work, check=False).returncode != 0, "CMS accepted two instances")
  fixture.write_text(cluster.replace('host="node"', 'host="node other"'))
  require(run(command, cwd=work, check=False).returncode != 0, "CMS accepted two hosts")
  fixture.write_text(cluster.replace(
    'grpc_port="{}" http_port="{}"'.format(grpc_port, http_port), ""))
  run(command, cwd=work)
  run(["xmllint", "--noout", "--schema", SCHEMA, config])
  fixture.write_text(cluster)
  run(command, cwd=work)

  descriptor = ET.parse(ROOT / "CMS/Plugin/AdServerAppDescriptor.xml")
  services = [item for item in descriptor.iter() if item.get("type") == "UrlInfoStorage"]
  require(len(services) == 1, "CMS service descriptor missing")
  require(services[0].get("minOccurs") == "0", "CMS requires optional UrlInfoStorage")
  require(services[0].get("maxOccurs") == services[0].get("maxHostOccurs") == "1",
    "CMS singleton limits missing")
  app_config = work / "app.xml"
  app_config.write_text('<urlInfoStorage xmlns="{}"/>'.format(NS))
  run(["xmllint", "--noout", "--schema",
    ROOT / "CMS/Plugin/xsd/RequestInfoSvcs/UrlInfoStorageApp.xsd", app_config])

  fixture.write_text(cluster.replace(service, ""))
  optional_output = work / "optional-output"
  optional_output.mkdir()
  plugin = ROOT / "CMS/Plugin"
  options = ["--services-xpath", xpath, "--app-xml", fixture,
    "--plugin-root", plugin, "--out-dir", optional_output]
  run([plugin / "exec/ServiceConf.sh", *options, "--xsl", STYLE,
    "--out-file", "UrlInfoStorage.xml"], cwd=work)
  backend_script = (plugin / "exec/BackendSubClusterConf.sh").read_text()
  require('--service-name "url_info_storage"' not in backend_script,
    "CMS still generates the retired UrlInfoStorage CORBA port")
  require(not list(optional_output.iterdir()), "CMS generated an absent service")
  fixture.write_text(cluster)
  return config


def test_http(port, kind="profile"):
  get_path = "/get_url_" + kind
  set_path = "/set_url_" + kind
  if kind == "profile":
    require(http(port, get_path, {"url": "https://grpc.test/profile"}) ==
      (200, {"text": 'Привет\n"text"\\'}), "gRPC profile is not visible through HTTP")
  require(http(port, get_path, [{"url": "https://missing.test/"},
    {"url": "https://grpc.test/empty"}]) == (200, [{"text": ""}, {"text": ""}]),
    "HTTP missing/empty profile mismatch")
  require(http(port, set_path, {"url": "https://http.test/profile",
    "text": 'HTTP: Привет\n"text"'}) == (200, {}), "HTTP set failed")
  require(http(port, set_path, [{"url": "same", "text": "first"},
    {"url": "same", "text": "last"}]) == (200, [{}, {}]), "HTTP batch set failed")
  require(http(port, get_path, {"url": "same"}) == (200, {"text": "last"}),
    "HTTP batch order not preserved")
  require(http(port, get_path, []) == (200, []), "empty batch failed")
  for path, body in [(get_path, {}), (get_path, {"url": ""}), (get_path, {"url": 5}),
    (get_path, None), (get_path, {"url": "same", "extra": 1}),
    (set_path, {"url": "same"}), (set_path, {"url": "same", "text": None})]:
    require(http(port, path, body)[0] == 400, "invalid HTTP request accepted")
  require(http(port, get_path, "{", raw=True)[0] == 400, "malformed JSON accepted")
  require(http(port, get_path, {}, method="GET")[0] == 405, "unsupported method accepted")
  status, result = http(port, get_path, [{"url": "same"}, {}, {"url": "same"}])
  require(status == 200 and result[1]["status"] == 400 and result[2] == {"text": "last"},
    "one invalid batch item affected other items")

  def roundtrip(index):
    url = "https://concurrent.test/{}".format(index)
    text = "x" * 32768 + str(index)
    require(http(port, set_path, {"url": url, "text": text}) == (200, {}), "write failed")
    require(http(port, get_path, {"url": url}) == (200, {"text": text}), "read failed")

  with concurrent.futures.ThreadPoolExecutor(max_workers=8) as executor:
    list(executor.map(roundtrip, range(32)))

  cas_path = "/compare_exchange_url_" + kind
  require(http(port, cas_path, {"url": "cas", "expected": "", "text": "start"}) ==
    (200, {"updated": True}), "CAS insert failed")
  require(http(port, cas_path, {"url": "cas", "expected": "", "text": "stale"}) ==
    (200, {"updated": False}), "CAS accepted stale write")

  def compete(index):
    return http(port, cas_path,
      {"url": "cas", "expected": "start", "text": str(index)})[1]["updated"]

  with concurrent.futures.ThreadPoolExecutor(max_workers=16) as executor:
    require(sum(executor.map(compete, range(64))) == 1, "CAS had multiple winners")
  status, results = http(port, cas_path, [
    {"url": "cas-batch", "expected": "", "text": "one"},
    {"url": "cas-batch", "expected": "one", "text": "two"},
    {"url": "cas-batch", "expected": "one", "text": "stale"},
    {"url": "invalid"}])
  require(status == 200 and results[:3] ==
    [{"updated": True}, {"updated": True}, {"updated": False}] and
    results[3]["status"] == 400, "CAS batch contract failed")


def test(config_only=False):
  with tempfile.TemporaryDirectory(prefix="url-info-storage-") as directory:
    work = Path(directory)
    if config_only:
      generate_config(work, 10433, 10434)
      print("UrlInfoStorage CMS configuration checks passed")
      return

    grpc_port, http_port = free_ports()
    config = generate_config(work, grpc_port, http_port)
    (work / "run").mkdir()
    (work / "log/UrlInfoStorage").mkdir(parents=True)
    endpoint = "127.0.0.1:{}".format(grpc_port)
    for iteration, stop_signal in enumerate((signal.SIGTERM, signal.SIGINT)):
      with (work / "process.out").open("w") as output:
        process = subprocess.Popen([str(BIN / "UrlInfoStorage"), str(config)],
          stdout=output, stderr=subprocess.STDOUT)
        try:
          deadline = time.monotonic() + 15
          while True:
            require(process.poll() is None, (work / "process.out").read_text())
            try:
              if http(http_port, "/get_url_profile", {"url": "ready"})[0] == 200:
                break
            except OSError:
              pass
            require(time.monotonic() < deadline, "startup timeout")
            time.sleep(0.05)
          pidfile = work / "run/UrlInfoStorage.pid"
          require(pidfile.exists(), "pidfile missing")
          duplicate = run([BIN / "UrlInfoStorage", config], check=False)
          require(duplicate.returncode != 0, "duplicate process accepted")
          if iteration == 0:
            print(run([BIN / "UrlInfoStorageGrpcTest", endpoint, "test"]).stdout, end="")
            test_http(http_port)
            test_http(http_port, "description")
            require(http(http_port, "/set_url_description",
              {"url": "same", "text": "site report"}) == (200, {}), "description write failed")
          require(http(http_port, "/get_url_description", {"url": "same"}) ==
            (200, {"text": "site report"}), "description lost on restart")
          require(http(http_port, "/get_url_profile", {"url": "same"}) ==
            (200, {"text": "last"}), "description overwrote categories")
          print(run([BIN / "UrlInfoStorageGrpcTest", endpoint, "persisted"]).stdout, end="")
          process.send_signal(stop_signal)
          require(process.wait(timeout=15) == 0, "service shutdown failed")
          require(not pidfile.exists(), "pidfile was not removed")
        finally:
          if process.poll() is None:
            process.kill()
            process.wait()
  print("UrlInfoStorage CMS, HTTP, gRPC, persistence and lifecycle checks passed")


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--config-only", action="store_true")
  test(config_only=parser.parse_args().config_only)
