
from Daemon import Daemon
from Shutdown import shutdown, ShuttingDown
from Singleton import Singleton
from Logger import log, initLog, logException, logReopen
from Util import *
import sys, os, os.path, string, signal, time, threading

# command line arguments: what to do with server. By default is 'run'

actRun         = 'run'
actStart       = 'start'
actStop        = 'stop'
actRestart     = 'restart'
actStatus      = 'status'
actHup         = 'hup'
actShowVersion = '-v'

actions = [actRun, actStart, actStop, actRestart, actStatus, actHup, actShowVersion]


class SrvApp(Singleton, Daemon):

  def __init__( self, srvName, srvDesc, defCfgFilePath):
    Singleton.__init__(self)
    self.workDir  = None
    self.cfgFilePath = None
    # used by relativePath
    self.workDir  = self.workDir or os.getcwd()  
    self.appDir   = os.path.dirname(sys.argv[0])
    self.action   = self._parseArgs()            # what to do
    pidFilePath = '%s.pid' % self.relativePath(self.workDir, srvName)
    Daemon.__init__(self, pidFilePath, srvDesc)
    self.cfgFilePath = self.cfgFilePath or self.relativePath(self.appDir, defCfgFilePath)
    self.daemonMode  = False
    self.signalMutex = threading.Lock()  # signal processing flag

  def _parseArgs( self ):
    action = actRun
    for v in sys.argv[1:]:
      vl = string.split(v, '=')
      if len(vl) == 2:
        if vl[0] in ['--cfgfile', '-c']:
          self.cfgFilePath = vl[1]
        elif vl[0] in ['--workdir', '-w']:
          self.workDir = vl[1]
        else:
          print 'unknown parameter %s' % v[0]
          return None
      if v in actions:
        if action <> actRun:
          print 'only one action may be specified'
          return None
        action = v
    return action

  # returns True on success, False on failure
  def run( self ):
    if self.action is None: return False  # invalid args
    if self.action == actShowVersion:
      self.showVersion()
      return True
    if self.action == actStop:
      return self.stopDaemon()
    if self.action == actHup:
      return self.hupDaemon()
    if self.action in [actRestart, actStatus]:
      print 'action "%s" is not implemented yet' % self.action
      return False
    if self.action == actStart:
      if os.path.exists(self.pidFilePath):
        print "WARNING! Pid file exists, may be server is already started."
      print 'starting %s daemon' % self.srvDesc
      self.daemonize()
      self.daemonMode = True
      # and pass thru to _run
    self._run()

  def srvDescription( self ):
    methodNotImplemented()
    
  def showVersion( self ):
    print self.srvDescription()

  # get file path relative to server root directory
  def relativePath( self, dir, *args ):
    if len(args) == 1 and not args[0]: return args[0]
    if args and args[0][0] in ['/', '.']: # it is absolute path
      return os.path.join(*args)
    return os.path.join(*(dir,) + args)

  def init( self ):
    pass

  def _init( self ):
    try:
      self.init()
    except ShuttingDown, x:
      return False
    except:
      logException(1)
      log(1, 'Error starting server: internal error')
      return False
    self.writePid()
    return True

  def start( self ):
    pass

  def _run( self ):
    signal.signal(signal.SIGINT,  self._shutdown)
    signal.signal(signal.SIGTERM, self._shutdown)
    signal.signal(signal.SIGHUP,  self._sigHup)

    if not self._init():
      shutdown.set()
      return

    try:
      self.start()
      log(2, 'Server is started')
      self.main()
      log(5, 'Stopping server...')
    except:
      logException(1)
      log(1, 'Server is stopped due to the following reason: internal error')
      shutdown.set()
    self.stop()
    log(2, 'Server is stopped.')
    self.removePid()
    
  def main( self ):
    shutdown.wait()
  
  def stop( self ):
    pass

  def initLog( self, logFileName, logLevel ):
    if self.daemonMode:
      logFilePath = self.relativePath(self.workDir, logFileName)
    else:
      logFilePath = '-'
    initLog(logLevel, logFilePath)

  def logReopen( self ):
    logReopen()

  def _sigHup( self, sigNum, frame ):
    # do not process overlapped signals
    if not self.signalMutex.acquire(0): return
    try:
      self.logReopen()
    except Exception, x:
      logException(1)
      log(1, 'SIGHUP handler: exception: %s' % x)
    except:
      logException(1)
      log(1, 'SIGHUP handler: invalid exception')
    self.signalMutex.release()

  def _shutdown( self, sigNum, frame ):
    # do not process overlapped signals
    if not self.signalMutex.acquire(False): return
    try:
      shutdown.set()
    except Exception, x:
      logException(1)
      log(1, 'signal handler: exception: %s' % x)
    except:
      logException(1)
      log(1, 'signal handler: invalid exception')
    self.signalMutex.release()
