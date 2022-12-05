
import time, Util
from MTQueue import MTQueue, XTimedOut
from MTValue import MTFlag
from TestComparison import ComparisonMixin
from CORBATest import CORBAFunTest
from FunTest import tlog

class FakeCallWrapper:
  def __init__( self, test, method ):
    assert isinstance(test, ComparisonMixin)
    assert isinstance(test, CORBAFunTest)
    self.test = test                       # parent test object
    self.method = method                   # method to wrapping
    self.storedMethod = getattr( self.test, self.method)
    setattr( self.test, self.method, self.fake)
    
  @Util.abstract
  def fake(self, *args, **kw): pass
    
  
class ExceptionCallWrapper(FakeCallWrapper):

  def __init__( self, test, method, exceptions ):
    assert len(exceptions) > 0
    FakeCallWrapper.__init__( self, test, method )
    self.exceptions = exceptions           # list of raising exceptions with arguments
                                           # and call-waiting timeout
    self.gotExceptions = MTQueue()         # storage for exceptions, which was raised on call
    self.excIdx    = 0                     # current exception index

  def fake(self, *args, **kw):
    assert self.excIdx < len(self.exceptions), "Invalid fake using"
    exc = self.exceptions[self.excIdx][0]
    args = self.exceptions[self.excIdx][1]
    self.excIdx+=1
    if self.excIdx >= len(self.exceptions):
      setattr( self.test, self.method, self.storedMethod)
    self.gotExceptions.push(exc)
    tlog(20, "Call fake '%s' raising '%s'" % (self.method, exc))
    raise exc(*args)

  def wait( self, skipCalls = [], firstCallTimeout = 1 ):
    timeout = firstCallTimeout
    for exc in self.exceptions:
      try:
        gotExc = self.gotExceptions.pop(timeout)
        self.test.compare(exc[0], gotExc, "exception")
        self.test.checkCallSequence(expCalls = [self.method],
                                    skipCalls = skipCalls)
        timeout = exc[2]
      except XTimedOut:
        self.test.fail("Not got expected exception : %s" % exc[0])
    # Truth call waiting
    self.test.checkCallSequence(expCalls = [self.method],
                                skipCalls = skipCalls,
                                timeout = timeout)


class DelayCallWrapper(FakeCallWrapper):

  ContinueTimeOut = 20 # timeout for waiting continued
  
  def __init__( self, test, method, delayCount = 1 ):
    FakeCallWrapper.__init__( self, test, method )
    self.callFlag = MTFlag()
    self.continueFlag = MTFlag()
    self.delayCount = delayCount

  def fake( self, *args, **kw ):
    tlog(10, "Start delaying call '%s'" % self.method)
    self.callFlag.set()
    if not self.continueFlag.wait(self.ContinueTimeOut):
      tlog(5, "Timing out on proceed '%s' call" % self.method)
    self.delayCount-=1
    if self.delayCount:
      self.callFlag = MTFlag()
      self.continueFlag = MTFlag()
    else:
      setattr( self.test, self.method, self.storedMethod)
    tlog(10, "Delaying call '%s' done" % self.method)
    return  self.storedMethod(*args, **kw)

  def wait( self, timeOut = 1 ):
    if not self.callFlag.wait(timeOut):
      raise Exception("Timing out on waiting '%s' call" % self.method)

  def proceed( self ):
    tlog(10, "Proceed call '%s'" % self.method)
    self.continueFlag.set()
