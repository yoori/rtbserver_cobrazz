import datetime
import logging
import os
import pathlib
import shlex
import subprocess
import tempfile


class RImpressionTrainExporter(object):
  def __init__(self, clickhouse_conn, logger=None):
    self._command = [
      'clickhouse-client',
      *shlex.split(clickhouse_conn),
    ]
    self._logger = logger or logging.getLogger(__name__)

  def export(self, output_file, train_rows, data_delay):
    if train_rows <= 0:
      raise ValueError('train_rows must be positive')
    if data_delay <= 0:
      raise ValueError('data_delay must be positive')

    date_to = (
      datetime.datetime.now(datetime.timezone.utc) -
      datetime.timedelta(seconds=data_delay)
    ).strftime('%Y-%m-%d %H:%M:%S')
    date_from = self._find_date_from(train_rows, date_to)
    self._logger.debug(
      'Exporting up to %d RImpressionTrain rows from %s to %s',
      train_rows,
      date_from,
      date_to)

    output_path = pathlib.Path(output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary_file = tempfile.NamedTemporaryFile(
      mode='w',
      dir=str(output_path.parent),
      prefix=output_path.name + '.',
      suffix='.tmp',
      delete=False)
    temporary_path = temporary_file.name

    try:
      with temporary_file:
        subprocess.run(
          self._command + [
            '--query',
            self._export_query(date_from, date_to, train_rows),
          ],
          check=True,
          stdout=temporary_file)
      os.replace(temporary_path, str(output_path))
    except Exception:
      try:
        os.unlink(temporary_path)
      except FileNotFoundError:
        pass
      raise

    return date_from

  def _find_date_from(self, train_rows, date_to):
    result = subprocess.run(
      self._command + [
        '--query',
        (
          'SELECT toDate(timestamp), count(*) FROM RImpression '
          "WHERE timestamp < '" + date_to + "' "
          'GROUP BY toDate(timestamp) ORDER BY toDate(timestamp) DESC'),
      ],
      check=True,
      capture_output=True,
      text=True)

    date_from = None
    selected_rows = 0
    for line in result.stdout.splitlines():
      date_value, row_count = line.split('\t')
      self._logger.debug('%s => %s', date_value, row_count)
      date_from = date_value
      selected_rows += int(row_count)
      if selected_rows >= train_rows:
        break

    if date_from is None:
      raise RuntimeError('RImpression contains no rows')
    return date_from

  @staticmethod
  def _export_query(date_from, date_to, train_rows):
    return (
      "SELECT "
      "If(click_timestamp IS NOT NULL, 1, 0) AS label, "
      "timestamp, "
      "device AS Device, "
      "url AS Link, "
      "publisher_id AS Publisher, "
      "tag_id AS Tag, "
      "etag AS ETag, "
      "campaign_id AS Campaign, "
      "ccg_id AS Group, "
      "ccid AS CCID, "
      "geo_ch AS GeoCh, "
      "user_ch AS UserCh, "
      "size_id AS SizeID, "
      "colo_id AS Colo, "
      "campaign_freq AS Campaign_Freq "
      "FROM RImpression "
      "WHERE timestamp >= '" + date_from + "' "
      "AND timestamp < '" + date_to + "' "
      "LIMIT " + str(train_rows) + " FORMAT CSVWithNames")
