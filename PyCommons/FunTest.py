
import ProcMgr
from Logger import log, initLog
from Util import *
import sys, traceback, os, os.path, string, time
from threading import currentThread

testLogLevel = 100

__funtest = 1  # flag to recognize this module in cleaning stack frames


def joinId( x, y ):
  if x:
    return '%s-%s' % (x, y)
  else:
    return y

def escapeId(id):
  table = string.maketrans(" \\/&", "____")
  return id.split('\n')[0].translate(table)

class TestInfo:

  def __init__( self, prefix, workDir, testLevel, tmpDir, logLevel, result ):
    self.prefix    = prefix   # common prefix for all test ids
    self.workDir   = workDir  # working dir for servers
    self.testLevel = testLevel  # test level
    self.tmpDir    = tmpDir
    self.logLevel  = logLevel
    self.result    = result  # TestResult

class TestResult:

  separator1 = '=' * 70
  separator2 = '-' * 70

  def __init__( self, prefix ):
    self.prefix   = prefix
    self.total    = 0
    self.failures = []  # (test name, excinfo) list
    self.errors   = []  #  --//--
    
  def startTest( self, testId, desc, level ):
    self.total += 1
    print '[%d] %s ...' % (level, desc),
    sys.stdout.flush()

  def addSuccess( self, testId, desc ):
    print 'ok'

  def addError( self, testId, desc, xInfo ):
    self.errors.append((desc, self._xInfo2str(xInfo)))
    print 'ERROR'

  def addFailure( self, testId, desc, xInfo ):
    self.failures.append((desc, self._xInfo2str(xInfo)))
    print 'FAIL'

  def addSetUpError( self, testId, desc, xInfo ):
    self.errors.append((desc, self._xInfo2str(xInfo)))
    print 'setUp ERROR'

  def _xInfo2str( self, xInfo ):
    return string.join(apply(traceback.format_exception, xInfo), '')

  def _x2str( self, error ):
    exctype, value, tb = error
    xdesc = traceback.format_exception_only(exctype, value)
    return ' '.join(xdesc).strip()

  def _is_relevant_tb_level( self, tb ):
    return tb.tb_frame.f_globals.has_key('__funtest')

  def _count_relevant_tb_levels( self, tb ):
    length = 0
    while tb and not self._is_relevant_tb_level(tb):
      length += 1
      tb = tb.tb_next
    return length

  def report( self ):
    log(1, 'total actions executed: %d, errors: %d, failures: %d' %
        (self.total, len(self.errors), len(self.failures)))
    print
    self.printErrorList('ERROR', self.errors)
    self.printErrorList('FAIL', self.failures)
    print 'total actions executed: %d, errors: %d, failures: %d' % \
          (self.total, len(self.errors), len(self.failures))
    print
    
  def printErrorList( self, flavour, errors ):
    for desc, xInfo in errors:
      print self.separator1
      print "%s: %s" % (flavour, desc)
      print self.separator2
      print "%s" % xInfo
    if errors:
      print self.separator1
      print


class TestSuite:

  class Failure(Exception): pass
  failureException = Failure

  def __init__( self, info, children = None ):
    assert isinstance(info, TestInfo)
    self.info = info
    self.children = children  # if not None - composite suite just run all its children
    self.id = None  # is set by _run
    self.setUpFailed = False

  def setUp( self ):
    pass

  def tearDown( self ):
    pass

  def isRoot( self ):
    return self.__class__ is TestSuite

  def _run( self, testId = None ):
    self.id = testId
    setUpTestId = (testId or 'root') + '-setUp'
    if not self.isRoot():
      self.setLog("setUp")
    if self.children:
      for kid in self.children:
        self.test(0, kid)
    else:
      try:
        try:
          self.setUp()
        except:
          desc = self.classDesc() + ' - setUp'
          xInfo = self.__excInfo()
          log(2, 'test %s setUp: error ====================================' % desc)
          self.info.result.addSetUpError(setUpTestId, desc, xInfo)
          self.setUpFailed = True
        try:
          self.run()
        finally:  # KeyboardInterrupt must only be here...
          if not self.isRoot():
            self.setLog('tearDown')
          self.tearDown()
      except:
        print 'Error running test suite'
        traceback.print_exc()
        xInfo = self.__excInfo()
        self.logStack(xInfo)
        # todo/problem remains: what to do - some threads are still running...
    
  def main( self ):
    
    r = self.info
    print '** funtest at %s, level=%d' % (r.workDir, r.testLevel)
    self._run()
    self.info.result.report()
    if len(self.info.result.errors):
      return False
    return True

  def classId( self ):
    if self.isRoot():
      return self.info.prefix
    else:
      return self.__class__.__name__

  def classDesc( self ):
    return self.__doc__ or self.__class__.__name__

  def testId( self, test ):
    if isinstance(test, TestSuite):
      return joinId(self.id, test.classDesc())
    else:
      return escapeId(test.__doc__ or test.__name__)
 
  def testDesc( self, test ):
    if isinstance(test, TestSuite):
      desc = test.classDesc()
    else:
      desc = test.__doc__ or test.__name__
    if self.isRoot():
      return desc
    else:
      return self.classDesc() + ': ' + desc
  
  def test( self, level, test, dep = True ):
    desc = self.testDesc(test)
    if self.setUpFailed and level < self.info.testLevel:
      print '[%d] %s ... FAIL' % (level, desc)
      return False  
    if not dep or level > self.info.testLevel:
      print '[%d] %s ... SKIPPED' % (level, desc)
      return False
    testId = self.testId(test)
    if isinstance(test, TestSuite):
      if not test.isRoot(): print ' --- %s ---' % test.classDesc()
      ok = test._run(testId)
    else:
      ok = self._primitiveTest(testId, desc, level, test)
    return ok

  def _primitiveTest( self, testId, desc, level, fn ):
    result = self.info.result
    result.startTest(testId, desc, level)
    self.setLog(testId)
    ok = False
    log(2, 'test %s (level %d): ====================================' % (desc, level))
    try:
      fn()
      time.sleep(0.2)  # let him finish before next one
      ok = True
      log(2, 'test %s: ok ====================================' % desc)
      result.addSuccess(testId, desc)
    except self.failureException, x:
      xInfo = self.__excInfo()
      result.addFailure(testId, desc, xInfo)
      time.sleep(0.2)  # let him finish before next one
      self.logStack(xInfo)
      log(2, 'test %s: failed ====================================' % desc)
    except KeyboardInterrupt:
      raise
    except:
      xInfo = self.__excInfo()
      result.addError(testId, desc, xInfo)
      time.sleep(0.2)  # let him finish before next one
      self.logStack(xInfo)
      log(2, 'test %s: error ====================================' % desc)
    return ok

  def setLog( self, id, root = False ):
    fname = os.path.join(self.info.tmpDir, '%s.log' % id)
    if not root:
      testDir = os.path.join(self.info.tmpDir, str2fname(self.classDesc()))
      if not os.path.exists(testDir):
        os.makedirs(testDir)
      fname = os.path.join(testDir, '%s.log' % id)
    initLog(testLogLevel, fname)

  def __excInfo(self):
    exctype, excvalue, tb = sys.exc_info()
    newtb = tb.tb_next
    if newtb is None:
      return (exctype, excvalue, tb)
    return (exctype, excvalue, newtb)

  def logStack( self, xInfo ):
    xtype, xval, tb = xInfo
    for lines in traceback.format_exception(xtype, xval, tb):
      for line in string.split(lines, '\n'):
        if not line: continue
        log(2, '  %s' % string.rstrip(line))

  def fail( self, msg ):
    raise self.failureException(msg)

  def failIf( self, expr, msg ):
    if expr: raise self.failureException(msg)

  def assert_( self, expr, msg = None ):
    if not expr: raise self.failureException(msg or str(expr))

  def assertEqual( self, x, y, msg = None ):
    if x != y:
      raise self.failureException('%s: %s != %s' % (msg, str(x), str(y)))

  def assertNotEqual( self, x, y, msg = None ):
    if x == y:
      raise self.failureException('%s: %s == %s' % (msg, str(x), str(y)))

  def assertRaises( self, x, fn, *args, **kw ):
    if hasattr(x,'__name__'): xname = x.__name__
    else: xname = str(x)
    try:
      apply(fn, args, kw)
      raise self.failureException, xname
    except x:
      return
    else:
      raise self.failureException, xname

  def assertNotRaises( self, x, fn, *args, **kw ):
    if hasattr(x,'__name__'): xname = x.__name__
    else: xname = str(x)
    try:
      return apply(fn, args, kw)
    except x:
      raise self.failureException, xname



class Process(ProcMgr.Process):

  def __init__( self, info, testName, procName, srcDir, srvName, cfgFName, opts = [] ):
    assert isinstance(info, TestInfo)
    self.info = info
    self.testName = testName  # test class desc
    self.tmpDirPrefix = os.path.join(info.tmpDir, '%s' % str2fname(testName))
    self.tmpDir = os.path.join(info.tmpDir, str2fname(testName), procName)
    srvPath = os.path.join(srcDir, srvName)
    self.cfgFilePath = os.path.join(self.tmpDir, cfgFName)
    cmd = '%s --cfgfile=%s %s' % \
          (os.path.join(info.workDir, srvPath), self.cfgFilePath, ' '.join(opts))
    ProcMgr.Process.__init__(self, procName, cmd)

  def relativePath( self, fname ):
    return os.path.join(self.tmpDir, fname)

  def start( self ):
    if os.access(self.tmpDir, os.W_OK):
      removeDirTree(self.tmpDir)
    os.makedirs(self.tmpDir)
    self.writeCfgFile()
    self.prepare()
    self.setUp(self.info)
    ProcMgr.Process.start(self)

  def prepare( self ):
    pass

  def setUp( self, r ):
    pass

  def writeCfgFile( self ):
    f = file(self.cfgFilePath, 'w')
    print >> f, '# Automatically generated file. Do not edit.'
    print >> f
    print >> f, 'LogLevel = %d' % self.info.logLevel
    print >> f
    self._writeCfgFile(f, self.info)
    f.close()

  def _writeCfgFile( self, f, info ):
    methodNotImplemented()


def str2fname( name ):
  for ch in ' :,|><;+&#@*^%[]\/"()':
    name = string.replace(name, ch, '_')
  return name

def collectSuites( info, module ):
  suites = []
  for name in dir(module):
    obj = getattr(module, name)
    if type(obj) == types.ClassType and \
       issubclass(obj, TestSuite) and \
       hasattr(obj, 'run'):
      suites.append(obj(info))
  return suites


def parseArgs(tmpDir, testLevel, logLevel ):
  prefix = '#no-prefix#'
  for v in sys.argv[1:]:
    vl = string.split(v, '=')
    if len(vl) == 2:
      name, val = vl
      if name == '--test-prefix':
        prefix = val
      elif name == '--tmpDir':
        tmpDir = val
      elif name in ['--testLevel', '-l']:
        testLevel = int(val)
      elif name in ['--logLevel', '-ll']:
        logLevel = int(val)
  return (prefix, tmpDir, testLevel, logLevel)


def makeTestInfo( workDir = '.' ):
  pid = os.getpid()
  host = os.uname()[1]
  suffix = '-%s-%d' % (host, pid)  # to make unique identifiers
  
  tmpDir = os.path.join(workDir, 'output')
  testLevel = None
  logLevel  = 29
  prefix, tmpDir, testLevel, logLevel = parseArgs(tmpDir, testLevel, logLevel)
  if testLevel is None:
    testLevel = 2
    
  result = TestResult(prefix)
  return TestInfo(prefix, workDir, testLevel, tmpDir, logLevel, result)

def runTests( info, root ):
  start = time.time()
  currentThread().setName('main')
  if os.access(info.tmpDir, os.W_OK):
    removeDirTree(info.tmpDir)
  os.mkdir(info.tmpDir)
  root.setLog('start', True)
  result = root.main()
  execTime = time.time() - start
  print "execution time: %d.%d (sec)" % \
        (int(execTime),
         int(execTime * 1000000 % 1000000))
  return result
  
  
def main():
  info = makeTestInfo()
  suites = collectSuites(info, __import__('__main__'))
  root = TestSuite(info, suites)
  if not runTests(info, root):
    sys.exit(1)
  sys.exit(0)

# test logging
def tlog( level, msg ):
  log(level, 'TEST: %s' % msg)
