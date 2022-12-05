#!/usr/bin/env python

import time
from CORBATest import *
from FunTest import tlog
from Util import currentTime, time2str
from OrbUtil import time2orb, orb2time
from OrbTestSuite import main
from SingleThread import SingleThread
                              
from ProbeObj import ProbeObjMixin

class ProbeObject(CORBAFunTest, ProbeObjMixin):
  'ProbeObject'

  def setUp( self ):
    self.LOGLEVEL = 100
    ProbeObjMixin.setUp( self )
  
  def tearDown( self ):
    pass
  
  def run( self ):
    res = self.test(1, self.testProbeObject)
  
  def testProbeObject( self ):
    "ProbeObject - ProcessControl"
    pc = self.getProcessControl()
    self.assert_(pc.is_alive(), 'ProcessControl started')
    pc.comment()
    pc.control('a', 'b')
    pc.shutdown(True)
    self.checkCallSequence(expCalls = ['ProcessControl_is_alive',
                                       'ProcessControl_comment',
                                       'ProcessControl_control',
                                       'ProcessControl_shutdown'])
  
if __name__ == '__main__':
 main()
