#!/usr/bin/env python3

from pathlib import Path
import os
import signal
import socket
import subprocess
import tempfile
import time
from xml.sax.saxutils import quoteattr


ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "build/bin"
NAMESPACE = "http://www.adintelligence.net/xsd/AdServer/Configuration"


def require(condition, message):
  if not condition:
    raise RuntimeError(message)


def run(command, check=True):
  env = os.environ.copy()
  env["PATH"] = str(BIN) + os.pathsep + env.get("PATH", "")
  env["PERL5LIB"] = str(ROOT / "DACS") + os.pathsep + env.get("PERL5LIB", "")
  result = subprocess.run([str(item) for item in command], stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT, universal_newlines=True, timeout=30, env=env)
  if check:
    require(result.returncode == 0, result.stdout)
  return result


def test():
  with tempfile.TemporaryDirectory(prefix="dictionary-grpc-") as directory:
    work = Path(directory)
    (work / "en").write_text("cat cats\ndog dogs\n")
    (work / "ru").write_text("кот коты\n", encoding="utf-8")
    with socket.socket() as listener:
      listener.bind(("127.0.0.1", 0))
      port = listener.getsockname()[1]
    endpoint = "127.0.0.1:{}".format(port)
    pidfile = work / "service.pid"
    schema = ROOT / "xsd/ChannelSvcs/DictionaryProviderConfig.xsd"
    config = work / "config.xml"
    config.write_text('''<cfg:AdConfiguration xmlns:cfg="{ns}"
      xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation={schema}>
      <cfg:DictionaryProviderConfig pid_file={pid} threads="2">
        <cfg:GrpcConfig process_threads="2">
          <cfg:Endpoint host="127.0.0.1" port="{port}"/>
        </cfg:GrpcConfig>
        <cfg:Dictionary lang="en" file={en}/><cfg:Dictionary lang="ru" file={ru}/>
        <cfg:Logger filename={log} log_level="7">
          <cfg:Suffix name=".trace" max_log_level="8"/>
        </cfg:Logger>
      </cfg:DictionaryProviderConfig></cfg:AdConfiguration>'''.format(
        ns=NAMESPACE, schema=quoteattr(NAMESPACE + " " + str(schema)),
        pid=quoteattr(str(pidfile)), port=port, en=quoteattr(str(work / "en")),
        ru=quoteattr(str(work / "ru")), log=quoteattr(str(work / "service.log"))))
    run(["xmllint", "--noout", "--schema", schema, config])
    print(run([BIN / "DictionaryProviderGrpcTest", config]).stdout, end="")
    guard = run(["perl", "-I", ROOT / "DACS", "-Mthreads", "-Mthreads::shared",
      "-MAdServer::Functions", "-e",
      'print AdServer::Functions::pidfile_start_guard($ARGV[0], "DictionaryProvider");',
      pidfile]).stdout
    for stop_signal in (signal.SIGTERM, signal.SIGINT):
      with (work / "process.out").open("w") as output:
        process = subprocess.Popen([str(BIN / "DictionaryProvider"), str(config)],
          stdout=output, stderr=subprocess.STDOUT)
        try:
          deadline = time.monotonic() + 10
          while run([BIN / "DictionaryProviderGrpcTest", "--remote", endpoint], False).returncode:
            require(process.poll() is None, (work / "process.out").read_text())
            require(time.monotonic() < deadline, "DictionaryProvider startup timeout")
            time.sleep(0.05)
          require(pidfile.exists(), "Missing pidfile")
          duplicate = run(["bash", "-c", guard], False)
          require(duplicate.returncode != 0, "Duplicate process accepted the same pidfile")
          require(int(pidfile.read_text().strip()) == process.pid, "Duplicate damaged pidfile")
          process.send_signal(stop_signal)
          require(process.wait(timeout=10) == 0, "DictionaryProvider shutdown failed")
          require(not pidfile.exists(), "Pidfile left after shutdown")
        finally:
          if process.poll() is None:
            process.kill()
            process.wait()
    print("PASS: real DictionaryProvider gRPC, duplicate start, SIGTERM/SIGINT, pidfile cleanup")


if __name__ == "__main__":
  test()
