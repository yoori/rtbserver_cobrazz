import zlib


def url_bucket(url, buckets):
  if buckets <= 0:
    raise ValueError('URL bucket count must be positive')
  return zlib.crc32(url.encode('utf-8')) % buckets
