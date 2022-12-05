
import struct
from Util import currentTime, time2str
from OrbUtil import time2orb, orb2time
from FunTest import tlog
from CORBACommons import OutOfMemory, ImplementationError, AProcessControl, IProcessControl
from TestComparison import ComparisonMixin
from CORBATestObj import CORBATestObj

import CORBACommons__POA

ObjectKey   = "ProcessControl"

class ProcessControlObj(CORBATestObj):

  def __init__( self, test):
    CORBATestObj.__init__( self, ObjectKey,
                           CORBACommons__POA.IProcessControl,
                           test)

class ProbeObjMixin(ComparisonMixin):
  
  IProcessControl = IProcessControl

  def setUp( self ):
    self.status = AProcessControl.AS_READY
    self.probeObj = ProcessControlObj(self)
    self.bindObject(ObjectKey, self.probeObj)
  
  def getProcessControl( self ):
    return self.getObject(ObjectKey, IProcessControl)

  def setReady( self ):
    self.status = AProcessControl.AS_READY
  
  def setNotAlive( self ):
    self.status = AProcessControl.AS_NOT_ALIVE
  
  def setAlive( self ):
    self.status = AProcessControl.AS_ALIVE
  
  def ProcessControl_is_alive( self ):
    tlog(10, "ProcessControl.is_alive()")
    return self.status
  
  def ProcessControl_shutdown( self, wait_for_completion ):
    tlog(10, "ProcessControl.shutdown(wait_for_completion=%d)" % \
         wait_for_completion)
    return

  def ProcessControl_comment( self ):
    tlog(10, "ProcessControl.comment()")
    return self.__class__.__name__

  def ProcessControl_control( self, param_name, param_value ):
    tlog(10, "ProcessControl.control(param_name=%s, param_value=%s)" % \
         (param_name, param_value))
    return 'ok'


