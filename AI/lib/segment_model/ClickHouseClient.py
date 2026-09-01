import time
import urllib.error
import urllib.parse
import urllib.request


class ClickHouseClient:
  def __init__(self, base_url, user='default', password='', timeout=30.0):
    self.base_url = base_url.rstrip('/')
    self.user = user
    self.password = password
    self.timeout = timeout

  def execute(self, query, data=None):
    parameters = urllib.parse.urlencode({'query': query.strip()})
    request = urllib.request.Request(
      self.base_url + '/?' + parameters,
      data=data if data is not None else b'',
      method='POST')
    request.add_header('X-ClickHouse-User', self.user)
    if self.password:
      request.add_header('X-ClickHouse-Key', self.password)
    try:
      with urllib.request.urlopen(request, timeout=self.timeout) as response:
        return response.read()
    except urllib.error.HTTPError as error:
      message = error.read().decode('utf-8', errors='replace')
      raise RuntimeError('ClickHouse request failed: ' + message.strip()) from error

  def wait_until_ready(self, timeout=120.0):
    deadline = time.monotonic() + timeout
    last_error = None
    while time.monotonic() < deadline:
      try:
        if self.execute('SELECT 1').strip() == b'1':
          return
      except (OSError, RuntimeError) as error:
        last_error = error
      time.sleep(1.0)
    raise RuntimeError('ClickHouse did not become ready') from last_error
