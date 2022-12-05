
import FunTest, sys, os
from omniORB import CORBA

import ConfigVars

class OrbTestInfo(FunTest.TestInfo):

  def __init__( self, prefix, workDir, testLevel, tmpDir, logLevel, result ):
    FunTest.TestInfo.__init__( self, prefix, workDir, testLevel, tmpDir, logLevel, result )
    self.orb = None

class OrbTestSuite (FunTest.TestSuite):
  
  def __init__( self, info, children = None ):
    assert isinstance(info, OrbTestInfo)
    FunTest.TestSuite.__init__(self, info, children)
    self.orbPort = ConfigVars.PORT_BASE + 99
    self.initOrb()
    self.activeCORBAObjects = []

  def isRoot( self ):
    return self.__class__ is OrbTestSuite

  def closeOrb( self ):
    if self.info.orb:
      self.info.orb.destroy()
      self.info.orb = None

  def initOrb( self ):
    if self.info.orb: self.info.orb.destroy()
    assert "-ORBendPoint" not in sys.argv, "You shouldn't set endpoint manually"
    sys.argv.extend(["-ORBendPoint", 'giop:tcp::%d' % self.orbPort,
                     "-ORBgiopMaxMsgSize", '4194304'])
    self.info.orb = CORBA.ORB_init(sys.argv, CORBA.ORB_ID)
    poa = self.info.orb.resolve_initial_references("omniINSPOA")
    poa._get_the_POAManager().activate()

  def bindObject( self, objectKey, servant ):
    poa = self.info.orb.resolve_initial_references("omniINSPOA")
    poa.activate_object_with_id(objectKey, servant)
    obj = poa.servant_to_reference(servant)
    self.activeCORBAObjects.append(objectKey)
    ior =  self.info.orb.object_to_string(obj)
    FunTest.tlog(5, "Server '%s' ior: %s" % (objectKey, ior))
    return ior

  def deactivateObjects( self ):
    poa = self.info.orb.resolve_initial_references("omniINSPOA")
    for objectKey in self.activeCORBAObjects:
      poa.deactivate_object(objectKey)

  def getObject( self, objectKey, objectType ):
    ior = "corbaloc:iiop:%s:%s/%s" % (os.environ['HOSTNAME'], self.orbPort, objectKey)
    obj = self.info.orb.string_to_object(ior)
    return obj._narrow(objectType) 

def makeTestInfo( workDir  = '.' ):
  ti = FunTest.makeTestInfo( workDir )
  return OrbTestInfo(ti.prefix, ti.workDir, ti.testLevel,
                     ti.tmpDir, ti.logLevel, ti.result)


def runTests( info, root ):
  FunTest.runTests( info, root )
  if info.orb: info.orb.destroy()

def main():
  info = makeTestInfo()
  suites = FunTest.collectSuites(info, __import__('__main__'))
  root = OrbTestSuite(info, suites)
  runTests(info, root)



