"""Named goals: durable publication state lives in ClickHouse, PG receives CSV only."""

import ast
import csv
import datetime
import json
import os

from ServiceUtilsPy.Context import Context
from rtbserver_utils.ExternalMetricsTypes import YANDEX_METRIKA


GOALS_TABLE = 'YandexMetrikaGoals'
NAMES_TABLE = 'YandexPostClickConvNamesSyncV3'
VISITS_TABLE = 'YandexPostClickConvVisitsSync'
GOALS_SYNC_TABLE = 'YandexPostClickConvGoalsSync'
GOAL_FIELDS = ('ym:s:goalsID', 'ym:s:goalsSerialNumber', 'ym:s:goalsDateTime')
NAMES_COLUMNS = ('metrika_id', 'conversion_name', 'name', 'version')
NAMES_CSV_COLUMNS = (
  'external_metrics_connection_type_id', 'conversion_name', 'name', 'version')
SYNC_COLUMNS = (
  'metrika_id', 'event_date', 'visit_id', 'goal_id', 'serial', 'request_id', 'published_at')

DDL = (
  f"""CREATE TABLE IF NOT EXISTS {GOALS_TABLE}
  (
    metrika_id UInt64, goal_id UInt64, name String, type String,
    version DateTime64(6, 'UTC')
  ) ENGINE = ReplacingMergeTree(version) ORDER BY (metrika_id, goal_id)""",
  f"""CREATE TABLE IF NOT EXISTS {NAMES_TABLE}
  (
    metrika_id UInt64, conversion_name String,
    name String, version DateTime64(6, 'UTC')
  ) ENGINE = ReplacingMergeTree(version)
  ORDER BY (metrika_id, conversion_name)""",
) + tuple(
  f"""CREATE TABLE IF NOT EXISTS {table}
  (
    metrika_id UInt64, event_date Date, visit_id UInt64, goal_id UInt64,
    serial UInt64, request_id String, published_at DateTime64(6, 'UTC')
  ) ENGINE = ReplacingMergeTree(published_at)
  PARTITION BY toYYYYMM(event_date)
  ORDER BY (metrika_id, event_date, visit_id, goal_id, serial)"""
  for table in (VISITS_TABLE, GOALS_SYNC_TABLE))


def conversion_name(counter_id, goal_id):
  return f'ym_{int(counter_id)}_{int(goal_id)}'


def goal_events(row):
  values = [row.get(field) for field in GOAL_FIELDS]
  if all(value is None for value in values):
    # Requests created before the upgrade do not contain goal arrays.
    return []
  if any(value is None for value in values):
    raise ValueError('Incomplete Logs API goal fields')
  arrays = [ast.literal_eval(value) for value in values]
  if any(not isinstance(value, list) for value in arrays):
    raise ValueError('Logs API goal field must be an array')
  if len({len(value) for value in arrays}) != 1:
    raise ValueError('Logs API goal arrays have different lengths')
  result = []
  for goal_id, serial, timestamp in zip(*arrays):
    if type(goal_id) is not int or goal_id <= 0 or type(serial) is not int or serial < 0:
      raise ValueError('Invalid goal ID or serial number')
    result.append((goal_id, serial, timestamp))
  return result


class ConversionImporter:
  def __init__(self, application, parse_term, parse_time, request_chunk):
    self.app = application
    self.parse_term = parse_term
    self.parse_time = parse_time
    self.request_chunk = request_chunk
    self.names_dir = os.path.join(application.out_dir, 'PostClickConvNames')
    os.makedirs(self.names_dir, exist_ok=True)
    self.goals = {}

  def prepare(self, api):
    goals = api.goals()
    counter_id = int(api.counter_id)
    now = datetime.datetime.now(datetime.timezone.utc)
    rows = [(counter_id, int(goal['id']), goal['name'], goal['type'], now) for goal in goals]
    if rows:
      self.app.ch.insert(
        GOALS_TABLE, rows, column_names=('metrika_id', 'goal_id', 'name', 'type', 'version'))
    self.goals[counter_id] = {
      conversion_name(counter_id, goal['id']): goal['name'] for goal in goals}
    self.publish_names(counter_id, self.goals[counter_id])

  def publish_names(self, counter_id, goals):
    if not goals:
      return
    existing = {
      row[1]: row for row in self.app.ch.query(
        f"SELECT {', '.join(NAMES_COLUMNS)} FROM {NAMES_TABLE} FINAL "
        'WHERE metrika_id = %(counter)s',
        parameters={'counter': counter_id}).result_rows
    }
    now = datetime.datetime.now(datetime.timezone.utc)
    rows = []
    for key, name in goals.items():
      old = existing.get(key)
      if name is None:
        if old:
          continue
        name = key
      if old and old[2] == name:
        continue
      version = max(now, old[3] + datetime.timedelta(microseconds=1)) if old else now
      rows.append((counter_id, key, name, version))
    if not rows:
      return
    # Commit the queue first. A retry can duplicate CSV; the version makes replay safe.
    with Context(self.app, out_dir=self.names_dir) as context:
      writer = context.files.get_line_writer(
        name='PostClickConvNames_' + context.fname_seed.replace('.', '-') + '.csv')
      writer.write_line(self.app._csv_line(NAMES_CSV_COLUMNS))
      for row in rows:
        writer.write_line(self.app._csv_line((YANDEX_METRIKA, *row[1:3], row[3].isoformat())))
      writer.write('\n')
    self.app.ch.insert(NAMES_TABLE, rows, column_names=NAMES_COLUMNS)

  def process(self, counter_id, event_date, text):
    counter_id = int(counter_id)
    records = {}
    names = {}
    for row in csv.DictReader(text.splitlines(), delimiter='\t'):
      term = self.parse_term(self.app._field(row, 'UTMTerm'))
      if term is None:
        continue
      request_id, distribution_hash = term
      visit_id = int(self.app._field(row, 'visitID'))
      for goal_id, serial, timestamp in goal_events(row):
        key = conversion_name(counter_id, goal_id)
        names[key] = self.goals.get(counter_id, {}).get(key)
        for kind, event_serial, event_time in (
            ('v', 0, self.app._field(row, 'dateTimeUTC')), ('g', serial, timestamp)):
          key = (kind, visit_id, goal_id, event_serial)
          action = f'{conversion_name(counter_id, goal_id)}:{kind}:{event_date}:{visit_id}'
          if kind == 'g':
            action += f':{serial}'
          payload = json.dumps({
            'external_metrics_connection_type_id': YANDEX_METRIKA,
            'source_date': event_date.isoformat(),
            'visits': int(kind == 'v'), 'goals': int(kind == 'g'),
          }, separators=(',', ':'), sort_keys=True)
          records[key] = (
            self.parse_time(event_time), request_id, distribution_hash, action, payload)
    self.publish_names(counter_id, names)
    for kind, table in (('v', VISITS_TABLE), ('g', GOALS_SYNC_TABLE)):
      processed = set(tuple(row) for row in self.app.ch.query(
        f'SELECT visit_id, goal_id, serial FROM {table} FINAL '
        'WHERE metrika_id = %(counter)s AND event_date = %(date)s',
        parameters={'counter': counter_id, 'date': event_date}).result_rows)
      new = {key: value for key, value in records.items()
             if key[0] == kind and key[1:] not in processed}
      if not new:
        continue
      self.publish_actions(new.values())
      now = datetime.datetime.now(datetime.timezone.utc)
      self.app.ch.insert(table, [
        (counter_id, event_date, *key[1:], value[1], now) for key, value in new.items()
      ], column_names=SYNC_COLUMNS)

  def publish_actions(self, records):
    with Context(self.app, out_dir=self.app.post_click_dir) as context:
      writers = {}
      for time, request_id, distribution_hash, action, payload in records:
        chunk = distribution_hash % self.app.chunks_count if distribution_hash is not None else \
          self.request_chunk(request_id, self.app.chunks_count)
        if chunk not in writers:
          writers[chunk] = context.files.get_line_writer(
            key=chunk, name=lambda chunk=chunk:
              f'PostClickAction.{context.fname_seed}.{self.app.chunks_count}.{chunk}')
          writers[chunk].write_line('PostClickAction\t1.0')
        writers[chunk].write_line(
          time.strftime('%Y-%m-%d_%H:%M:%S') + '\t' + request_id + '\t' + action + '\t' + payload)
      for writer in writers.values():
        writer.write('\n')
