
from ProxyObj import ProxyObj

def MixIn(testClass, corbaClass):
  if corbaClass not in testClass.__bases__:
    testClass.__bases__ += (corbaClass,)

class CORBATestObj(ProxyObj):

  def __init__( self, name, base, test):
    MixIn(self.__class__, base)
    self.__name = name
    self.__test = test

  def __getattr__( self, name ):
    if name and name[0] == '_':
      return getattr(self.__test, name)
    return self.ProxyMethod(self, name)

  def _proxyObjCall( self, method, *args, **kw ):
    methodName = "%s_%s" % (self.__name, method)
    fn = getattr(self.__test, methodName)
    assert callable(fn), fn
    self.__test.addCall(methodName)
    return fn(*args, **kw)





