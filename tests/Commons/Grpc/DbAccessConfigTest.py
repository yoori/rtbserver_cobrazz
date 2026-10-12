#!/usr/bin/env python3

import json
import os
from pathlib import Path
import runpy
import subprocess
import tempfile
from unittest import mock
from xml.etree import ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
NS = {"cfg": "http://www.adintelligence.net/xsd/AdServer/Configuration"}
UTILITY = runpy.run_path(str(ROOT / "bin/DBAccess.py"))


def require(condition, message):
  if not condition:
    raise RuntimeError(message)


def test():
  with tempfile.TemporaryDirectory(prefix="dbaccess-json-") as directory:
    work = Path(directory)
    source = ET.parse(str(ROOT / "CMS/tests/Configs/cobrazz/adserver.xml"))
    cluster = source.find(".//serviceGroup[@descriptor='AdCluster']")
    require(cluster is not None, "fixture has no AdCluster")
    for service in cluster.findall(".//service"):
      if service.get("descriptor", "").endswith("/LogGeneralizer"):
        parent = service.find("configuration/cfg:logGeneralizer", NS)
        network = parent.find("cfg:networkParams", NS)
        if network is None:
          network = ET.SubElement(parent, "{" + NS["cfg"] + "}networkParams")
        network.set("grpc_port", "19511")
    config_source = work / "input.xml"
    source.write(str(config_source))
    nonempty = 0
    for host in [node.get("hostName") for node in source.findall("host")] + ["absent-host"]:
      config = work / "DBAccess.json"
      result = subprocess.run(["xsltproc", "--nonet",
        "--stringparam", "XPATH", "//serviceGroup[@descriptor='AdCluster'][1]",
        "--stringparam", "HOST", host, "-o", str(config),
        str(ROOT / "CMS/Plugin/xslt/DBAccess.xsl"), str(config_source)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=15)
      require(result.returncode == 0, result.stderr)
      data = json.loads(config.read_text())
      require(set(data) == {"services"}, "unexpected JSON keys")
      for service in data["services"]:
        require(set(service) == {"name", "endpoint"}, "unexpected service fields")
        require(service["endpoint"].startswith(host + ":"), "wrong host selected")
        if service["name"] == "LogGeneralizer":
          require(service["endpoint"] == host + ":19511", "custom gRPC port lost")
      if data["services"]:
        nonempty += 1
        require(UTILITY["load_services"](str(config), None) == data["services"], "loader mismatch")
      elif host == "absent-host":
        require(data == {"services": []}, "empty JSON config")
    require(nonempty > 0, "no DBAccess configs generated")

    config = work / "invalid.json"
    invalid = [
      "", "not json", "our @services = ();", "[]", "{}", '{"services": {}}',
      '{"services": []}', '{"services": [null]}',
      '{"services": [{"name": "s", "endpoint": 123}]}',
      '{"services": [{"name": "", "endpoint": "host:123"}]}',
      '{"services": [{"name": "s", "endpoint": "host:0"}]}',
      '{"services": [{"name": "s", "endpoint": "host:65536"}]}',
      '{"services": [{"name": "s", "endpoint": "host:123; touch /tmp/no"}]}',
      '{"services": [{"name": "s", "endpoint": "corbaloc:iiop:host:123/ProcessControl"}]}',
    ]
    for document in invalid:
      config.write_text(document)
      with mock.patch("subprocess.run") as probe:
        try:
          UTILITY["load_services"](str(config), None)
        except ValueError:
          pass
        else:
          raise RuntimeError("Invalid config accepted: " + document)
        probe.assert_not_called()

    target = {"name": "AnyService", "endpoint": "[::1]:123"}
    config.write_text(json.dumps({"services": [target]}))
    require(UTILITY["load_services"](str(config), "AnyService") == [target],
      "generic service/IPv6 support")
    for returncode, expected in ((0, 0), (1, 1), (2, 2), (255, 2), (-15, 2)):
      with mock.patch("subprocess.run", return_value=subprocess.CompletedProcess(
          [], returncode, "result\n")) as probe:
        code, message = UTILITY["run_for"](target, -1, 25)
        require(code == expected and "AnyService" in message, "exit status mapping")
        require(probe.call_args[0][0] ==
          ["GrpcProbeObj", "[::1]:123", "25", "-control", "DB", "-1"], "probe arguments")
        require(not probe.call_args[1].get("shell"), "probe uses shell")
    for error in (FileNotFoundError("missing"), subprocess.TimeoutExpired("GrpcProbeObj", 1)):
      with mock.patch("subprocess.run", side_effect=error):
        require(UTILITY["run_for"](target, 0, 25)[0] == 2, "probe failure hidden")

    for value in ("0", "-1", "3600001", "12junk"):
      env = dict(os.environ, DBACCESS_TIMEOUT_MS=value)
      result = subprocess.run([str(ROOT / "bin/DBAccess.py"), str(config), "-1"], env=env,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=5)
      require(result.returncode == 2 and "timeout" in result.stderr, "invalid timeout accepted")
    result = subprocess.run([str(ROOT / "bin/DBAccess.py"), str(config), "-1", "MissingService"],
      stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=5)
    require(result.returncode == 2 and "no matching" in result.stderr, "missing service accepted")
  print("PASS: CMS JSON, host filtering, validation, generic services, probe arguments and errors")


if __name__ == "__main__":
  test()
