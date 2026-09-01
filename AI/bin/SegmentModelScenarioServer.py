#!/usr/bin/python3.12

import argparse
import datetime
import json
import multiprocessing
import os
import pathlib
import signal
import sys
from http.server import BaseHTTPRequestHandler
from http.server import ThreadingHTTPServer


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from segment_model.ScenarioDefinition import SegmentModelScenario


class RequestStatistics:
  def __init__(self):
    self.lock = multiprocessing.Lock()
    self.profile_requests = multiprocessing.RawValue('Q', 0)
    self.failed_requests = multiprocessing.RawValue('Q', 0)

  def increment(self, failed=False):
    with self.lock:
      self.profile_requests.value += 1
      self.failed_requests.value += int(failed)

  def to_dict(self):
    with self.lock:
      return {
        'profile_requests': self.profile_requests.value,
        'failed_requests': self.failed_requests.value,
      }


def make_handler(scenario, statistics):
  class ScenarioRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
      if self.path == '/health':
        self._write_json({'status': 'ready'})
        return
      if self.path == '/stats':
        self._write_json(statistics.to_dict())
        return
      self._write_json({'error': 'not found'}, 404)

    def do_POST(self):
      if self.path != '/get_user_navigation_profile':
        self._write_json({'error': 'not found'}, 404)
        return
      try:
        content_length = int(self.headers.get('Content-Length', '0'))
        request = json.loads(self.rfile.read(content_length))
        user_ids = request['user_ids']
        if not isinstance(user_ids, list) or not user_ids:
          raise ValueError('user_ids must be a non-empty array')
        cutoff = _parse_date(request['date'])
        profiles = [_profile(scenario, user_id, cutoff) for user_id in user_ids]
        for _ in user_ids:
          statistics.increment()
        self._write_json({'profiles': profiles})
      except (KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        statistics.increment(failed=True)
        self._write_json({'error': str(error)}, 400)

    def log_message(self, format_string, *args):
      del format_string
      del args

    def _write_json(self, value, status=200):
      body = json.dumps(value, separators=(',', ':')).encode('utf-8')
      self.send_response(status)
      self.send_header('Content-Type', 'application/json')
      self.send_header('Content-Length', str(len(body)))
      self.end_headers()
      self.wfile.write(body)

  return ScenarioRequestHandler


def _parse_date(value):
  if isinstance(value, int):
    timestamp = value
  elif isinstance(value, str):
    parsed = datetime.datetime.strptime(value, '%Y-%m-%d')
    timestamp = int(parsed.replace(tzinfo=datetime.timezone.utc).timestamp())
  else:
    raise ValueError('date must be a YYYY-MM-DD string or Unix timestamp')
  if timestamp < 0:
    raise ValueError('date must not be negative')
  return timestamp - timestamp % 86400


def _profile(scenario, user_id, cutoff):
  numeric_uid = scenario.numeric_uid(user_id)
  sample = scenario.sample(numeric_uid, cutoff)
  counts = {}
  for navigation in sample['navigations']:
    navigation_date = navigation['timestamp'] - navigation['timestamp'] % 86400
    if navigation_date > cutoff:
      continue
    key = (navigation_date, navigation['url'])
    counts[key] = counts.get(key, 0) + 1
  navigations = [
    {
      'date': datetime.datetime.fromtimestamp(
        navigation_date,
        datetime.timezone.utc).strftime('%Y-%m-%d'),
      'url': url,
      'count': count,
    }
    for (navigation_date, url), count in sorted(counts.items())
  ]
  return {
    'user_id': user_id,
    'found': True,
    'cohort': sample['cohort_name'],
    'variant_id': sample['variant_id'],
    'navigations': navigations,
  }


def main():
  parser = argparse.ArgumentParser(description='Run an ExpressionMatcher scenario mock.')
  parser.add_argument('--scenario', required=True)
  parser.add_argument('--host', default='0.0.0.0')
  parser.add_argument('--port', type=int, default=8080)
  parser.add_argument('--processes', type=int, default=1)
  args = parser.parse_args()
  if args.processes <= 0:
    parser.error('--processes must be positive')
  scenario = SegmentModelScenario.from_json(args.scenario)
  statistics = RequestStatistics()
  server = ThreadingHTTPServer((args.host, args.port), make_handler(scenario, statistics))
  server.daemon_threads = True
  print(
    'ExpressionMatcher scenario server listens on port ' + str(args.port) +
    ' with ' + str(args.processes) + ' processes',
    flush=True)
  _serve(server, args.processes)


def _serve(server, processes):
  children = []
  for _ in range(processes - 1):
    child = os.fork()
    if child == 0:
      try:
        server.serve_forever()
      finally:
        server.server_close()
      os._exit(0)
    children.append(child)
  try:
    server.serve_forever()
  except KeyboardInterrupt:
    pass
  finally:
    server.server_close()
    for child in children:
      try:
        os.kill(child, signal.SIGTERM)
      except ProcessLookupError:
        pass
    for child in children:
      try:
        os.waitpid(child, 0)
      except ChildProcessError:
        pass


if __name__ == '__main__':
  main()
