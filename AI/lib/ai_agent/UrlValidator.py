import ipaddress
import socket
import urllib.parse


class UrlValidationError(ValueError):
  pass


class PublicUrlValidator:
  def __init__(
      self,
      allowed_domains=(),
      resolve_host=socket.getaddrinfo):
    self.allowed_domains = tuple(
      domain.lower().rstrip('.') for domain in allowed_domains if domain)
    self.resolve_host = resolve_host

  def validate(self, url):
    if not isinstance(url, str):
      raise UrlValidationError('URL must be a string')
    parsed = urllib.parse.urlsplit(url)
    if parsed.scheme not in ('http', 'https') or not parsed.hostname:
      raise UrlValidationError('only HTTP and HTTPS URLs are allowed')
    if parsed.username is not None or parsed.password is not None:
      raise UrlValidationError('credentials in URLs are not allowed')

    hostname = parsed.hostname.lower().rstrip('.')
    if self.allowed_domains and not any(
        hostname == domain or hostname.endswith('.' + domain)
        for domain in self.allowed_domains):
      raise UrlValidationError('URL host is not allowed: ' + hostname)

    try:
      addresses = self.resolve_host(hostname, parsed.port or 0)
    except OSError as error:
      raise UrlValidationError(
        'cannot resolve URL host: ' + hostname) from error
    for address in addresses:
      ip = ipaddress.ip_address(address[4][0])
      if not ip.is_global:
        raise UrlValidationError(
          'URL resolves to a non-public address: ' + str(ip))
    return url
