#!/usr/bin/python3.12

import io
import json
import pathlib
import sys
import unittest
from unittest import mock


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.ExpressionMatcherClient import ExpressionMatcherClient


class ExpressionMatcherClientTest(unittest.TestCase):
  def test_posts_user_and_impression_date_using_real_http_contract(self):
    response = io.BytesIO(json.dumps({
      'profiles': [
        {
          'user_id': 'user-7',
          'found': True,
          'navigations': [
            {'date': '2026-01-01', 'url': 'a.com', 'count': 2},
          ],
        },
      ],
    }).encode('utf-8'))
    timestamp = 1767375000
    with mock.patch('urllib.request.urlopen', return_value=response) as urlopen:
      navigations = ExpressionMatcherClient('http://expression-matcher').profile(
        'user-7',
        timestamp)
    request = urlopen.call_args.args[0]
    self.assertEqual(
      'http://expression-matcher/get_user_navigation_profile',
      request.full_url)
    self.assertEqual('POST', request.get_method())
    self.assertEqual({
      'user_ids': ['user-7'],
      'date': '2026-01-02',
    }, json.loads(request.data))
    self.assertEqual(2, navigations[0]['count'])

  def test_fans_out_all_users_to_all_hosts_and_merges_only_found_profiles(self):
    responses = {
      'http://em-1/get_user_navigation_profile': {
        'profiles': [
          {'user_id': 'user-1', 'navigations': [{'date': '2026-01-01', 'url': 'a'}]},
        ],
      },
      'http://em-2/get_user_navigation_profile': {
        'profiles': [
          {'user_id': 'user-2', 'navigations': [{'date': '2026-01-01', 'url': 'b'}]},
        ],
      },
    }

    def open_request(request, timeout):
      del timeout
      return io.BytesIO(json.dumps(responses[request.full_url]).encode('utf-8'))

    with mock.patch('urllib.request.urlopen', side_effect=open_request) as urlopen:
      profiles = ExpressionMatcherClient(['http://em-1', 'http://em-2']).profiles(
        ['user-1', 'user-2', 'user-3'],
        1767375000)
    self.assertEqual(['a'], [item['url'] for item in profiles['user-1']])
    self.assertEqual(['b'], [item['url'] for item in profiles['user-2']])
    self.assertEqual([], profiles['user-3'])
    self.assertEqual(2, urlopen.call_count)
    for call in urlopen.call_args_list:
      self.assertEqual({
        'user_ids': ['user-1', 'user-2', 'user-3'],
        'date': '2026-01-02',
      }, json.loads(call.args[0].data))

  def test_rejects_a_user_found_on_multiple_hosts(self):
    response = json.dumps({
      'profiles': [{'user_id': 'user-1', 'navigations': []}],
    }).encode('utf-8')
    with mock.patch(
        'urllib.request.urlopen',
        side_effect=lambda request, timeout: io.BytesIO(response)):
      with self.assertRaisesRegex(RuntimeError, 'multiple hosts'):
        ExpressionMatcherClient(['http://em-1', 'http://em-2']).profiles(
          ['user-1'],
          1767375000)


if __name__ == '__main__':
  unittest.main()
