
from Util import *
from Condition import Condition
from Shutdown import shutdown


class XTimedOut(Exception): pass


class MTQueue:

  def __init__( self ):
    self.queue = []
    self.cond = Condition()

  def push( self, val ):
    self.cond.acquire()
    try:
      self.queue.append(val)
      self.cond.notifyAll()
    finally:
      self.cond.release()

  def pop( self, timeOut = None ):
    shutdown.check()
    self.cond.acquire()
    try:
      while len(self.queue) == 0:
        if not self.cond.wait(timeOut):
          raise XTimedOut('Timing out waiting on mt-queue')
      val = self.queue[0]
      self.queue = self.queue[1:]
      return val
    finally:
      self.cond.release()

  def empty( self ):
    self.cond.acquire()
    try:
      return len(self.queue) == 0
    finally:
      self.cond.release()

  def getAll( self ):
    self.cond.acquire()
    try:
      list = self.queue
      self.queue = []
      return list
    finally:
      self.cond.release()

  def size( self ):
    self.cond.acquire()
    try:
      return len(self.queue)
    finally:
      self.cond.release()

  def clear( self ):
    self.cond.acquire()
    try:
      self.queue = []
    finally:
      self.cond.release()
    

