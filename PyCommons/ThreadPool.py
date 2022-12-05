
from MTQueue import MTQueue
from SingleThread import SingleThread
from Shutdown import shutdown, ShuttingDown
from Logger import log, logException
from Util import *
from threading import Thread


class Task:
  
  def execute( self ):
    methodNotImplemented()


class FnTask(Task):
  
  def __init__( self, target, args = (), kwargs = {} ):
    self.target = target
    self.args   = args
    self.kwargs = kwargs

 
  def execute( self ):
    apply(self.target, self.args, self.kwargs)


class ThreadPool:

  def __init__( self, name, threadCount, target = None ):
    assert threadCount > 0, 'there must be at least one thread in the pool'
    self.name = name
    self.threadCount = threadCount
    if target:
      self.target = target
    else:
      self.target = self._run
    self.threads = []
    self.taskQueue = MTQueue()
    for i in range(self.threadCount):
      self.threads.append(SingleThread('%s#%d' % (self.name, i), self.target))

  def start( self ):
    for thread in self.threads:
      thread.start()

  def close( self ):
    """
      Wait for all threads to exit.
      
      Note! 
        You must send *stop signal* manually, using SShutdown.shutdown.set(), so the
        usage pattern looks like:
        - - - 
        SShutdown.shutdown.set()
        my-thread-pool.close()
    """
    assert self.threads, '%s is not started yet' % self.name
    log(11, '%s: waiting for threads...' % self.name)
    for thread in self.threads:
      thread.close()
    log(11, '%s: stopped.' % self.name)

  def execute( self, task ):
    self.taskQueue.push(task)

  def _run( self ):
    while True:
      task = self.taskQueue.pop()
      log(20, 'executing task %s...' % task)
      task.execute()
      log(20, 'done executing task %s.' % task)
