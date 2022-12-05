#!/usr/bin/env python

from OrbTestSuite import *
from FunTest import collectSuites
import os, os.path, sys, re


def loadModules( files ):
  test = re.compile(r'^test.+\.py$')
  files = filter(test.search, files)                     
  def fileNameToModuleName( fname ):
    return os.path.splitext(fname)[0]
  moduleNames = map(fileNameToModuleName, files)
  modules = map(__import__, moduleNames)
  return modules


def funOrbRegressionTest(dir = None):
  baseDir = dir
  files = []
  for idx, v in enumerate(sys.argv[1:]):
    if v[0] == '-':
      continue
    if baseDir is None:
      baseDir = v
    else:
      files.append('%s.py' % v)
  if baseDir is None:
    print 'usage: %s <testDir> [tests...]' % sys.argv[0]
    sys.exit(1)

  sys.path.insert(0, baseDir)
  if not files:
    files = os.listdir(baseDir)
  modules = loadModules(files)

  info = makeTestInfo(baseDir)
  def collect( module ):
    suites = collectSuites(info, module)
    if suites:
      return OrbTestSuite(info, suites)
    else:
      return None
  root = OrbTestSuite(info, filter(None, map(collect, modules)))

  runTests(info, root)


if __name__ == '__main__':
  funOrbRegressionTest()
