#!/usr/bin/python3.12

import pathlib
import sys
import unittest
import urllib.parse
from unittest import mock


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.ClickHouseClient import ClickHouseClient


class ClickHouseClientTest(unittest.TestCase):
  def test_strips_query_before_appending_insert_data(self):
    response = mock.MagicMock()
    response.__enter__.return_value.read.return_value = b''
    with mock.patch('urllib.request.urlopen', return_value=response) as urlopen:
      ClickHouseClient('http://clickhouse:8123').execute(
        '\nINSERT INTO RImpression FORMAT TabSeparated\n',
        b'0\tvalue\n')
    request = urlopen.call_args.args[0]
    query = urllib.parse.parse_qs(urllib.parse.urlparse(request.full_url).query)['query'][0]
    self.assertEqual('INSERT INTO RImpression FORMAT TabSeparated', query)
    self.assertEqual(b'0\tvalue\n', request.data)


if __name__ == '__main__':
  unittest.main()
