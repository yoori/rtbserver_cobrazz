#!/usr/bin/env python3

import argparse
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[3]


def test(build):
  commands = (ROOT / "tests/AutoTests/Commons/Admins/Command.cpp").read_text()
  assert "corbaloc" not in commands and "cmd.add_cmd_i(address);" in commands
  profiles = (ROOT / "tests/AutoTests/Units/UserProfiling/SpecificProfilesAbsent.cpp").read_text()
  assert "corbaloc" not in profiles and "CorbaRefOptEnum" not in profiles
  assert 'add_cmd_i("-r");\n    add_cmd_i(address);' in profiles
  examples = (ROOT / "tests/AutoTests/Commons/Admins/Admins.py").read_text()
  assert "corbaloc" not in examples
  for name, command, options in (
      ("RequestInfoAdmin", "print-request", ("--ref", "--host_and_port")),
      ("ExpressionMatcherAdmin", "print", ("--ref", "--expression-matcher"))):
    binary = build / "bin" / name
    result = subprocess.run([str(binary), "help"], stdout=subprocess.PIPE,
      stderr=subprocess.PIPE, universal_newlines=True, timeout=5)
    assert result.returncode == 0 and "gRPC endpoint" in result.stdout, name
    for option in options:
      for reference in ("corbaloc::127.0.0.1:1/Service", "corbaloc:iiop:127.0.0.1:1/Service",
          "corbaloc::127.0.0.1:1", "IOR:0123456789", "127.0.0.1:1/Service"):
        result = subprocess.run([str(binary), command, option + "=" + reference],
          stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, timeout=5)
        assert result.returncode != 0, (name, reference, result.stdout, result.stderr)
        assert "Expected gRPC endpoint host:port" in result.stderr, (name, result.stderr)
    print("PASS: {} rejects legacy object references; help still succeeds".format(name))


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("build", nargs="?", type=Path, default=ROOT / "build")
  test(parser.parse_args().build.resolve())
