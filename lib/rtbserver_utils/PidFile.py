import errno
import fcntl
import os
import pathlib


class PidFile:
  def __init__(self, file_name, service_name='service'):
    self.path = pathlib.Path(file_name)
    self.service_name = service_name
    self.file = None

  def __enter__(self):
    self.path.parent.mkdir(parents=True, exist_ok=True)
    self.file = self.path.open('a+')
    try:
      fcntl.flock(self.file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError as error:
      self.file.close()
      self.file = None
      if error.errno in (errno.EACCES, errno.EAGAIN):
        raise RuntimeError(
          'Another ' + self.service_name +
          ' instance is already running: ' + str(self.path))
      raise

    self.file.seek(0)
    self.file.truncate()
    self.file.write(str(os.getpid()) + '\n')
    self.file.flush()
    return self

  def __exit__(self, exception_type, exception_value, traceback):
    if self.file is None:
      return
    try:
      try:
        self.path.unlink()
      except FileNotFoundError:
        pass
      fcntl.flock(self.file.fileno(), fcntl.LOCK_UN)
    finally:
      self.file.close()
      self.file = None
