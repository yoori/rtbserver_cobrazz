#!/usr/bin/env python3
"""Local validation for feat/raction-clickhouse (no C++ build required)."""
from __future__ import annotations

import csv
import io
import os
import re
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
failures: list[str] = []


def ok(msg: str) -> None:
  print(f'OK  {msg}')


def fail(msg: str) -> None:
  failures.append(msg)
  print(f'FAIL {msg}')


def main() -> int:
  for rel in [
    'bin/RActionClickhouseAdapter.py',
    'bin/RImpressionStatUploader.py',
  ]:
    r = subprocess.run(
      [sys.executable, '-m', 'py_compile', str(ROOT / rel)],
      capture_output=True, text=True)
    (ok if r.returncode == 0 else fail)(
      f'py_compile {rel}' + (f': {r.stderr.strip()}' if r.returncode else ''))

  sh = ROOT / 'CMS/Plugin/exec/synclogs_rsync_side_copy.sh'
  r = subprocess.run(['bash', '-n', str(sh)], capture_output=True, text=True)
  (ok if r.returncode == 0 else fail)('bash -n synclogs_rsync_side_copy.sh')

  for rel in [
    'CMS/Plugin/xslt/LogProcessing/SyncLogs.xsl',
    'CMS/Plugin/xslt/LogProcessing/ClickhouseUploader.xsl',
  ]:
    r = subprocess.run(['xmllint', '--noout', str(ROOT / rel)], capture_output=True, text=True)
    (ok if r.returncode == 0 else fail)(f'xmllint {rel}')

  sync = (ROOT / 'CMS/Plugin/xslt/LogProcessing/SyncLogs.xsl').read_text()
  if sync.count('RequestInfoManager/Out/ResearchAction/RAction_*') == 1 and 'RActionClickhouse' in sync:
    ok('SyncLogs RAction side-copy once')
  else:
    fail('SyncLogs RAction wiring unexpected')

  chxsl = (ROOT / 'CMS/Plugin/xslt/LogProcessing/ClickhouseUploader.xsl').read_text()
  if 'RActionClickhouse' in chxsl:
    ok('ClickhouseUploader watches RActionClickhouse')
  else:
    fail('ClickhouseUploader missing RActionClickhouse')

  sample = textwrap.dedent('''\
    Timestamp,Device,IP Address,UID,URL,Action ID,Order ID,Order Value
    2026-08-03 08:07:07,12700029,95.24.37.71,uid1,https://hoff.ru/,9457,,0.0
    2026-08-03_09:00:00,@1,@1.2.3.4,-,@https://x/,@100,@ord,1.5
    ''')
  with tempfile.TemporaryDirectory() as td:
    f = Path(td) / 'RAction_test.csv'
    f.write_text(sample)
    r = subprocess.run(
      [sys.executable, str(ROOT / 'bin/RActionClickhouseAdapter.py'), str(f)],
      capture_output=True, text=True)
    if r.returncode != 0:
      fail(f'adapter: {r.stderr}')
    else:
      rows = list(csv.reader(io.StringIO(r.stdout)))
      if rows[0][0] != 'time' or rows[1][5] != '9457' or rows[2][4] != 'https://x/':
        fail(f'adapter output unexpected: {rows}')
      else:
        ok('RAction adapter CSV')

  patterns = {n: re.compile(rf'android {n}([._;]|$)', re.I) for n in range(8, 17)}
  cases = [
    (8, 'mozilla/5.0 (linux; android 8.0.0; sm)', True),
    (8, 'mozilla/5.0 (linux; android 8.1.0; sm)', True),
    (8, 'mozilla/5.0 (linux; android 8; sm)', True),
    (8, 'mozilla/5.0 (linux; android 18.0; sm)', False),
    (8, 'mozilla/5.0 (linux; android 80; sm)', False),
    (10, 'mozilla/5.0 (linux; android 10; pixel)', True),
    (10, 'mozilla/5.0 (linux; android 10.0.0; pixel)', True),
    (16, 'mozilla/5.0 (linux; android 16; x)', True),
  ]
  for major, ua, expect in cases:
    got = bool(patterns[major].search(ua.lower()))
    (ok if got == expect else fail)(
      f'regex android{major} expect={expect} ua={ua!r}')

  fix = (ROOT / 'docs/sql/2026-08-03_fix_android_osversion_regexp.sql').read_text()
  set_lines = [ln for ln in fix.splitlines() if 'SET match_regexp' in ln]
  if len(set_lines) >= 9 and all("([._;]|$)" in ln and '\\' not in ln.split('=', 1)[-1] for ln in set_lines):
    ok('fix SQL pattern android N([._;]|$)')
  else:
    fail(f'fix SQL pattern mismatch: {set_lines[:2]}')

  with tempfile.TemporaryDirectory() as td:
    td_path = Path(td)
    src = td_path / 'RAction_x.csv'
    src.write_text('a\n')
    (td_path / 'ResearchAction').mkdir()
    (td_path / 'RActionClickhouse').mkdir()
    r = subprocess.run(
      [str(sh), str(src),
       str(td_path / 'ResearchAction') + '/',
       str(td_path / 'RActionClickhouse') + '/'],
      capture_output=True, text=True)
    if r.returncode == 0 and (td_path / 'ResearchAction' / 'RAction_x.csv').exists() \
        and (td_path / 'RActionClickhouse' / 'RAction_x.csv').exists():
      ok('side_copy dual rsync')
    else:
      fail(f'side_copy failed: {r.returncode} {r.stderr}')

  print(f'\nSUMMARY passed_checks_failed={len(failures)}')
  for item in failures:
    print(' -', item)
  return 1 if failures else 0


if __name__ == '__main__':
  # allow running from repo root: python3 docs/... or bin
  sys.exit(main())
