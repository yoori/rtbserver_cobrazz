#!/usr/bin/python3.12

import argparse
import json
import os
import pathlib
import sys


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SOURCE_ROOT / 'lib'))

from ai_agent.ClusterDataTools import ClickHouseClient
from ai_agent.ClusterDataTools import PostgresClient
from ai_agent.WebTools import serialize_tool_result


def tool_definition(name, description, properties, required=()):
  return {
    'name': name,
    'description': description,
    'inputSchema': {
      'type': 'object',
      'properties': properties,
      'required': list(required),
      'additionalProperties': False,
    },
    'annotations': {
      'readOnlyHint': True,
      'destructiveHint': False,
      'idempotentHint': True,
      'openWorldHint': False,
    },
  }


POSTGRES_TOOLS = [
  tool_definition(
    'postgres_list_relations',
    'List PostgreSQL tables and views in a schema.',
    {'schema': {'type': 'string', 'default': 'public'}}),
  tool_definition(
    'postgres_describe_relation',
    'List PostgreSQL columns for schema.relation.',
    {'relation': {'type': 'string'}},
    ('relation',)),
  tool_definition(
    'postgres_select',
    'Execute one read-only PostgreSQL SELECT or WITH query.',
    {'query': {'type': 'string'}},
    ('query',)),
]

CLICKHOUSE_TOOLS = [
  tool_definition(
    'clickhouse_list_relations',
    'List ClickHouse tables and views in a database.',
    {'database': {'type': 'string', 'default': 'default'}}),
  tool_definition(
    'clickhouse_describe_relation',
    'List ClickHouse columns for database.relation.',
    {
      'relation': {'type': 'string'},
      'database': {'type': 'string', 'default': 'default'},
    },
    ('relation',)),
  tool_definition(
    'clickhouse_select',
    'Execute one read-only ClickHouse SELECT or WITH query.',
    {
      'query': {'type': 'string'},
      'database': {'type': 'string', 'default': 'default'},
    },
    ('query',)),
]


def write_message(message):
  sys.stdout.write(json.dumps(message, separators=(',', ':')) + '\n')
  sys.stdout.flush()


def tool_error(error):
  return {
    'content': [{'type': 'text', 'text': str(error)}],
    'isError': True,
  }


def run():
  parser = argparse.ArgumentParser(description='Read-only cluster data MCP server.')
  parser.add_argument('--query-timeout-ms', type=int, default=5000)
  parser.add_argument('--max-rows', type=int, default=1000)
  parser.add_argument('--max-result-bytes', type=int, default=1024 * 1024)
  args = parser.parse_args()

  postgres = None
  if os.environ.get('AI_POSTGRES_CONNECTION_STRING'):
    postgres = PostgresClient(
      os.environ['AI_POSTGRES_CONNECTION_STRING'],
      args.query_timeout_ms,
      args.max_rows,
      args.max_result_bytes)
  clickhouse = None
  if os.environ.get('AI_CLICKHOUSE_URL'):
    clickhouse = ClickHouseClient(
      os.environ['AI_CLICKHOUSE_URL'],
      os.environ.get('AI_CLICKHOUSE_USER', 'default'),
      os.environ.get('AI_CLICKHOUSE_PASSWORD', ''),
      args.query_timeout_ms,
      args.max_rows,
      args.max_result_bytes)
  tools = []
  if postgres:
    tools.extend(POSTGRES_TOOLS)
  if clickhouse:
    tools.extend(CLICKHOUSE_TOOLS)

  for line in sys.stdin:
    try:
      request = json.loads(line)
      request_id = request.get('id')
      method = request.get('method')
      if request_id is None:
        continue
      if method == 'initialize':
        result = {
          'protocolVersion': '2025-06-18',
          'capabilities': {'tools': {}},
          'serverInfo': {'name': 'foros-cluster-data', 'version': '1.0'},
        }
      elif method == 'tools/list':
        result = {'tools': tools}
      elif method == 'tools/call':
        params = request.get('params') or {}
        arguments = params.get('arguments') or {}
        try:
          name = params.get('name')
          if name == 'postgres_list_relations' and postgres:
            result = serialize_tool_result(postgres.list_relations(**arguments))
          elif name == 'postgres_describe_relation' and postgres:
            result = serialize_tool_result(postgres.describe_relation(**arguments))
          elif name == 'postgres_select' and postgres:
            result = serialize_tool_result(postgres.query(**arguments))
          elif name == 'clickhouse_list_relations' and clickhouse:
            result = serialize_tool_result(clickhouse.list_relations(**arguments))
          elif name == 'clickhouse_describe_relation' and clickhouse:
            result = serialize_tool_result(clickhouse.describe_relation(**arguments))
          elif name == 'clickhouse_select' and clickhouse:
            result = serialize_tool_result(clickhouse.query(**arguments))
          else:
            raise ValueError('unknown or disabled tool: ' + str(name))
        except Exception as error:
          result = tool_error(error)
      else:
        write_message({
          'jsonrpc': '2.0',
          'id': request_id,
          'error': {'code': -32601, 'message': 'Method not supported'},
        })
        continue
      write_message({'jsonrpc': '2.0', 'id': request_id, 'result': result})
    except Exception as error:
      print('ClusterDataMcp request failed: ' + str(error), file=sys.stderr)


if __name__ == '__main__':
  run()
