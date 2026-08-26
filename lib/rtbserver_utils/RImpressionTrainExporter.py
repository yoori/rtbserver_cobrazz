import datetime
import logging
import math
import os
import pathlib
import shlex
import subprocess
import tempfile


class RImpressionTrainExporter(object):
  SOURCE_PARTITIONS = 100
  VALIDATION_PARTITIONS = (0, 1)

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

    date_from, date_to = self.find_date_range(train_rows, data_delay)
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

  def find_date_range(self, source_rows, data_delay):
    if source_rows <= 0:
      raise ValueError('source_rows must be positive')
    if data_delay <= 0:
      raise ValueError('data_delay must be positive')

    date_to = (
      datetime.datetime.now(datetime.timezone.utc) -
      datetime.timedelta(seconds=data_delay)
    ).strftime('%Y-%m-%d %H:%M:%S')
    return self._find_date_from(source_rows, date_to), date_to

  def count_rows(self, date_from, date_to, condition=None):
    result = subprocess.run(
      self._command + [
        '--query',
        self._count_query(date_from, date_to, condition),
      ],
      check=True,
      capture_output=True,
      text=True)
    return int(result.stdout.strip())

  def ssp_ctr_logloss(
      self,
      date_from,
      date_to,
      rows,
      condition=None,
      offset_rows=0,
  ):
    if rows <= 0:
      raise ValueError('rows must be positive')
    if offset_rows < 0:
      raise ValueError('offset_rows must not be negative')
    result = subprocess.run(
      self._command + [
        '--query',
        self._ssp_ctr_logloss_query(
          date_from,
          date_to,
          rows,
          condition,
          offset_rows),
      ],
      check=True,
      capture_output=True,
      text=True)
    value = float(result.stdout.strip())
    if not math.isfinite(value):
      raise RuntimeError('SSP CTR logloss is not finite')
    return value

  def eligible_campaigns(
      self,
      date_from,
      date_to,
      activity_period,
      min_impressions,
  ):
    if activity_period <= 0:
      raise ValueError('activity_period must be positive')
    if min_impressions <= 0:
      raise ValueError('min_impressions must be positive')
    training_condition = self.training_condition()
    validation_condition = self.validation_condition()
    query = (
      'SELECT campaign_id, '
        'countIf(' + training_condition + ') AS training_impressions, '
        'countIf(' + validation_condition + ') AS validation_impressions '
      'FROM RImpression '
      "WHERE timestamp >= '" + date_from + "' "
        "AND timestamp < '" + date_to + "' "
        'AND campaign_id > 0 '
        'AND campaign_id IN ('
          'SELECT campaign_id FROM RImpression '
          'WHERE timestamp >= subtractSeconds('
          "toDateTime('" + date_to + "'), " + str(activity_period) + ') '
          "AND timestamp < '" + date_to + "' "
          'AND campaign_id > 0 '
          'GROUP BY campaign_id) '
      'GROUP BY campaign_id '
      'HAVING training_impressions > ' + str(min_impressions) + ' '
      'ORDER BY campaign_id')
    result = subprocess.run(
      self._command + ['--query', query],
      check=True,
      capture_output=True,
      text=True)
    campaigns = []
    for line in result.stdout.splitlines():
      campaign_id, training_impressions, validation_impressions = (
        line.split('\t'))
      campaigns.append((
        int(campaign_id),
        int(training_impressions),
        int(validation_impressions),
      ))
    return campaigns

  def export_chunks(
      self,
      output_dir,
      file_prefix,
      max_rows,
      chunk_rows,
      date_from,
      date_to,
      condition=None,
      offset_rows=0,
      label='click',
  ):
    if max_rows <= 0:
      raise ValueError('max_rows must be positive')
    if chunk_rows <= 0:
      raise ValueError('chunk_rows must be positive')
    if offset_rows < 0:
      raise ValueError('offset_rows must not be negative')

    output_dir = pathlib.Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    process = subprocess.Popen(
      self._command + [
        '--query',
        self._export_query(
          date_from,
          date_to,
          max_rows,
          condition,
          offset_rows,
          label),
      ],
      stdout=subprocess.PIPE)
    current_path = None
    completed = False
    try:
      header = process.stdout.readline()
      if not header:
        return_code = process.wait()
        completed = True
        if return_code != 0:
          raise subprocess.CalledProcessError(return_code, process.args)
        raise RuntimeError('RImpression export returned no CSV header')

      exported_rows = 0
      chunk_index = 0
      while exported_rows < max_rows:
        output_path = output_dir / (
          file_prefix + '-' + str(chunk_index).zfill(3) + '.csv')
        temporary_file = tempfile.NamedTemporaryFile(
          mode='wb',
          dir=str(output_dir),
          prefix=output_path.name + '.',
          suffix='.tmp',
          delete=False)
        temporary_path = temporary_file.name
        row_count = 0
        try:
          with temporary_file:
            temporary_file.write(header)
            rows_to_read = min(chunk_rows, max_rows - exported_rows)
            while row_count < rows_to_read:
              line = process.stdout.readline()
              if not line:
                break
              temporary_file.write(line)
              row_count += 1

          if row_count == 0:
            os.unlink(temporary_path)
            break
          os.replace(temporary_path, output_path)
        except Exception:
          try:
            os.unlink(temporary_path)
          except FileNotFoundError:
            pass
          raise

        exported_rows += row_count
        is_final_chunk = (
          exported_rows >= max_rows or row_count < rows_to_read)
        current_path = output_path
        if is_final_chunk:
          trailing_size = 0
          while True:
            trailing_data = process.stdout.read(1024 * 1024)
            if not trailing_data:
              break
            trailing_size += len(trailing_data)
          return_code = process.wait()
          completed = True
          if return_code != 0:
            raise subprocess.CalledProcessError(return_code, process.args)
          if trailing_size:
            raise RuntimeError(
              'RImpression export returned ' + str(trailing_size) +
              ' bytes after the expected ' + str(exported_rows) +
              ' CSV rows; a field probably contains a line break')

        try:
          yield output_path, row_count
        finally:
          try:
            output_path.unlink()
          except FileNotFoundError:
            pass
          current_path = None

        chunk_index += 1
        if is_final_chunk:
          break

      if not completed:
        return_code = process.wait()
        completed = True
        if return_code != 0:
          raise subprocess.CalledProcessError(return_code, process.args)
    finally:
      if current_path is not None:
        try:
          current_path.unlink()
        except FileNotFoundError:
          pass
      if process.stdout is not None:
        process.stdout.close()
      if not completed and process.poll() is None:
        process.terminate()
        try:
          process.wait(timeout=5)
        except subprocess.TimeoutExpired:
          process.kill()
          process.wait()

  @classmethod
  def validation_condition(cls):
    return cls._source_partition_expression() + ' IN (' + ','.join(
      str(partition) for partition in cls.VALIDATION_PARTITIONS) + ')'

  @classmethod
  def training_condition(cls):
    return cls._source_partition_expression() + ' NOT IN (' + ','.join(
      str(partition) for partition in cls.VALIDATION_PARTITIONS) + ')'

  @staticmethod
  def campaign_condition(campaign_id):
    campaign_id = int(campaign_id)
    if campaign_id <= 0:
      raise ValueError('campaign_id must be positive')
    return 'campaign_id = ' + str(campaign_id)

  @staticmethod
  def ssp_ctr_condition():
    return 'ssp_ctr IS NOT NULL'

  @classmethod
  def training_partition_condition(cls, partition_index, partition_count):
    if partition_count <= 0:
      raise ValueError('partition_count must be positive')
    if partition_index < 0 or partition_index >= partition_count:
      raise ValueError('partition_index is out of range')
    source_partitions = [
      partition
      for partition in range(cls.SOURCE_PARTITIONS)
      if (
        partition not in cls.VALIDATION_PARTITIONS and
        partition % partition_count == partition_index)
    ]
    if not source_partitions:
      return '0'
    return cls._source_partition_expression() + ' IN (' + ','.join(
      str(partition) for partition in source_partitions) + ')'

  def export_partitioned_chunks(
      self,
      output_dir,
      file_prefix,
      max_rows,
      chunk_rows,
      partition_count,
      date_from,
      date_to,
      condition=None,
      label='click',
  ):
    remaining_rows = max_rows
    for partition_index in range(partition_count):
      if remaining_rows == 0:
        break
      rows_to_export = min(chunk_rows, remaining_rows)
      partition_condition = self.training_partition_condition(
        partition_index,
        partition_count)
      if condition is not None:
        partition_condition = (
          '(' + partition_condition + ') AND (' + condition + ')')
      chunks = self.export_chunks(
        output_dir,
        file_prefix + '-' + str(partition_index).zfill(3),
        rows_to_export,
        rows_to_export,
        date_from,
        date_to,
        partition_condition,
        label=label)
      try:
        for output_path, row_count in chunks:
          remaining_rows -= row_count
          yield output_path, row_count
      finally:
        chunks.close()

  @classmethod
  def required_source_rows(cls, training_rows, validation_rows):
    if training_rows <= 0:
      raise ValueError('training_rows must be positive')
    if validation_rows <= 0:
      raise ValueError('validation_rows must be positive')
    validation_partitions = len(cls.VALIDATION_PARTITIONS)
    training_partitions = cls.SOURCE_PARTITIONS - validation_partitions
    return max(
      math.ceil(training_rows * cls.SOURCE_PARTITIONS / training_partitions),
      math.ceil(
        validation_rows * cls.SOURCE_PARTITIONS / validation_partitions))

  @classmethod
  def _source_partition_expression(cls):
    return 'sipHash64(request_id) % ' + str(cls.SOURCE_PARTITIONS)

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

  @classmethod
  def _export_query(
      cls,
      date_from,
      date_to,
      train_rows,
      condition=None,
      offset_rows=0,
      label='click',
  ):
    if label == 'click':
      label_expression = 'If(click_timestamp IS NOT NULL, 1, 0)'
    elif label == 'ssp_ctr':
      label_expression = 'assumeNotNull(ssp_ctr)'
    else:
      raise ValueError("Unsupported export label: '" + str(label) + "'")
    query = (
      "SELECT "
      + label_expression + " AS label, "
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
        "campaign_freq AS Campaign_Freq, "
        "ifNull(ssp_tag_id, '') AS SSP_Tag_ID, "
        "ifNull(toString(ssp_ctr), '') AS SSP_CTR, "
        "ifNull(toString(ssp_viewability), '') AS SSP_Viewability, "
        "ifNull(toString(ssp_vtr), '') AS SSP_VTR "
      "FROM RImpression "
      "WHERE timestamp >= '" + date_from + "' "
        "AND timestamp < '" + date_to + "' ")
    if condition is not None:
      query += 'AND (' + condition + ') '
    return (
      query +
      'ORDER BY timestamp DESC, request_id DESC '
      'LIMIT ' + str(train_rows) +
      ((' OFFSET ' + str(offset_rows)) if offset_rows else '') +
      ' FORMAT CSVWithNames')

  @classmethod
  def _ssp_ctr_logloss_query(
      cls,
      date_from,
      date_to,
      rows,
      condition=None,
      offset_rows=0,
  ):
    score = 'greatest(least(score, 1 - 1e-15), 1e-15)'
    where_condition = cls.ssp_ctr_condition()
    if condition is not None:
      where_condition = (
        '(' + where_condition + ') AND (' + condition + ')')
    return (
      'SELECT avg(if(clicked, -log(' + score + '), '
        '-log1p(-' + score + '))) FROM ('
        'SELECT click_timestamp IS NOT NULL AS clicked, '
          'assumeNotNull(ssp_ctr) AS score '
        'FROM RImpression '
        "WHERE timestamp >= '" + date_from + "' "
          "AND timestamp < '" + date_to + "' "
          'AND (' + where_condition + ') '
        'ORDER BY timestamp DESC, request_id DESC '
        'LIMIT ' + str(rows) +
        ((' OFFSET ' + str(offset_rows)) if offset_rows else '') + ')')

  @staticmethod
  def _count_query(date_from, date_to, condition=None):
    query = (
      "SELECT count(*) FROM RImpression "
      "WHERE timestamp >= '" + date_from + "' "
        "AND timestamp < '" + date_to + "' ")
    if condition is not None:
      query += 'AND (' + condition + ')'
    return query
