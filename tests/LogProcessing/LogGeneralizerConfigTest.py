#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile
from xml.etree import ElementTree as etree


ROOT = Path(__file__).resolve().parents[2]
NS = {"cfg": "http://www.adintelligence.net/xsd/AdServer/Configuration"}


def run(command, directory):
  result = subprocess.run(command, cwd=directory, stdout=subprocess.PIPE,
    stderr=subprocess.PIPE, universal_newlines=True, timeout=30)
  if result.returncode:
    raise RuntimeError(result.stderr)


def test():
  with tempfile.TemporaryDirectory(prefix="loggeneralizer-config-") as directory:
    work = Path(directory)
    for port, grpc_port in ((None, None), ("18011", None), ("18011", "19511")):
      source = etree.parse(str(ROOT / "CMS/tests/Configs/cobrazz/adserver.xml"))
      generalizers = source.findall(
        ".//service[@descriptor='AdCluster/BackendSubCluster/LogGeneralizer']")
      assert generalizers
      for service in generalizers:
        parent = service.find("configuration/cfg:logGeneralizer", NS)
        network = service.find("configuration/cfg:logGeneralizer/cfg:networkParams", NS)
        if network is None:
          network = etree.SubElement(parent, "{" + NS["cfg"] + "}networkParams")
        threads = parent.find("cfg:threadParams", NS)
        if threads is None:
          threads = etree.SubElement(parent, "{" + NS["cfg"] + "}threadParams")
        threads.set("min", "17")
        for key, value in (("port", port), ("grpc_port", grpc_port)):
          network.attrib.pop(key, None)
          if value is not None:
            network.set(key, value)
      config = work / "input.xml"
      source.write(str(config))
      results = {}
      for name, folder in (("LogGeneralizer", "LogProcessing"),
          ("CampaignServer", "CampaignManagement")):
        output = work / (name + ".xml")
        xpath = "//service[@descriptor='AdCluster/BackendSubCluster/" + name + "'][1]"
        run(["xsltproc", "--nonet", "--stringparam", "XPATH", xpath,
          "--stringparam", "MODE", "CENTRAL-OR-REMOTE-MODE",
          "--stringparam", "SERVICE_ID", "1", "--stringparam", "OUT_DIR", str(work),
          "-o", str(output),
          str(ROOT / "CMS/Plugin/xslt" / folder / (name + ".xsl")), str(config)], work)
        results[name] = etree.parse(str(output))
        schema_folder = "LogProcessing" if name == "LogGeneralizer" else "CampaignSvcs"
        run(["xmllint", "--nonet", "--noout", "--schema",
          str(ROOT / "xsd" / schema_folder / (name + "Config.xsd")), str(output)], work)
      lg = results["LogGeneralizer"]
      assert lg.find(".//cfg:CorbaConfig", NS) is None
      assert lg.find(".//cfg:GrpcConfig", NS).get("process_threads") == "17"
      actual = lg.find(".//cfg:GrpcConfig/cfg:Endpoint", NS).get("port")
      if port is not None:
        assert actual == (grpc_port or str(int(port) + 500)), actual
      refs = [ref.get("endpoint") for ref in results["CampaignServer"].findall(
        ".//cfg:LogGeneralizerGrpcRefs/cfg:Ref", NS)]
      assert refs and all(ref.endswith(":" + actual) for ref in refs), refs
      assert actual in (work / "logGeneralizer.port").read_text()
      assert results["CampaignServer"].find(".//cfg:LogGeneralizerCorbaRef", NS) is None
      assert results["CampaignServer"].find(".//cfg:CorbaConfig", NS) is None
    print("PASS: LogGeneralizer/CampaignServer endpoints agree for default/custom gRPC ports")


if __name__ == "__main__":
  test()
