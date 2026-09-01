import csv
import io
import json
import os
import re
import shlex
import subprocess
import urllib.error
import urllib.parse
import urllib.request


IDENTIFIER_PATTERN = re.compile(r'[A-Za-z_][A-Za-z0-9_$]*')
POSTGRES_ENVIRONMENT = {
  'host': 'PGHOST',
  'hostaddr': 'PGHOSTADDR',
  'port': 'PGPORT',
  'dbname': 'PGDATABASE',
  'user': 'PGUSER',
  'password': 'PGPASSWORD',
  'sslmode': 'PGSSLMODE',
  'connect_timeout': 'PGCONNECT_TIMEOUT',
  'application_name': 'PGAPPNAME',
}


def normalize_select_query(query, max_query_chars=100000):
  if not isinstance(query, str) or not query.strip():
    raise ValueError('query must be a non-empty string')
  if len(query) > max_query_chars:
    raise ValueError('query is too long')
  query = query.strip()
  if query.endswith(';'):
    query = query[:-1].rstrip()
  first_word = _scan_query(query)
  if first_word not in ('select', 'with'):
    raise ValueError('only SELECT and WITH queries are allowed')
  return query


def parse_relation_name(value, default_namespace):
  if not isinstance(value, str):
    raise ValueError('relation must be a string')
  parts = value.split('.')
  if len(parts) == 1:
    parts.insert(0, default_namespace)
  if len(parts) != 2 or not all(IDENTIFIER_PATTERN.fullmatch(part) for part in parts):
    raise ValueError('relation must be namespace.name')
  return tuple(parts)


def sql_literal(value):
  return "'" + value.replace("'", "''") + "'"


def _scan_query(query):
  first_word = None
  index = 0
  size = len(query)
  while index < size:
    char = query[index]
    if char.isspace():
      index += 1
      continue
    if query.startswith('--', index):
      newline = query.find('\n', index + 2)
      index = size if newline < 0 else newline + 1
      continue
    if query.startswith('/*', index):
      index = _skip_block_comment(query, index)
      continue
    if char in ('\'', '"'):
      index = _skip_quoted(query, index, char)
      continue
    if char == '$':
      match = re.match(r'\$[A-Za-z_][A-Za-z0-9_]*\$|\$\$', query[index:])
      if match:
        delimiter = match.group(0)
        end = query.find(delimiter, index + len(delimiter))
        if end < 0:
          raise ValueError('unterminated dollar-quoted string')
        index = end + len(delimiter)
        continue
    if char == ';':
      raise ValueError('multiple SQL statements are not allowed')
    if char.isalpha() or char == '_':
      end = index + 1
      while end < size and (query[end].isalnum() or query[end] in ('_', '$')):
        end += 1
      if first_word is None:
        first_word = query[index:end].lower()
      index = end
      continue
    index += 1
  if first_word is None:
    raise ValueError('query contains no SQL statement')
  return first_word


def _skip_quoted(query, index, delimiter):
  index += 1
  while index < len(query):
    if query[index] == '\\':
      index += 2
      continue
    if query[index] == delimiter:
      if index + 1 < len(query) and query[index + 1] == delimiter:
        index += 2
        continue
      return index + 1
    index += 1
  raise ValueError('unterminated quoted string')


def _skip_block_comment(query, index):
  depth = 1
  index += 2
  while index < len(query):
    if query.startswith('/*', index):
      depth += 1
      index += 2
    elif query.startswith('*/', index):
      depth -= 1
      index += 2
      if depth == 0:
        return index
    else:
      index += 1
  raise ValueError('unterminated block comment')


class PostgresClient:
  def __init__(
      self,
      connection_string,
      query_timeout_ms=5000,
      max_rows=1000,
      max_result_bytes=1024 * 1024,
      psql_binary='/usr/bin/psql'):
    self.environment = self._connection_environment(connection_string)
    self.query_timeout_ms = query_timeout_ms
    self.max_rows = max_rows
    self.max_result_bytes = max_result_bytes
    self.psql_binary = psql_binary

  def query(self, query):
    query = normalize_select_query(query)
    limited_query = (
      'SELECT * FROM (' + query + ') AS mcp_result LIMIT ' +
      str(self.max_rows + 1))
    copy_query = (
      'COPY (' + limited_query +
      ") TO STDOUT WITH (FORMAT CSV, HEADER TRUE, NULL '\\N')")
    environment = os.environ.copy()
    environment.update(self.environment)
    environment['PGAPPNAME'] = 'foros-cluster-data-mcp'
    environment['PGOPTIONS'] = (
      '-c default_transaction_read_only=on '
      '-c statement_timeout=' + str(self.query_timeout_ms) + ' '
      '-c lock_timeout=1000')
    try:
      result = subprocess.run(
        [
          self.psql_binary,
          '--no-psqlrc',
          '--quiet',
          '--set=ON_ERROR_STOP=1',
          '--command',
          copy_query,
        ],
        env=environment,
        capture_output=True,
        timeout=max(1.0, self.query_timeout_ms / 1000.0 + 2.0),
        check=False)
    except subprocess.TimeoutExpired as error:
      raise RuntimeError('PostgreSQL query timeout') from error
    if result.returncode != 0:
      message = result.stderr.decode('utf-8', errors='replace').strip()
      raise RuntimeError('PostgreSQL query failed: ' + message)
    if len(result.stdout) > self.max_result_bytes:
      raise RuntimeError('PostgreSQL result exceeds byte limit')
    return self._parse_csv(result.stdout)

  def list_relations(self, schema='public'):
    if not IDENTIFIER_PATTERN.fullmatch(schema):
      raise ValueError('invalid schema name')
    return self.query(
      'SELECT table_schema, table_name, table_type '
      'FROM information_schema.tables '
      'WHERE table_schema = ' + sql_literal(schema) + ' '
      'ORDER BY table_name')

  def describe_relation(self, relation):
    schema, name = parse_relation_name(relation, 'public')
    return self.query(
      'SELECT table_schema, table_name, ordinal_position, column_name, '
      'data_type, is_nullable '
      'FROM information_schema.columns '
      'WHERE table_schema = ' + sql_literal(schema) + ' '
      'AND table_name = ' + sql_literal(name) + ' '
      'ORDER BY ordinal_position')

  def _parse_csv(self, value):
    rows = list(csv.reader(io.StringIO(value.decode('utf-8'))))
    if not rows:
      return {'columns': [], 'rows': [], 'row_count': 0, 'truncated': False}
    columns = rows[0]
    data = [
      [None if field == '\\N' else field for field in row]
      for row in rows[1:]
    ]
    truncated = len(data) > self.max_rows
    if truncated:
      data = data[:self.max_rows]
    return {
      'columns': columns,
      'rows': data,
      'row_count': len(data),
      'truncated': truncated,
    }

  @staticmethod
  def _connection_environment(connection_string):
    if not connection_string:
      raise ValueError('PostgreSQL connection string is empty')
    environment = {}
    for token in shlex.split(connection_string):
      name, separator, value = token.partition('=')
      if not separator or name not in POSTGRES_ENVIRONMENT:
        raise ValueError('unsupported PostgreSQL connection parameter: ' + name)
      environment[POSTGRES_ENVIRONMENT[name]] = value
    if 'PGDATABASE' not in environment or 'PGUSER' not in environment:
      raise ValueError('PostgreSQL dbname and user are required')
    return environment


class ClickHouseClient:
  def __init__(
      self,
      url,
      user='default',
      password='',
      query_timeout_ms=5000,
      max_rows=1000,
      max_result_bytes=1024 * 1024):
    self.url = url.rstrip('/') + '/'
    self.user = user
    self.password = password
    self.query_timeout_ms = query_timeout_ms
    self.max_rows = max_rows
    self.max_result_bytes = max_result_bytes

  def query(self, query, database='default'):
    query = normalize_select_query(query)
    if not IDENTIFIER_PATTERN.fullmatch(database):
      raise ValueError('invalid database name')
    limited_query = (
      'SELECT * FROM (' + query + ') AS mcp_result LIMIT ' +
      str(self.max_rows + 1) + ' FORMAT JSONCompact')
    params = urllib.parse.urlencode({
      'database': database,
      'readonly': '2',
      'max_execution_time': str(self.query_timeout_ms / 1000.0),
      'max_result_rows': str(self.max_rows + 1),
      'result_overflow_mode': 'throw',
      'max_result_bytes': str(self.max_result_bytes),
    })
    request = urllib.request.Request(
      self.url + '?' + params,
      data=limited_query.encode('utf-8'),
      headers={
        'Content-Type': 'text/plain; charset=utf-8',
        'X-ClickHouse-User': self.user,
        'X-ClickHouse-Key': self.password,
      })
    try:
      with urllib.request.urlopen(
          request,
          timeout=max(1.0, self.query_timeout_ms / 1000.0 + 2.0)) as response:
        body = response.read(self.max_result_bytes + 1)
    except urllib.error.HTTPError as error:
      message = error.read(8192).decode('utf-8', errors='replace').strip()
      raise RuntimeError('ClickHouse query failed: ' + message) from error
    except urllib.error.URLError as error:
      raise RuntimeError('ClickHouse request failed: ' + str(error.reason)) from error
    if len(body) > self.max_result_bytes:
      raise RuntimeError('ClickHouse result exceeds byte limit')
    result = json.loads(body)
    columns = [column['name'] for column in result.get('meta', [])]
    data = result.get('data', [])
    truncated = len(data) > self.max_rows
    if truncated:
      data = data[:self.max_rows]
    return {
      'columns': columns,
      'rows': data,
      'row_count': len(data),
      'truncated': truncated,
    }

  def list_relations(self, database='default'):
    if not IDENTIFIER_PATTERN.fullmatch(database):
      raise ValueError('invalid database name')
    return self.query(
      'SELECT database, name, engine FROM system.tables '
      'WHERE database = ' + sql_literal(database) + ' ORDER BY name',
      'system')

  def describe_relation(self, relation, database='default'):
    relation_database, name = parse_relation_name(relation, database)
    return self.query(
      'SELECT database, table, position, name, type '
      'FROM system.columns '
      'WHERE database = ' + sql_literal(relation_database) + ' '
      'AND table = ' + sql_literal(name) + ' ORDER BY position',
      'system')
