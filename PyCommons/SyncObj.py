
from ProxyObj import ProxyObj
import threading


class Sync(ProxyObj):

  def __init__( self, base, mutex = None, mtSafeAttrs = None ):
    self.__base = base
    self.__mutex = mutex or threading.Lock()
    self.__mtSafeAttrs = mtSafeAttrs or []

  def __getattr__( self, name ):
    if name in self.__mtSafeAttrs:
      return getattr(self.__base, name)
    if not name or name[0] == '_':
      raise AttributeError('access to protected attribute')
    return self.ProxyMethod(self, name)

  def _proxyObjCall( self, method, *args, **kw ):
    fn = getattr(self.__base, method)
    assert callable(fn), fn  # try to deny access to data members - at least to not callable ones
    return self._syncObjCall(method, fn, *args, **kw)

  def _syncObjCall( self, name, fn, *args, **kw ):
    self.__mutex.acquire()
    try:
      return fn(*args, **kw)
    finally:
      self.__mutex.release()

  def _assertIsInstance( self, cls ):
    assert isinstance(self.__base, cls)


class ActiveSync(Sync):

  def __init__( self, base, mutex, mtSafeAttrs = None ):
    Sync.__init__(self, base, mutex, (mtSafeAttrs or []) + ['start', 'close'])


def assertIsSyncInstance( obj, cls ):
  assert isinstance(obj, Sync)
  obj._assertIsInstance(cls)
