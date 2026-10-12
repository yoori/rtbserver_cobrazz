#!/usr/bin/env python3

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
REMOVED = (
  "cmake/Modules/FindTAO.cmake",
  "cmake/Modules/FindIDL.cmake",
  "Commons/CorbaAlgs.hpp",
  "Commons/CorbaObject.hpp",
  "Commons/CorbaConfig.hpp",
  "Commons/CorbaConfig.cpp",
  "Commons/CorbaTypes.idl",
  "CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp",
  "CampaignSvcs/CampaignCommons/CampaignCommons.idl",
  "CampaignSvcs/CampaignServer/CampaignServer.idl",
  "CampaignSvcs/CampaignServer/BillStatInfo.idl",
  "CampaignSvcs/CampaignServer/StatInfo.idl",
  "CampaignSvcs/CampaignCommons/CorbaCampaignTypes.hpp",
  "CampaignSvcs/CampaignCommons/ExpressionChannelCorbaAdapter.hpp",
  "CampaignSvcs/CampaignServer/NonLinkedExpressionChannelCorbaAdapter.hpp",
  "CampaignSvcs/CampaignCommons/CampaignConfigAdapter.hpp",
  "CampaignSvcs/CampaignCommons/ExpressionChannelConfigAdapter.hpp",
  "CampaignSvcs/CampaignServer/NonLinkedExpressionChannelConfigAdapter.hpp",
)
SOURCE_ROOTS = (
  "CampaignSvcs", "ChannelSvcs", "Commons", "Frontends", "LogProcessing",
  "RequestInfoSvcs", "UserInfoSvcs", "Utils", "tests",
)
CORBA_INCLUDE = re.compile(
  r'^\s*#\s*include\s*[<"](?:CORBA/|CORBACommons/|tao/|'
  r'Commons/Corba(?:Algs|Config|Object|Types)\.hpp|'
  r'CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter\.hpp|'
  r'CampaignSvcs/(?:CampaignCommons|CampaignServer)/[^">]*Corba[^">]*\.hpp|'
  r'[^">]*_[cs]\.hpp)', re.MULTILINE)


def test():
  for name in REMOVED:
    assert not (ROOT / name).exists(), "Retired CORBA source remains: " + name

  spec = (ROOT / "RPM/SPECS/server.spec").read_text()
  assert not re.search(r"ace-tao|__ace_ver_req|CORBAInfResolve", spec), "Retired RPM dependencies"
  assert "BuildRequires: pkgconfig(ACE)" in spec, "HTTP/Stream still require ACE headers"
  assert "Requires: libACE.so.6.2.1()(64bit)" in spec, "HTTP/Stream still require the ACE runtime"
  for name in ("CampaignCommons/CampaignConfigTypes.hpp", "CampaignServer/CampaignServerTypes.hpp",
      "CampaignServer/StatTypes.hpp", "CampaignServer/BillStatTypes.hpp"):
    source = (ROOT / "CampaignSvcs" / name).read_text()
    assert not re.search(r"\bstruct\s+\w+", source), "Intermediate campaign DTO remains: " + name

  cluster = ROOT / "tests/AutoTests/PyUtils/Cluster"
  assert not any(path.is_file() for path in cluster.rglob("*")), str(cluster)

  for name in ("foros-server.supp", "foros-server6.supp", "tests/Test.supp",
      "tests/AutoTests/Commons/Admins/Admins.hpp"):
    assert not re.search(r"CORBA|TAO", (ROOT / name).read_text()), name

  unixcommons = ROOT.parent / "unixcommons_cobrazz"
  assert unixcommons.is_dir(), "Missing unixcommons source tree: " + str(unixcommons)
  for name in ("cmake/Modules/FindTAO.cmake", "cmake/Modules/FindIDL.cmake",
      "src/xsd/RepositoryConfig.xsd"):
    assert not (unixcommons / name).exists(), name
  for name in ("src/CORBA", "tests/CORBA"):
    directory = unixcommons / name
    assert not any(path.is_file() for path in directory.rglob("*")), str(directory)
  for name in ("src/Generics/Values.hpp", "src/Generics/Singleton.hpp", "tests/Test.supp"):
    assert not re.search(r"CORBA|Corba|TAO|DP_CLIENT_ORBS",
      (unixcommons / name).read_text()), name

  for directory in SOURCE_ROOTS:
    for path in (ROOT / directory).rglob("*"):
      if not path.is_file():
        continue
      if path.suffix in (".cpp", ".hpp", ".h"):
        assert not CORBA_INCLUDE.search(path.read_text()), str(path)
      elif path.name == "CMakeLists.txt":
        assert not re.search(r"\b(?:CORBAConfig|CORBACommons|ProcessControlIDL)\b",
          path.read_text()), str(path)

  print("PASS: retired CORBA sources, includes and CORBAConfig target removed")


if __name__ == "__main__":
  test()
