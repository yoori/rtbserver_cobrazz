
from Shutdown import shutdown, ShuttingDown
from Logger import log, logException
from Util import *
from threading import Thread


class SingleThread:

  def __init__( self, threadName, target = None, args = () ):
    self.threadName = threadName
    self.target = target  # run method
    self.args   = args
    self.thread = Thread(name=self.threadName, target=self._run)

  def start( self ):
    self.thread.start()

  def close( self ):
    log(11, 'waiting for %s thread...' % self.threadName)
    self.thread.join()
    log(11, '%s thread stopped' % self.threadName)

  def _run( self ):
    log(11, 'thread started')
    try:
      self.run()
      log(11, 'thread finished')
    except ShuttingDown:
      log(11, 'thread shutted down')
    except:
      self.failed()

  def failed( self ):
    logException(1)
    log(1, 'thread failed')
    shutdown.set()

  def run( self ):
    assert self.target, "SThread 'run' method is not overloaded and 'target' is not passed"
    self.target(*self.args)

