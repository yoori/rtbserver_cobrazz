import signal


class SignalInterruptHandler(object):
  def __init__(self, sigs = [ signal.SIGINT ], handler = None):
    self._sigs = sigs
    self._handlers = []
    if handler is not None :
      self._handlers.append(handler)

  def add_handler(self, handler):
    self._handlers.append(handler)
    
  def __enter__(self):
    self._interrupted = False
    self._released = False
    self._original_handlers = {}
    for sig in self._sigs :
      self._original_handlers[sig] = signal.getsignal(sig)

    def handler(signum, frame):
      self.release()
      self._interrupted = True

      for handler in self._handlers :
        try :
          handler(signum, frame)
        except Exception as e :
          print("SignalInterruptHandler: error on handler call: " + str(e))
          pass

    for sig in self._sigs :
      signal.signal(sig, handler)
    return self

  def interrupted(self) -> bool :
    return self._interrupted

  def __exit__(self, type, value, tb):
    self.release()

  def release(self):
    if self._released:
      return False

    for sig, original_handler in self._original_handlers.items() :
      signal.signal(sig, original_handler)
    self._released = True

    return True
