#!/usr/bin/env python3

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
REMOVED = (
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
)
SOURCE_ROOTS = (
  "CampaignSvcs", "ChannelSvcs", "Commons", "Frontends", "LogProcessing",
  "RequestInfoSvcs", "UserInfoSvcs", "Utils", "tests",
)
CORBA_INCLUDE = re.compile(
  r'^\s*#\s*include\s*[<"](?:CORBA/|CORBACommons/|tao/|'
  r'Commons/Corba(?:Algs|Config|Object|Types)\.hpp|'
  r'CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter\.hpp)', re.MULTILINE)


def test():
  for name in REMOVED:
    assert not (ROOT / name).exists(), "Retired CORBA source remains: " + name

  for directory in SOURCE_ROOTS:
    for path in (ROOT / directory).rglob("*"):
      if not path.is_file():
        continue
      if path.suffix in (".cpp", ".hpp", ".h"):
        assert not CORBA_INCLUDE.search(path.read_text()), str(path)
      elif path.name == "CMakeLists.txt":
        assert not re.search(r"\bCORBAConfig\b", path.read_text()), str(path)

  print("PASS: retired CORBA sources, includes and CORBAConfig target removed")


if __name__ == "__main__":
  test()
