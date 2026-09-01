import json
import selectors
import subprocess
import threading


class McpError(RuntimeError):
  pass


class McpClient:
  PROTOCOL_VERSION = '2025-06-18'

  def __init__(
      self,
      command,
      arguments,
      request_timeout=90.0,
      server_name='MCP'):
    self.command = command
    self.arguments = tuple(arguments)
    self.request_timeout = request_timeout
    self.server_name = server_name
    self.process = None
    self.next_request_id = 1
    self.tools = []
    self.stderr_thread = None

  def start(self):
    if self.process is not None and self.process.poll() is None:
      return
    self.close()
    self.process = subprocess.Popen(
      [self.command, *self.arguments],
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
      encoding='utf-8',
      bufsize=1)
    self.stderr_thread = threading.Thread(
      target=self._copy_stderr,
      args=(self.process, self.server_name),
      daemon=True)
    self.stderr_thread.start()
    try:
      self._request('initialize', {
        'protocolVersion': self.PROTOCOL_VERSION,
        'capabilities': {},
        'clientInfo': {
          'name': 'foros-ai-agent',
          'version': '1.0',
        },
      })
      self._notification('notifications/initialized')
      self.tools = self._request('tools/list').get('tools', [])
    except Exception:
      self.close()
      raise

  def list_tools(self):
    self.start()
    return list(self.tools)

  def call_tool(self, name, arguments):
    self.start()
    return self._request('tools/call', {
      'name': name,
      'arguments': arguments,
    })

  def close(self):
    process = self.process
    self.process = None
    self.tools = []
    if process is None:
      return
    try:
      if process.stdin:
        process.stdin.close()
    except OSError:
      pass
    if process.poll() is None:
      process.terminate()
      try:
        process.wait(timeout=5)
      except subprocess.TimeoutExpired:
        process.kill()
        process.wait()

  def _request(self, method, params=None):
    request_id = self.next_request_id
    self.next_request_id += 1
    message = {
      'jsonrpc': '2.0',
      'id': request_id,
      'method': method,
    }
    if params is not None:
      message['params'] = params
    self._write(message)

    while True:
      response = self._read()
      if response.get('id') != request_id:
        if 'method' in response and 'id' in response:
          self._write({
            'jsonrpc': '2.0',
            'id': response['id'],
            'error': {'code': -32601, 'message': 'Method not supported'},
          })
        continue
      if 'error' in response:
        error = response['error']
        raise McpError(
          method + ' failed: ' + str(error.get('message', error)))
      return response.get('result', {})

  def _notification(self, method, params=None):
    message = {
      'jsonrpc': '2.0',
      'method': method,
    }
    if params is not None:
      message['params'] = params
    self._write(message)

  def _write(self, message):
    if self.process is None or self.process.poll() is not None:
      raise McpError('MCP server is not running')
    try:
      self.process.stdin.write(
        json.dumps(message, separators=(',', ':')) + '\n')
      self.process.stdin.flush()
    except (BrokenPipeError, OSError) as error:
      raise McpError('Cannot write to MCP server: ' + str(error)) from error

  def _read(self):
    if self.process is None:
      raise McpError('MCP server is not running')
    selector = selectors.DefaultSelector()
    try:
      selector.register(self.process.stdout, selectors.EVENT_READ)
      if not selector.select(self.request_timeout):
        raise McpError('MCP response timeout')
      line = self.process.stdout.readline()
    finally:
      selector.close()
    if not line:
      return_code = self.process.poll()
      raise McpError(
        'MCP server closed stdout, return code=' + str(return_code))
    try:
      return json.loads(line)
    except json.JSONDecodeError as error:
      raise McpError('Invalid MCP response: ' + line.rstrip()) from error

  @staticmethod
  def _copy_stderr(process, server_name):
    for line in process.stderr:
      print(server_name + ': ' + line.rstrip(), flush=True)
