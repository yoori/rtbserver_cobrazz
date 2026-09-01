#!/usr/bin/python3.12

import argparse
import json
import pathlib
import sys


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from ai_agent.WebTools import WebTools
from ai_agent.WebTools import serialize_tool_result


TOOLS = [
  {
    'name': 'web_search',
    'description': (
      'Search the public web. Returns structured result URLs, titles, and '
      'snippets. Use it to discover external evidence before opening pages; '
      'do not treat snippets alone as verified evidence.'),
    'inputSchema': {
      'type': 'object',
      'properties': {
        'query': {'type': 'string', 'minLength': 1, 'maxLength': 400},
        'count': {'type': 'integer', 'minimum': 1, 'maximum': 20},
        'region': {
          'type': 'string',
          'description': 'DuckDuckGo region, for example ru-ru or wt-wt.',
        },
        'domains': {
          'type': 'array',
          'items': {'type': 'string'},
          'maxItems': 10,
        },
      },
      'required': ['query'],
      'additionalProperties': False,
    },
    'annotations': {
      'readOnlyHint': True,
      'destructiveHint': False,
      'idempotentHint': True,
      'openWorldHint': True,
    },
  },
  {
    'name': 'web_fetch',
    'description': (
      'Fetch a public HTTP or HTTPS page and return its final URL, status, '
      'title, and normalized visible text. Use the browser for pages that '
      'require JavaScript, and search for another source if fetching fails.'),
    'inputSchema': {
      'type': 'object',
      'properties': {
        'url': {'type': 'string'},
        'max_chars': {'type': 'integer', 'minimum': 1000},
      },
      'required': ['url'],
      'additionalProperties': False,
    },
    'annotations': {
      'readOnlyHint': True,
      'destructiveHint': False,
      'idempotentHint': True,
      'openWorldHint': True,
    },
  },
]


def split_list(value):
  return tuple(item.strip() for item in value.split(',') if item.strip())


def write_message(message):
  sys.stdout.write(json.dumps(message, separators=(',', ':')) + '\n')
  sys.stdout.flush()


def tool_error(error):
  return {
    'content': [{
      'type': 'text',
      'text': str(error),
    }],
    'isError': True,
  }


def run():
  parser = argparse.ArgumentParser(description='Read-only web tools MCP server.')
  parser.add_argument('--proxy-server', default='')
  parser.add_argument('--request-timeout-ms', type=int, default=30000)
  parser.add_argument('--max-download-bytes', type=int, default=2 * 1024 * 1024)
  parser.add_argument('--max-content-chars', type=int, default=100000)
  parser.add_argument('--allowed-domains', default='')
  args = parser.parse_args()
  web_tools = WebTools(
    proxy_server=args.proxy_server,
    request_timeout=args.request_timeout_ms / 1000.0,
    max_download_bytes=args.max_download_bytes,
    max_content_chars=args.max_content_chars,
    allowed_domains=split_list(args.allowed_domains))

  for line in sys.stdin:
    try:
      request = json.loads(line)
      request_id = request.get('id')
      method = request.get('method')
      if request_id is None:
        continue
      if method == 'initialize':
        result = {
          'protocolVersion': '2025-06-18',
          'capabilities': {'tools': {}},
          'serverInfo': {'name': 'foros-web-tools', 'version': '1.0'},
        }
      elif method == 'tools/list':
        result = {'tools': TOOLS}
      elif method == 'tools/call':
        params = request.get('params') or {}
        arguments = params.get('arguments') or {}
        try:
          if params.get('name') == 'web_search':
            result = serialize_tool_result(web_tools.search(**arguments))
          elif params.get('name') == 'web_fetch':
            result = serialize_tool_result(web_tools.fetch(**arguments))
          else:
            raise ValueError('unknown tool: ' + str(params.get('name')))
        except Exception as error:
          result = tool_error(error)
      else:
        write_message({
          'jsonrpc': '2.0',
          'id': request_id,
          'error': {'code': -32601, 'message': 'Method not supported'},
        })
        continue
      write_message({'jsonrpc': '2.0', 'id': request_id, 'result': result})
    except Exception as error:
      print('WebToolsMcp request failed: ' + str(error), file=sys.stderr)


if __name__ == '__main__':
  run()
