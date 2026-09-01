#!/usr/bin/python3.12

import pathlib
import sys
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from ai_agent.UrlValidator import PublicUrlValidator
from ai_agent.UrlValidator import UrlValidationError
from ai_agent.WebTools import DuckDuckGoResultParser
from ai_agent.WebTools import PageTextParser


def public_address(hostname, port):
  del hostname
  return [(2, 1, 6, '', ('93.184.216.34', port))]


def private_address(hostname, port):
  del hostname
  return [(2, 1, 6, '', ('127.0.0.1', port))]


class WebToolsTest(unittest.TestCase):
  def test_parses_search_results_and_redirect_urls(self):
    parser = DuckDuckGoResultParser()
    parser.feed('''
      <a class="result__a"
         href="//duckduckgo.com/l/?uddg=https%3A%2F%2Fexample.com%2Fdoc">
        Example document
      </a>
      <a class="result__snippet">Useful result text.</a>
    ''')

    self.assertEqual([{
      'title': 'Example document',
      'url': 'https://example.com/doc',
      'snippet': 'Useful result text.',
    }], parser.results)

  def test_extracts_visible_page_text(self):
    parser = PageTextParser()
    parser.feed('''
      <html><head><title>Example title</title><style>hidden</style></head>
      <body><h1>Heading</h1><p>Visible <b>text</b>.</p>
      <script>ignored()</script></body></html>
    ''')

    self.assertEqual('Example title', parser.title())
    self.assertIn('Heading', parser.text())
    self.assertIn('Visible text.', parser.text())
    self.assertNotIn('hidden', parser.text())
    self.assertNotIn('ignored', parser.text())

  def test_rejects_private_addresses(self):
    validator = PublicUrlValidator(resolve_host=private_address)
    with self.assertRaisesRegex(UrlValidationError, 'non-public'):
      validator.validate('https://example.com/private')

  def test_accepts_public_addresses(self):
    validator = PublicUrlValidator(resolve_host=public_address)
    self.assertEqual(
      'https://example.com/',
      validator.validate('https://example.com/'))


if __name__ == '__main__':
  unittest.main()
