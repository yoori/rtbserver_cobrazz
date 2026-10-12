#!/usr/bin/env python3

import copy
from pathlib import Path
import subprocess
import tempfile
from xml.etree import ElementTree as etree


ROOT = Path(__file__).resolve().parents[3]
NS = {"cfg": "http://www.adintelligence.net/xsd/AdServer/Configuration"}
Q = "{" + NS["cfg"] + "}"


def run(command, directory):
  result = subprocess.run(command, cwd=directory, stdout=subprocess.PIPE,
    stderr=subprocess.PIPE, universal_newlines=True, timeout=30)
  if result.returncode:
    raise RuntimeError(result.stderr)


def transform(work, config, name, folder, xpath, mode="CENTRAL-OR-REMOTE-MODE"):
  output = work / (name + ".xml")
  command = ["xsltproc", "--nonet"]
  for key, value in (("XPATH", xpath), ("MODE", mode), ("SERVICE_ID", "1"),
      ("OUT_DIR", str(work)), ("HOST", "adbe00"), ("CLUSTER_ID", "1"),
      ("SERVICE_COUNT", "1"), ("CONF_TYPE", "HttpFrontend")):
    command += ["--stringparam", key, value]
  command += ["-o", str(output), str(ROOT / "CMS/Plugin/xslt" / folder / (name + ".xsl")),
    str(config)]
  run(command, work)
  return etree.parse(str(output))


def test():
  with tempfile.TemporaryDirectory(prefix="campaign-grpc-config-") as directory:
    work = Path(directory)
    for base_port, grpc_port in ((None, None), ("19006", None), ("19006", "22006")):
      source = etree.parse(str(ROOT / "CMS/tests/Configs/cobrazz/adserver.xml"))
      # This fixture predates the UserInfoController rename used by the current XSLT.
      for service in source.findall(
          ".//service[@descriptor='AdCluster/FrontendSubCluster/UserInfoManagerController']"):
        service.set("descriptor", "AdCluster/FrontendSubCluster/UserInfoController")
        service.find("configuration/cfg:userInfoManagerController", NS).tag = Q + "userInfoController"
      servers = source.findall(
        ".//service[@descriptor='AdCluster/BackendSubCluster/CampaignServer']")
      assert servers
      for service in servers:
        parent = service.find("configuration/cfg:campaignServer", NS)
        network = parent.find("cfg:networkParams", NS)
        if network is None:
          network = etree.SubElement(parent, Q + "networkParams")
        for key, value in (("port", base_port), ("grpc_port", grpc_port)):
          network.attrib.pop(key, None)
          if value is not None:
            network.set(key, value)
        threads = parent.find("cfg:threadParams", NS)
        if threads is None:
          threads = etree.SubElement(parent, Q + "threadParams")
        threads.set("min", "7")
      config = work / "input.xml"
      source.write(str(config))
      xpath = "//service[@descriptor='AdCluster/BackendSubCluster/CampaignServer'][1]"
      server = transform(work, config, "CampaignServer", "CampaignManagement", xpath)
      run(["xmllint", "--nonet", "--noout", "--schema",
        str(ROOT / "xsd/CampaignSvcs/CampaignServerConfig.xsd"),
        str(work / "CampaignServer.xml")], work)
      assert server.find(".//cfg:CorbaConfig", NS) is None
      assert server.find(".//cfg:GrpcConfig", NS).get("process_threads") == "7"
      port = server.find(".//cfg:GrpcConfig/cfg:Endpoint", NS).get("port")
      if base_port is not None:
        assert port == (grpc_port or str(int(base_port) + 500)), port
      for name, folder, cluster in (
          ("CampaignManager", "CampaignManagement", "FrontendSubCluster"),
          ("BillingServer", "CampaignManagement", "FrontendSubCluster"),
          ("UserInfoManager", "UserInfoProfiling", "FrontendSubCluster"),
          ("ChannelController", "ChannelServing", "FrontendSubCluster"),
          ("ExpressionMatcher", "LogProcessing", "BackendSubCluster"),
          ("RequestInfoManager", "LogProcessing", "BackendSubCluster")):
        xpath = "//service[@descriptor='AdCluster/" + cluster + "/" + name + "'][1]"
        client = transform(work, config, name, folder, xpath)
        refs = client.findall(".//cfg:CampaignServerGrpcRefs/cfg:Ref", NS)
        assert refs and all(ref.get("endpoint").endswith(":" + port) for ref in refs), name
        assert client.find(".//cfg:CampaignServerCorbaRef", NS) is None
      client = transform(work, config, "AdFrontend", "Frontend",
        "//service[@descriptor='AdCluster/FrontendSubCluster/HttpFrontend'][1]")
      refs = client.findall(".//cfg:CampaignServerGrpcRefs/cfg:Ref", NS)
      assert refs and all(ref.get("endpoint").endswith(":" + port) for ref in refs)
      assert port in (work / "campaignServer.port").read_text()
      # Slave servers must discover primary servers through the same endpoint calculation.
      primary = servers[0].find("configuration/cfg:campaignServer", NS)
      update = primary.find("cfg:updateParams", NS)
      if update is None:
        update = etree.SubElement(primary, Q + "updateParams")
      update.set("primary", "true")
      slave = copy.deepcopy(servers[0])
      slave.set("host", "slave-test")
      slave.find("configuration/cfg:campaignServer/cfg:updateParams", NS).set("primary", "false")
      for group in source.findall(".//serviceGroup"):
        if servers[0] in list(group):
          group.append(slave)
          break
      source.write(str(config))
      server = transform(work, config, "CampaignServer", "CampaignManagement",
        "//service[@host='slave-test']")
      refs = server.findall(".//cfg:CampaignServerGrpcRefs/cfg:Ref", NS)
      assert refs and all(ref.get("endpoint").endswith(":" + port) for ref in refs)
    print("PASS: CampaignServer, six client configs and slave use matching gRPC endpoints")


if __name__ == "__main__":
  test()
