#!/usr/bin/env python3

import os
from pathlib import Path
import re
import subprocess
import tempfile
from xml.etree import ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
PLUGIN = ROOT / "CMS/Plugin"
NS = "http://www.adintelligence.net/xsd/AdServer/Configuration"


def run(command, **kwargs):
  return subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    universal_newlines=True, timeout=30, **kwargs)


def check_schemas(work):
  document = work / "empty.xml"
  document.write_text('<Unknown xmlns="' + NS + '"/>')
  all_schemas = list((PLUGIN / "xsd").rglob("*.xsd"))
  all_schemas += list((ROOT / "xsd").rglob("*.xsd"))
  retired = re.compile(r"CorbaConfig|CorbaObjectRef|SimpleCorbaRef|EndpointObjectType")
  for schema in all_schemas:
    assert not retired.search(schema.read_text()), str(schema)
    ET.parse(str(schema))
  schemas = [PLUGIN / "xsd" / name for name in (
    "AdClusterAppType.xsd", "AdFrontendAppType.xsd", "AdProfilingClusterAppType.xsd",
    "AdProxyClusterAppType.xsd", "Commons/AdServerCommonsApp.xsd",
    "CampaignManagement/BillingServerAppType.xsd", "CampaignManagement/CampaignManagerAppType.xsd",
    "ChannelServing/ChannelServerAppType.xsd", "LogProcessing/ProxySyncLogsAppType.xsd",
    "LogProcessing/SyncLogsAppType.xsd", "UserInfoProfiling/UserInfoControllerAppType.xsd",
    "UserInfoProfiling/UserInfoManagerAppType.xsd")]
  schemas += [ROOT / "xsd" / name for name in (
    "AdServerCommons/AdServerCommons.xsd", "CampaignSvcs/BillingServerConfig.xsd",
    "CampaignSvcs/CampaignManagerConfig.xsd", "CampaignSvcs/CampaignServerConfig.xsd",
    "ChannelSvcs/ChannelControllerConfig.xsd")]
  for schema in schemas:
    result = run(["xmllint", "--nonet", "--noout", "--schema", str(schema), str(document)])
    assert result.returncode in (0, 3), str(schema) + ": " + result.stderr
  assert not (PLUGIN / "xsd/AdServerApp.xsd").exists()
  assert not (ROOT / "xsd/ChannelSvcs/ChannelManagerControllerConfig.xsd").exists()

  schema = work / "stunnel.xsd"
  schema.write_text('<xsd:schema xmlns:xsd="http://www.w3.org/2001/XMLSchema" '
    'xmlns:cfg="' + NS + '"><xsd:import namespace="' + NS + '" schemaLocation="' +
    str(PLUGIN / "xsd/Commons/AdServerCommonsApp.xsd") + '"/>'
    '<xsd:element name="Endpoint" type="cfg:STunnelRef"/></xsd:schema>')
  document.write_text('<Endpoint host="log-host" port="10108"/>')
  result = run(["xmllint", "--nonet", "--noout", "--schema", str(schema), str(document)])
  assert result.returncode == 0, result.stderr
  print("PASS: {} schemas compile; STunnel endpoint preserved".format(len(schemas)))


def check_templates(work):
  for stylesheet in (PLUGIN / "xslt").rglob("*.xsl"):
    source = stylesheet.read_text()
    assert not re.search(r'corbaloc:|name="(?:FillCorbaRef|GeneratePreStartRefList)"', source)
    assert "<cfg:CorbaConfig" not in source, str(stylesheet)
    ET.parse(str(stylesheet))
  for name in ("AdServer", "AdProfilingServer"):
    wrapper = work / "prestart.xsl"
    wrapper.write_text('''<xsl:stylesheet version="1.0"
      xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
      <xsl:import href="''' + str(PLUGIN / "xslt/OCM" / (name + ".xsl")) + '''"/>
      <xsl:template match="/">
        <xsl:call-template name="GeneratePreStartGrpcRefList">
          <xsl:with-param name="services"
            select="//service[@descriptor='AdCluster/FrontendSubCluster/CampaignManager']"/>
          <xsl:with-param name="service-config" select="'configuration/cfg:campaignManager'"/>
          <xsl:with-param name="def-port" select="10103"/>
          <xsl:with-param name="services-name" select="'CampaignManager'"/>
        </xsl:call-template>
      </xsl:template>
    </xsl:stylesheet>''')
    result = run(["xsltproc", "--nonet", "--stringparam", "XPATH", "/",
      "--stringparam", "ISP_ZONE", "0", "--stringparam", "CURRENT_TIME", "test",
      str(wrapper),
      str(ROOT / "CMS/tests/Configs/cobrazz/adserver.xml")], cwd=str(work))
    assert result.returncode == 0, result.stderr
    assert "#grpc://localhost:" in result.stdout, result.stdout
    assert "corbaloc" not in result.stdout
  print("PASS: XSLT cleanup; both OCM prestart generators use gRPC")


def check_prestart(work):
  ssh = work / "ssh"
  ssh.write_text('#!/bin/sh\nprintf "%s\\n" "$@" >> "$PROBE_CAPTURE"\n'
    'exit "$PROBE_EXIT"\n')
  ssh.chmod(0o700)
  capture = work / "ssh.args"
  env = dict(os.environ, PATH=str(work) + os.pathsep + os.environ["PATH"],
    PROBE_CAPTURE=str(capture), PROBE_EXIT="0")
  command = ["perl", "-I", str(PLUGIN / "lib"), str(PLUGIN / "exec/bin/PreStartChecker.pl"),
    "--ssh-identity=test-key", "--env=test-env"]
  for endpoint in ("localhost:25507", "[::1]:25507"):
    result = run(command + ["--serv-CampaignManager=local#grpc://" + endpoint], env=env)
    assert result.returncode == 0, result.stderr
    assert "GrpcProbeObj " + endpoint in capture.read_text()
  before = capture.read_text()
  for reference in ("corbaloc::localhost:10103/ProcessControl", "localhost:25507",
      "grpc://localhost:25507;bad"):
    result = run(command + ["--serv-CampaignManager=local#" + reference], env=env)
    assert result.returncode != 0 and "Expected grpc://host:port" in result.stderr
    assert capture.read_text() == before, "Invalid reference reached SSH"
  env["PROBE_EXIT"] = "1"
  result = run(command + ["--serv-CampaignManager=local#grpc://localhost:25507"], env=env)
  assert result.returncode == 1, "Failed probe reported ready"
  print("PASS: PreStartChecker gRPC, IPv6, failed probe and invalid reference rejection")


def test():
  with tempfile.TemporaryDirectory(prefix="no-corba-config-") as directory:
    work = Path(directory)
    check_schemas(work)
    check_templates(work)
    check_prestart(work)


if __name__ == "__main__":
  test()
