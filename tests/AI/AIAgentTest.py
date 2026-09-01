#!/usr/bin/python3.12

import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from ai_agent.AgentService import AgentRequestError
from ai_agent.AgentService import AgentService


class FakeMcpClient:
  def __init__(self):
    self.calls = []

  def list_tools(self):
    return [
      {
        'name': 'browser_navigate',
        'description': 'Navigate to a URL',
        'inputSchema': {
          'type': 'object',
          'properties': {'url': {'type': 'string'}},
          'required': ['url'],
        },
      },
      {
        'name': 'browser_close',
        'description': 'Close browser',
        'inputSchema': {'type': 'object', 'properties': {}},
      },
      {
        'name': 'web_fetch',
        'description': 'Fetch a URL',
        'inputSchema': {
          'type': 'object',
          'properties': {'url': {'type': 'string'}},
          'required': ['url'],
        },
      },
      {
        'name': 'browser_evaluate',
        'description': 'Evaluate JavaScript',
        'inputSchema': {'type': 'object', 'properties': {}},
      },
    ]

  def call_tool(self, name, arguments):
    self.calls.append((name, arguments))
    return {
      'content': [{
        'type': 'text',
        'text': 'Example page',
      }],
    }

  def close(self):
    pass


class FakeOllamaClient:
  def __init__(self, responses):
    self.responses = list(responses)
    self.requests = []

  def chat(self, request):
    self.requests.append(request.copy())
    return self.responses.pop(0)


def public_address(hostname, port):
  del hostname
  return [(2, 1, 6, '', ('93.184.216.34', port))]


class AIAgentTest(unittest.TestCase):
  def test_browser_tool_result_is_returned_to_ollama(self):
    ollama = FakeOllamaClient([
      {
        'message': {
          'role': 'assistant',
          'content': '',
          'tool_calls': [{
            'function': {
              'name': 'browser_navigate',
              'arguments': {'url': 'https://example.com/'},
            },
          }],
        },
      },
      {
        'message': {
          'role': 'assistant',
          'content': 'The page is available.',
        },
        'done': True,
      },
    ])
    mcp = FakeMcpClient()
    service = AgentService(
      ollama,
      mcp,
      'qwen3',
      allowed_tools=('browser_navigate', 'browser_close'),
      resolve_host=public_address)

    response = service.chat({
      'messages': [{'role': 'user', 'content': 'Open example.com'}],
    })

    self.assertEqual('The page is available.', response['message']['content'])
    self.assertEqual('qwen3', ollama.requests[0]['model'])
    self.assertFalse(ollama.requests[0]['stream'])
    self.assertEqual(
      ['browser_navigate', 'browser_close'],
      [tool['function']['name'] for tool in ollama.requests[0]['tools']])
    self.assertEqual('tool', ollama.requests[1]['messages'][-1]['role'])
    self.assertIn('Example page', ollama.requests[1]['messages'][-1]['content'])
    self.assertEqual([
      ('browser_navigate', {'url': 'https://example.com/'}),
      ('browser_close', {}),
    ], mcp.calls)

  def test_private_network_url_is_rejected(self):
    ollama = FakeOllamaClient([{
      'message': {
        'role': 'assistant',
        'tool_calls': [{
          'function': {
            'name': 'browser_navigate',
            'arguments': {'url': 'http://127.0.0.1/admin'},
          },
        }],
      },
    }])
    mcp = FakeMcpClient()
    service = AgentService(
      ollama,
      mcp,
      'qwen3',
      allowed_tools=('browser_navigate', 'browser_close'))

    with self.assertRaisesRegex(AgentRequestError, 'non-public'):
      service.chat({
        'messages': [{'role': 'user', 'content': 'Open localhost'}],
      })
    self.assertEqual([('browser_close', {})], mcp.calls)

  def test_private_network_web_fetch_is_rejected(self):
    ollama = FakeOllamaClient([{
      'message': {
        'role': 'assistant',
        'tool_calls': [{
          'function': {
            'name': 'web_fetch',
            'arguments': {'url': 'http://127.0.0.1/admin'},
          },
        }],
      },
    }])
    mcp = FakeMcpClient()
    service = AgentService(
      ollama,
      mcp,
      'qwen3',
      allowed_tools=('web_fetch', 'browser_close'))

    with self.assertRaisesRegex(AgentRequestError, 'non-public'):
      service.chat({
        'messages': [{'role': 'user', 'content': 'Fetch localhost'}],
      })
    self.assertEqual([('browser_close', {})], mcp.calls)

  def test_disallowed_mcp_tools_are_not_exposed(self):
    ollama = FakeOllamaClient([])
    service = AgentService(
      ollama,
      FakeMcpClient(),
      'qwen3',
      allowed_tools=('browser_navigate',),
      resolve_host=public_address)

    self.assertEqual(
      ['browser_navigate'],
      service.health()['tools'])

  def test_streaming_request_is_rejected(self):
    service = AgentService(
      FakeOllamaClient([]),
      FakeMcpClient(),
      'qwen3')
    with self.assertRaisesRegex(AgentRequestError, 'streaming'):
      service.chat({
        'messages': [{'role': 'user', 'content': 'test'}],
        'stream': True,
      })


if __name__ == '__main__':
  unittest.main()
