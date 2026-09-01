import json
import urllib.error
import urllib.request


class OllamaError(RuntimeError):
  pass


class OllamaClient:
  def __init__(self, base_url, timeout=300.0):
    self.base_url = base_url.rstrip('/')
    self.timeout = timeout

  def chat(self, request):
    body = json.dumps(request, separators=(',', ':')).encode('utf-8')
    http_request = urllib.request.Request(
      self.base_url + '/api/chat',
      data=body,
      headers={'Content-Type': 'application/json'},
      method='POST')
    try:
      with urllib.request.urlopen(
          http_request,
          timeout=self.timeout) as response:
        return json.load(response)
    except urllib.error.HTTPError as error:
      response_body = error.read().decode('utf-8', errors='replace')
      raise OllamaError(
        'Ollama returned HTTP ' + str(error.code) + ': ' + response_body) from error
    except (OSError, ValueError) as error:
      raise OllamaError('Ollama request failed: ' + str(error)) from error
