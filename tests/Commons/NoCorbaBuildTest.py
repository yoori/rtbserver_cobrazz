#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[2]
RETIRED = re.compile(r"CORBA|TAO|IDL_COMMON|(?:^|[/\\])[^/\\]*IDL(?:[./\\]|$)")
LIBRARY = re.compile(r"\[(lib[^\]]+)\]")


def check_sources():
  source = (ROOT / "CMakeLists.txt").read_text()
  assert not re.search(r"find_package\(\s*(?:TAO|IDL)\b", source)
  assert "IDL_DIRECTORY" not in source
  assert "CORBA_INCLUDES" not in source
  assert "/src/CORBA" not in source
  for name in ("Plain/Parsing/CMakeLists.txt", "cmake/Modules/FindLL.cmake",
      "cmake/Modules/FindYY.cmake"):
    assert "IDL" not in (ROOT / name).read_text(), name


def check_build(build, binaries):
  directories = build / "CMakeFiles/TargetDirectories.txt"
  assert directories.is_file(), "Configure the project first: " + str(build)
  linked = 0
  checked = set()
  for name in directories.read_text().splitlines():
    directory = Path(name)
    assert not RETIRED.search(str(directory.relative_to(build))), name
    link_file = directory / "link.txt"
    if not link_file.is_file():
      continue
    command = shlex.split(link_file.read_text())
    assert not any(RETIRED.search(token) for token in command), str(link_file)
    linked += 1
    if binaries and "-o" in command:
      artifact = (directory.parents[1] / command[command.index("-o") + 1]).resolve()
      if artifact in checked:
        continue
      assert artifact.is_file(), "Build the target first: " + str(artifact)
      dynamic = subprocess.check_output(["readelf", "-d", str(artifact)],
        universal_newlines=True, stderr=subprocess.STDOUT)
      for dependency in LIBRARY.findall(dynamic):
        assert not RETIRED.search(dependency), str(artifact) + ": " + dependency
      checked.add(artifact)
  assert linked, "No link commands found in " + str(build)
  print("PASS: {} link commands, {} ELF artifacts without CORBA/TAO".format(
    linked, len(checked)))


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("build", type=Path)
  parser.add_argument("--binaries", action="store_true")
  args = parser.parse_args()
  check_sources()
  check_build(args.build.resolve(), args.binaries)


if __name__ == "__main__":
  main()
