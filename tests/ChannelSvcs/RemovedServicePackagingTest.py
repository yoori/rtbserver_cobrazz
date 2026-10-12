#!/usr/bin/env python3

from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def run(command):
  result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    universal_newlines=True, timeout=30)
  if result.returncode:
    raise RuntimeError(result.stdout)


def test():
  source = (ROOT / "CMakeLists.txt").read_text()
  rules = re.findall(
    r"install\(DIRECTORY \$\{CMAKE_(?:RUNTIME|LIBRARY)_OUTPUT_DIRECTORY\}[^)]*\)", source)
  assert len(rules) == 3, "Review the changed binary/library installation rules"
  with tempfile.TemporaryDirectory(prefix="retired-service-packaging-") as directory:
    work = Path(directory)
    for name in ("bin/ChannelSearchService", "bin/ChannelSearchAdmin", "bin/ChannelServer",
        "bin/CTRPredictorSVMGenerator", "bin/SVMGenerator", "lib/libSVMGeneratorConfigXSD.a",
        "lib/libCTRPredictorSVMGeneratorConfigXSD.a",
        "lib/libCTRPredictorSVMGeneratorConfigXSD.so",
        "lib/libChannelSearchServiceIDL.so", "lib/libChannelSearchServiceIDL.so.1",
        "lib/libChannelSearchServiceIDL.a", "lib/libChannelSearchServiceConfigXSD.a",
        "lib/libChannelContainer.so", "lib/libChannelCommonsIDL.so",
        "lib/libChannelServerIDL.so", "lib/libChannelUpdateBaseIDL.so",
        "lib/libChannelUpdateBaseIDL.so.1", "lib/libChannelUpdateGrpcClient.so",
        "lib/libCampaignFreqIDL.so", "lib/libCookieInfoPackIDL.so", "lib/libFreqCapInfoIDL.so",
        "lib/libProcessControlVars.so", "lib/libProcessControlVars.so.1",
        "bin/ProcessControlVarsTest", "lib/libProcessControlIDL.so",
        "lib/libLogGeneralizerIDL.so", "lib/libLogGeneralizerIDL.so.1",
        "lib/libLogGeneralizerIDL.a", "lib/libLogGeneralizerGrpc.so",
        "lib/libCampaignServerIDL.so", "lib/libCampaignCommonsIDL.so", "lib/libBillStatInfoIDL.so"):
      path = work / name
      path.parent.mkdir(parents=True, exist_ok=True)
      path.write_text("test fixture\n")
    for name in ("CampaignServerStubs_v350", "CampaignServer_v350IDL",
        "CampaignCommons_v350IDL", "BillStatInfo_v350IDL"):
      for suffix in (".so", ".so.1", ".a"):
        (work / "lib" / ("lib" + name + suffix)).write_text("test fixture\n")
    project = '''cmake_minimum_required(VERSION 3.16)
project(RemovedServicePackaging NONE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/")
'''
    (work / "CMakeLists.txt").write_text(project + "\n".join(rules) + "\n")
    run(["cmake", "-S", str(work), "-B", str(work / "build")])
    run(["cmake", "--install", str(work / "build"), "--prefix", str(work / "install")])
    installed = [str(path.relative_to(work / "install"))
      for path in (work / "install").rglob("*") if path.is_file()]
    assert "bin/ChannelServer" in installed, installed
    assert "bin/SVMGenerator" in installed, installed
    assert "lib/libChannelContainer.so" in installed, installed
    assert "lib/libChannelUpdateGrpcClient.so" in installed, installed
    assert "lib/libProcessControlIDL.so" in installed, installed
    assert "lib/libLogGeneralizerGrpc.so" in installed, installed
    for name in ("CampaignServerIDL", "CampaignCommonsIDL", "BillStatInfoIDL"):
      assert "lib/lib" + name + ".so" in installed, installed
    assert not any("v350" in path for path in installed), installed
    assert not any("ProcessControlVars" in path for path in installed), installed
    assert not any("ChannelSearch" in path for path in installed), installed
    assert not any("CTRPredictorSVMGenerator" in path for path in installed), installed
    for name in ("ChannelCommonsIDL", "ChannelServerIDL", "ChannelUpdateBaseIDL",
        "CampaignFreqIDL", "CookieInfoPackIDL", "FreqCapInfoIDL", "LogGeneralizerIDL"):
      assert not any(name in path for path in installed), installed
  print("PASS: stale retired service binaries/libraries excluded; active artifacts installed")


if __name__ == "__main__":
  test()
