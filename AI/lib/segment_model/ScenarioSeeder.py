CREATE_TABLE_QUERY = """
CREATE TABLE RImpression
(
  impression_id UInt64,
  request_id String,
  timestamp DateTime('UTC'),
  uid Nullable(String),
  click_timestamp Nullable(DateTime('UTC'))
)
ENGINE = MergeTree
ORDER BY impression_id
"""

INSERT_QUERY = """
INSERT INTO RImpression
SELECT
  impression_id,
  toString(impression_id),
  toDateTime(event_timestamp, 'UTC'),
  user_id,
  if(clicked != 0, toNullable(toDateTime(event_timestamp, 'UTC')), NULL)
FROM input(
  'impression_id UInt64, event_timestamp UInt64, user_id String, clicked UInt8')
FORMAT TabSeparated
"""


def seed_scenario(client, scenario, chunk_rows=100000, reset=False):
  if reset:
    client.execute('DROP TABLE IF EXISTS RImpression')
  client.execute(CREATE_TABLE_QUERY.replace('CREATE TABLE', 'CREATE TABLE IF NOT EXISTS', 1))
  existing_rows = int(client.execute('SELECT count() FROM RImpression').strip())
  if existing_rows:
    raise RuntimeError('RImpression is not empty; use reset for a reproducible test')
  for begin in range(0, scenario.rows, chunk_rows):
    end = min(scenario.rows, begin + chunk_rows)
    rows = []
    for numeric_uid in range(begin, end):
      rows.append('\t'.join((
        str(numeric_uid),
        str(scenario.timestamp(numeric_uid)),
        scenario.user_id(numeric_uid),
        str(scenario.clicked(numeric_uid)),
      )))
    client.execute(INSERT_QUERY, ('\n'.join(rows) + '\n').encode('utf-8'))
    print('Seeded RImpression rows: ' + str(end) + '/' + str(scenario.rows), flush=True)
  _verify_seed(client, scenario)


def _verify_seed(client, scenario):
  result = client.execute(
    'SELECT count(), countIf(click_timestamp IS NOT NULL), '
    'min(impression_id), max(impression_id) FROM RImpression')
  values = result.decode('utf-8').strip().split('\t')
  if int(values[0]) != scenario.rows or int(values[2]) != 0:
    raise RuntimeError('RImpression verification failed')
  if int(values[3]) != scenario.rows - 1:
    raise RuntimeError('RImpression impression_id range is incomplete')
  print('RImpression verification: rows=' + values[0] + ', clicks=' + values[1], flush=True)
