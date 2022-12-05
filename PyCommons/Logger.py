
from Util import *
import time, sys, string, traceback, StringIO, thread, os, codecs, types
from threading import Lock, currentThread


if sys.platform != 'win32':
  import syslog
  

defLogLevel = 5
normalLevel = 20  # level num for massive messages must be greater than this
maxLFileSize = 100 * 1024  # max and min log file size for windows for AutoTruncateLogger
minLFileSize =  10 * 1024


def makePrefix( level, useTime ):
  if useTime:
    t = time.time()
    ms = int((t - int(t)) * 1000) % 1000
    timeStr = '%s.%03d ' % (time.strftime('%Y-%m-%d %H:%M:%S', pygmtime(t)), ms)
  else:
    timeStr = ''
  threadName = currentThread().getName()
  return '%s[%s] %02d' % (timeStr, threadName, level)

class Logger:

  def __init__( self, logLevel ):
    self._logLevel = logLevel
    self.mutex = Lock()
  
  def logLogLevel( self ):
    self.log(0, 'Log level set to %d' % self.logLevel())

  def logLevel( self ):
    self.mutex.acquire()
    try:
      return self._logLevel
    finally:
      self.mutex.release()

  def setLogLevel( self, level ):
    self.mutex.acquire()
    self._logLevel = level
    self.mutex.release()
    self.logLogLevel()

  def log( self, level, msg ):
    self.mutex.acquire()
    try:
      if level <= self._logLevel:
        for line in msg.split('\n'):
          self._log(level, line)
    finally:
      self.mutex.release()

  # to be called from SIGHUP handler
  def reopen( self ):
    self.mutex.acquire()
    try:
      self._reopen()
    finally:
      self.mutex.release()

  def _reopen( self ):
    pass
    

class FileLogger(Logger):
  
  def __init__( self, logLevel, fileName = None, alwaysSync = False ):
    Logger.__init__(self, logLevel)
    self._fileName = fileName
    if fileName is None or fileName == '-':
      self._file = sys.stdout
    elif fileName == 'stderr':
      self._file = sys.stderr
    else:
      self._file = file(fileName, 'a+')
    self.alwaysSync = alwaysSync

  def _log( self, level, msg ):
    print >> self._file, '%s  %s' % (makePrefix(level, True), msg)
    if level <= normalLevel or self.alwaysSync: self._file.flush()

  def _reopen( self ):
    if self._fileName is None or self._fileName == '-': return
    self._log(0, 'Reopening...')
    self._file.close()
    self._file = file(self._fileName, 'a+')
    self._log(0, 'Reopened. Log level = %d' % self._logLevel)
    

class SysLogger(Logger):

  def __init__( self, app, logLevel ):
    Logger.__init__(self, logLevel)
    syslog.openlog(app, syslog.LOG_PID, syslog.LOG_USER)

  def _getPriority( self, level ):
    if level <=  2: return syslog.LOG_ERR
    if level <=  3: return syslog.LOG_WARNING
    if level <=  5: return syslog.LOG_NOTICE
    if level <= 10: return syslog.LOG_INFO
    return syslog.LOG_DEBUG

  def _log( self, level, msg ):
    syslog.syslog(self._getPriority(level), '%s  %s' % (makePrefix(level, False), msg))


# drops oldest lines from log file periodically, keeping maximum and minimum log sizes
class AutoTruncateLogger(FileLogger):

  def __init__( self, logLevel, fileName, alwaysSync, maxSize, minSize ):
    FileLogger.__init__(self, logLevel, fileName, alwaysSync)
    self.maxSize = maxSize
    self.minSize = minSize

  def _log( self, level, msg ):
    FileLogger._log(self, level, msg)
    self._checkSize()

  def _checkSize( self ):
    f = self._file
    if f.tell() < self.maxSize: return
    f.seek(-self.minSize, 2)  # 2 means relative to the file's end
    lines = f.readlines()
    f.seek(0)
    f.truncate()
    f.writelines(lines[1:])  # first line is not complete
    FileLogger._log(self, 0, 'Log is truncated to approx %d bytes / %d lines.' % (self.minSize, len(lines)))


class NullLogger(Logger):

  def _log( self, level, msg ):
    pass


logger = FileLogger(defLogLevel)


def _initLog( logLevel = None, fileName = None, alwaysSync = 0, app = 'python' ):
  #use logger.set please
  global logger
  if logLevel is None: logLevel = defLogLevel
  if sys.platform != 'win32' and fileName == 'syslog':
    logger = SysLogger(app, logLevel)
  elif fileName == 'none':
    logger = NullLogger(0)
  elif sys.platform == 'win32':
    logger = AutoTruncateLogger(logLevel, fileName, alwaysSync, maxLFileSize, minLFileSize)
  else:
    logger = FileLogger(logLevel, fileName, alwaysSync)
  logger.logLogLevel()

def initLog( logLevel = None, fileName = None, app = 'python' ):
  alwaysSync = 0
  for v in sys.argv:
    l = string.split(v, '=')
    if len(l) == 2 and l[0] == '--loglevel':
      logLevel = int(l[1])
    if len(l) == 2 and l[0] == '--log':
      fileName = l[1]
    if len(l) == 2 and l[0] == '--logsync':
      alwaysSync = int(l[1])
  _initLog(logLevel, fileName, alwaysSync, app)


def logLevel():
  return logger.logLevel()

def setLogLevel( level ):
  logger.setLogLevel(level)
  
def log( level, msg ):
  if logger:
    logger.log(level, msg)
  else:
    print '%s [starting]: %s' % (makePrefix(level, True), msg)

def logReopen():
  if logger:
    logger.reopen()


def logException( level ):
  for line in traceback.format_exc().splitlines():
    log(1, '  %s' % line)
  # unit test support
  # log Exception executed on failed thread - THIS trace we want to see on unittest output
  setThreadTrace()

def strException():
  f = StringIO.StringIO()
  traceback.print_exc(100, f)
  trace = f.getvalue()
  return trace

# unit test support
_threadExInfo = None

def setThreadTrace():
  global _threadExInfo
  if not _threadExInfo: # or we are not first failed thread
    _threadExInfo = sys.exc_info()

def clearThreadTrace():
  global _threadExInfo
  _threadExInfo = None

def getThreadTrace():
  return _threadExInfo

