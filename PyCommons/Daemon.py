
from Util import *
import sys, os, signal


class Daemon:

  def __init__( self, pidFilePath, srvDesc ):
    self.pidFilePath = pidFilePath  # absolute path to pid file, string
    self.srvDesc     = srvDesc      # server description, used in console messages; string

  def writePid( self ):
    f = file(self.pidFilePath, 'w')
    print >> f, os.getpid()
    f.close()

  def readPid( self ):
    try:
      f = file(self.pidFilePath)
      pid = int(f.readline())
      f.close()
      return pid
    except IOError:
      return None

  def removePid( self ):
    try:
      os.remove(self.pidFilePath)
    except OSError:
      pass  # can already be removed by another instance 

  def daemonize( self ):
    if os.fork() != 0: sys.exit(0)
    #os.setpgrp()
    os.setsid()
    os.close(sys.__stdin__.fileno())
    os.close(sys.__stdout__.fileno())
    os.close(sys.__stderr__.fileno())

  def stopDaemon( self ):
    print 'Stopping %s...' % self.srvDesc
    pid = self.readPid()
    if not pid:
      print '%s is not started (pid file is not found)' % self.srvDesc
      return False
    try:
      os.kill(pid, signal.SIGTERM)
    except OSError:
      print '%s is not started (process does not exist)' % self.srvDesc
      return False
    while self.readPid():
      time.sleep(0.2)
    print '%s is stopped.' % self.srvDesc
    return True

  def hupDaemon( self ):
    print 'Hupping %s...' % self.srvDesc
    pid = self.readPid()
    if not pid:
      print '%s is not started (pid file is not found)' % self.srvDesc
      return False
    try:
      os.kill(pid, signal.SIGHUP)
    except OSError:
      print '%s is not started (process does not exist)' % self.srvDesc
      return False
    print '%s is hupped.' % self.srvDesc
    return True
