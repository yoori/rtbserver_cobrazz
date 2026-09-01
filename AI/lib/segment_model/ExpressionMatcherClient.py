import concurrent.futures
import datetime
import json
import time
import urllib.error
import urllib.request


class ExpressionMatcherClient:
  def __init__(self, base_urls, timeout=10.0, retries=3, workers=16):
    if isinstance(base_urls, str):
      base_urls = [base_urls]
    self.base_urls = tuple(url.rstrip('/') for url in base_urls)
    if not self.base_urls:
      raise ValueError('at least one ExpressionMatcher URL is required')
    if workers <= 0:
      raise ValueError('ExpressionMatcher worker count must be positive')
    self.timeout = timeout
    self.retries = retries
    self.workers = workers

  def profile(self, user_id, timestamp):
    return self.profiles([user_id], timestamp)[user_id]

  def profiles(self, user_ids, timestamp):
    user_ids = tuple(dict.fromkeys(user_ids))
    if not user_ids:
      return {}
    date = datetime.datetime.fromtimestamp(timestamp, datetime.timezone.utc).strftime('%Y-%m-%d')
    body = json.dumps({
      'user_ids': user_ids,
      'date': date,
    }, separators=(',', ':')).encode('utf-8')
    requests = [self._make_request(base_url, body) for base_url in self.base_urls]
    if len(requests) == 1:
      results = [self._read_json(requests[0])]
    else:
      with concurrent.futures.ThreadPoolExecutor(
          max_workers=min(self.workers, len(requests))) as executor:
        results = list(executor.map(self._read_json, requests))
    requested = set(user_ids)
    found = {}
    for result in results:
      profiles = result.get('profiles')
      if not isinstance(profiles, list):
        raise RuntimeError('ExpressionMatcher returned malformed profiles')
      for profile in profiles:
        user_id = profile.get('user_id')
        if user_id not in requested:
          raise RuntimeError('ExpressionMatcher returned an unrequested user')
        if not profile.get('found', True):
          continue
        if user_id in found:
          raise RuntimeError('ExpressionMatcher user was found on multiple hosts')
        navigations = profile.get('navigations', [])
        if not isinstance(navigations, list):
          raise RuntimeError('ExpressionMatcher returned malformed navigations')
        found[user_id] = navigations
    return {user_id: found.get(user_id, []) for user_id in user_ids}

  @staticmethod
  def _make_request(base_url, body):
    return urllib.request.Request(
      base_url + '/get_user_navigation_profile',
      data=body,
      headers={'Content-Type': 'application/json'},
      method='POST')

  def _read_json(self, request):
    last_error = None
    for attempt in range(self.retries + 1):
      try:
        with urllib.request.urlopen(request, timeout=self.timeout) as response:
          return json.load(response)
      except (OSError, urllib.error.HTTPError, json.JSONDecodeError) as error:
        last_error = error
        if attempt < self.retries:
          time.sleep(0.1 * (2 ** attempt))
    raise RuntimeError('ExpressionMatcher profile request failed') from last_error
