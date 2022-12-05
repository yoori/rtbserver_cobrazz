
from Logger import log, logException
from Util import *
from threading import Thread, Event
import subprocess, signal, os, time, string


# class to process output from a process asynchronously
class BindOutput:

  def __init__( self, name, file, fn, fnArgs = (), finishFn = None, finArgs = () ):
    self.name = name
    self.file = file
    self.fn = fn
    self.fnArgs = fnArgs
    self.finishFn = finishFn
    self.finArgs = finArgs
    self.thread = Thread(None, self._run, '%s' % self.name)
    self.thread.start()
    
  def _run( self ):
    try:
      while 1:
        line = self.file.readline()
        if not line:
          if self.finishFn:
            try:
              apply(self.finishFn, self.finArgs)
            except:
              logException(1)
          break
        if line[-1] == '\n': line = line[:-1]
        try:
          apply(self.fn, (line,) + self.fnArgs)
        except:
          logException(1)
    except:
      logException(1)

  def close( self ):
    self.thread.join()


# Class to manage (start, stop and log output) a process.
# 'start' must be called from the main thread, so that signals for subprocess work.
class Process:

  def __init__( self, name, cmd, procMgr = None ):
    self.name = name
    self.cmd = cmd
    self.procMgr = procMgr
    if self.procMgr:
      self.procMgr.add(self)

  def start( self ):
    log(2, 'starting "%s"' % self.cmd)
    try:
      self.proc = subprocess.Popen(
        self.cmd, shell=True, bufsize=0,
        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, close_fds=True)
      self.out = BindOutput(self.name, self.proc.stdout, self._output, (), self._finished)
      self.err = BindOutput(self.name, self.proc.stderr, self._error)
    except:
      logException(1)
      

  # will block until process finishes
  def _close( self ):
    self.out.close()
    self.err.close()

  def _output( self, line ):
    log(5, '%s: %s' % (self.name, line))
 
  def _error( self, line ):
    log(1, '%s: [ERROR] %s' % (self.name, line))

  def _finished( self ):
    log(3, '%s finished' % self.name)

  def _kill( self, sig = signal.SIGTERM ):
    if self.proc.poll() == None:
      log(13, 'sending signal %d to %s' % (sig, self.name))
      os.kill(self.proc.pid, sig)
    else:
      log(13, 'not sending signal %d to %s - already stopped' % (sig, self.name))

  def stop( self ):
    try:
      self._kill()
    except:
      pass
    self._close()


# Class to manage all subprocesses. Must be run in main thread
class ProcMgr:

  def __init__( self ):
    self._processes = []

  def add( self, proc ):
    self._processes.append(proc)

  def start( self ):
    for proc in self._processes:
      if proc.startOnStart:
        proc.start()

  def close( self ):
    log(11, 'stopping subprocesses...')
    for proc in self._processes:
      log(12, 'stopping %s' % proc.name)
      proc.stop()
      log(12, '%s is stopped' % proc.name)
    log(11, 'stopping subprocesses: done')

