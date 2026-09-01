import html.parser
import json
import re
import urllib.error
import urllib.parse
import urllib.request

from .UrlValidator import PublicUrlValidator


USER_AGENT = (
  'Mozilla/5.0 (X11; Linux x86_64) '
  'AppleWebKit/537.36 (KHTML, like Gecko) '
  'Chrome/124.0 Safari/537.36')


def _class_tokens(attributes):
  return set(dict(attributes).get('class', '').split())


def _clean_text(value):
  return ' '.join(value.split())


def _result_url(href):
  url = urllib.parse.urljoin('https://duckduckgo.com/', href)
  parsed = urllib.parse.urlsplit(url)
  redirect_url = urllib.parse.parse_qs(parsed.query).get('uddg')
  return redirect_url[0] if redirect_url else url


class DuckDuckGoResultParser(html.parser.HTMLParser):
  def __init__(self):
    super().__init__(convert_charrefs=True)
    self.results = []
    self.capture = None
    self.capture_parts = []
    self.capture_url = ''

  def handle_starttag(self, tag, attributes):
    if tag != 'a':
      return
    classes = _class_tokens(attributes)
    if 'result__a' in classes:
      self.capture = 'title'
      self.capture_parts = []
      self.capture_url = _result_url(dict(attributes).get('href', ''))
    elif 'result__snippet' in classes and self.results:
      self.capture = 'snippet'
      self.capture_parts = []

  def handle_data(self, data):
    if self.capture:
      self.capture_parts.append(data)

  def handle_endtag(self, tag):
    if tag != 'a' or not self.capture:
      return
    value = _clean_text(''.join(self.capture_parts))
    if self.capture == 'title' and self.capture_url:
      self.results.append({
        'title': value,
        'url': self.capture_url,
        'snippet': '',
      })
    elif self.capture == 'snippet' and self.results:
      self.results[-1]['snippet'] = value
    self.capture = None
    self.capture_parts = []
    self.capture_url = ''


class PageTextParser(html.parser.HTMLParser):
  BLOCK_TAGS = frozenset((
    'article', 'aside', 'blockquote', 'br', 'dd', 'div', 'dl', 'dt',
    'figcaption', 'footer', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6',
    'header', 'li', 'main', 'nav', 'ol', 'p', 'pre', 'section',
    'table', 'td', 'th', 'tr', 'ul'))
  IGNORED_TAGS = frozenset(('canvas', 'noscript', 'script', 'style', 'svg'))

  def __init__(self):
    super().__init__(convert_charrefs=True)
    self.ignored_depth = 0
    self.in_title = False
    self.title_parts = []
    self.text_parts = []

  def handle_starttag(self, tag, attributes):
    del attributes
    if tag in self.IGNORED_TAGS:
      self.ignored_depth += 1
      return
    if self.ignored_depth:
      return
    if tag == 'title':
      self.in_title = True
    if tag in self.BLOCK_TAGS:
      self.text_parts.append('\n')

  def handle_endtag(self, tag):
    if tag in self.IGNORED_TAGS:
      if self.ignored_depth:
        self.ignored_depth -= 1
      return
    if self.ignored_depth:
      return
    if tag == 'title':
      self.in_title = False
    if tag in self.BLOCK_TAGS:
      self.text_parts.append('\n')

  def handle_data(self, data):
    if self.ignored_depth:
      return
    if self.in_title:
      self.title_parts.append(data)
    self.text_parts.append(data)

  def title(self):
    return _clean_text(''.join(self.title_parts))

  def text(self):
    value = ''.join(self.text_parts)
    lines = (_clean_text(line) for line in value.splitlines())
    return '\n'.join(line for line in lines if line)


class ValidatingRedirectHandler(urllib.request.HTTPRedirectHandler):
  def __init__(self, validator):
    super().__init__()
    self.validator = validator

  def redirect_request(self, request, file_pointer, code, message, headers, url):
    self.validator.validate(url)
    return super().redirect_request(
      request, file_pointer, code, message, headers, url)


class WebTools:
  def __init__(
      self,
      proxy_server='',
      request_timeout=30.0,
      max_download_bytes=2 * 1024 * 1024,
      max_content_chars=100000,
      allowed_domains=(),
      resolve_host=None):
    self.request_timeout = request_timeout
    self.max_download_bytes = max_download_bytes
    self.max_content_chars = max_content_chars
    validator_args = {'allowed_domains': allowed_domains}
    if resolve_host is not None:
      validator_args['resolve_host'] = resolve_host
    self.validator = PublicUrlValidator(**validator_args)
    handlers = [ValidatingRedirectHandler(self.validator)]
    if proxy_server:
      handlers.insert(0, urllib.request.ProxyHandler({
        'http': proxy_server,
        'https': proxy_server,
      }))
    self.opener = urllib.request.build_opener(*handlers)

  def search(self, query, count=10, region='wt-wt', domains=()):
    if not isinstance(query, str) or not query.strip():
      raise ValueError('query must be a non-empty string')
    if len(query) > 400:
      raise ValueError('query is too long')
    if not isinstance(count, int) or isinstance(count, bool) or not 1 <= count <= 20:
      raise ValueError('count must be between 1 and 20')
    if not isinstance(region, str) or len(region) > 16:
      raise ValueError('invalid region')
    if not isinstance(domains, (list, tuple)) or len(domains) > 10:
      raise ValueError('domains must be an array with at most 10 items')
    checked_domains = []
    for domain in domains:
      if not isinstance(domain, str) or not re.fullmatch(
          r'[A-Za-z0-9.-]+', domain):
        raise ValueError('invalid search domain')
      checked_domains.append(domain)

    effective_query = query.strip()
    if checked_domains:
      domain_query = ' OR '.join('site:' + domain for domain in checked_domains)
      effective_query += ' (' + domain_query + ')'
    url = 'https://html.duckduckgo.com/html/?' + urllib.parse.urlencode({
      'q': effective_query,
      'kl': region,
    })
    body, final_url, status, content_type, unused = self._download(
      url, validate=False)
    del unused
    parser = DuckDuckGoResultParser()
    parser.feed(self._decode(body, content_type))

    best_results = {}
    order = []
    for result in parser.results:
      result_url = result['url']
      if result_url not in best_results:
        best_results[result_url] = result
        order.append(result_url)
      elif self._result_score(result) > self._result_score(
          best_results[result_url]):
        best_results[result_url] = result
    results = [best_results[url] for url in order[:count]]
    return {
      'provider': 'duckduckgo',
      'query': query.strip(),
      'status': status,
      'search_url': final_url,
      'results': results,
    }

  def fetch(self, url, max_chars=None):
    self.validator.validate(url)
    if max_chars is None:
      max_chars = self.max_content_chars
    if not isinstance(max_chars, int) or isinstance(max_chars, bool):
      raise ValueError('max_chars must be an integer')
    max_chars = min(max(max_chars, 1000), self.max_content_chars)
    body, final_url, status, content_type, download_truncated = self._download(
      url, validate=True)
    decoded = self._decode(body, content_type)
    media_type = content_type.split(';', 1)[0].strip().lower()
    title = ''
    if media_type in ('text/html', 'application/xhtml+xml'):
      parser = PageTextParser()
      parser.feed(decoded)
      title = parser.title()
      text = parser.text()
    elif media_type.startswith('text/') or media_type in (
        'application/json', 'application/xml'):
      text = decoded
    else:
      text = ''
    truncated = len(text) > max_chars or download_truncated
    return {
      'url': url,
      'final_url': final_url,
      'status': status,
      'content_type': content_type,
      'title': title,
      'text': text[:max_chars],
      'truncated': truncated,
    }

  def _download(self, url, validate):
    if validate:
      self.validator.validate(url)
    request = urllib.request.Request(url, headers={
      'Accept': 'text/html,application/xhtml+xml,text/plain,application/json',
      'User-Agent': USER_AGENT,
    })
    try:
      response = self.opener.open(request, timeout=self.request_timeout)
    except urllib.error.HTTPError as error:
      response = error
    with response:
      body = response.read(self.max_download_bytes + 1)
      download_truncated = len(body) > self.max_download_bytes
      if download_truncated:
        body = body[:self.max_download_bytes]
      return (
        body,
        response.geturl(),
        response.getcode(),
        response.headers.get('Content-Type', 'application/octet-stream'),
        download_truncated)

  @staticmethod
  def _decode(body, content_type):
    charset = 'utf-8'
    for part in content_type.split(';')[1:]:
      name, separator, value = part.strip().partition('=')
      if separator and name.lower() == 'charset':
        charset = value.strip(' \"\'')
        break
    try:
      return body.decode(charset, errors='replace')
    except LookupError:
      return body.decode('utf-8', errors='replace')

  @staticmethod
  def _result_score(result):
    return len(result.get('title', '')) + len(result.get('snippet', ''))


def serialize_tool_result(value):
  return {
    'content': [{
      'type': 'text',
      'text': json.dumps(value, ensure_ascii=False, separators=(',', ':')),
    }],
    'structuredContent': value,
  }
