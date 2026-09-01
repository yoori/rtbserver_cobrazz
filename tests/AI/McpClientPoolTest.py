#!/usr/bin/python3.12

import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from ai_agent.McpClient import McpError
from ai_agent.McpClientPool import McpClientPool


class FakeClient:
  def __init__(self, tool_names):
    self.tool_names = tool_names
    self.calls = []
    self.closed = False

  def list_tools(self):
    return [{'name': name} for name in self.tool_names]

  def call_tool(self, name, arguments):
    self.calls.append((name, arguments))
    return {'server': self.tool_names[0]}

  def close(self):
    self.closed = True


class McpClientPoolTest(unittest.TestCase):
  def test_routes_tools_to_owning_clients(self):
    browser = FakeClient(('browser_navigate',))
    web = FakeClient(('web_search', 'web_fetch'))
    pool = McpClientPool((browser, web))

    self.assertEqual(
      ['browser_navigate', 'web_search', 'web_fetch'],
      [tool['name'] for tool in pool.list_tools()])
    self.assertEqual(
      {'server': 'web_search'},
      pool.call_tool('web_fetch', {'url': 'https://example.com'}))
    self.assertEqual(
      [('web_fetch', {'url': 'https://example.com'})],
      web.calls)
    self.assertEqual([], browser.calls)

  def test_rejects_duplicate_tool_names(self):
    first = FakeClient(('web_search',))
    second = FakeClient(('web_search',))
    pool = McpClientPool((first, second))

    with self.assertRaisesRegex(McpError, 'duplicate'):
      pool.list_tools()
    self.assertTrue(first.closed)
    self.assertTrue(second.closed)


if __name__ == '__main__':
  unittest.main()
