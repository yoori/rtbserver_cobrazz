#!/usr/bin/env python3

import datetime
import io
import pathlib
import subprocess
import tempfile
import unittest
from unittest import mock

from rtbserver_utils.RImpressionTrainExporter import RImpressionTrainExporter


class RImpressionTrainExporterTest(unittest.TestCase):
  class ProcessStub:
    def __init__(self, rows, require_drained=False):
      self.args = ['clickhouse-client']
      self.stdout = io.BytesIO(
        b'label,Device\n' + b''.join(rows))
      self.return_code = None
      self.terminated = False
      self.require_drained = require_drained

    def wait(self, timeout=None):
      del timeout
      if self.require_drained:
        self.assert_stdout_drained()
      self.return_code = 0
      return self.return_code

    def assert_stdout_drained(self):
      current_position = self.stdout.tell()
      self.stdout.seek(0, io.SEEK_END)
      end_position = self.stdout.tell()
      self.stdout.seek(current_position)
      if current_position != end_position:
        raise AssertionError('wait called before stdout was drained')

    def poll(self):
      return self.return_code

    def terminate(self):
      self.terminated = True
      self.return_code = -15

    def kill(self):
      self.return_code = -9

  def test_export(self):
    calls = []

    def run(command, **kwargs):
      calls.append(command)
      if kwargs.get('capture_output'):
        return subprocess.CompletedProcess(
          command,
          0,
          stdout='2026-08-13\t60\n2026-08-12\t50\n')

      kwargs['stdout'].write('label,Device\n1,Mobile\n')
      return subprocess.CompletedProcess(command, 0)

    with tempfile.TemporaryDirectory() as temp_dir:
      output_file = pathlib.Path(temp_dir) / 'RImpressionTrain.csv'
      exporter = RImpressionTrainExporter('-h click00 --port 9000')
      current_time = datetime.datetime(
        2026, 8, 14, 12, 0, 0, tzinfo=datetime.timezone.utc)
      with mock.patch(
          'rtbserver_utils.RImpressionTrainExporter.subprocess.run',
          side_effect=run,
      ), mock.patch(
          'rtbserver_utils.RImpressionTrainExporter.datetime.datetime',
      ) as datetime_mock:
        datetime_mock.now.return_value = current_time
        date_from = exporter.export(output_file, 100, 86400)

      self.assertEqual(date_from, '2026-08-12')
      self.assertEqual(output_file.read_text(), 'label,Device\n1,Mobile\n')

    self.assertEqual(calls[0][:5], [
      'clickhouse-client',
      '-h',
      'click00',
      '--port',
      '9000',
    ])
    self.assertIn("WHERE timestamp < '2026-08-13 12:00:00'", calls[0][-1])
    self.assertIn("WHERE timestamp >= '2026-08-12'", calls[1][-1])
    self.assertIn("timestamp < '2026-08-13 12:00:00'", calls[1][-1])
    self.assertIn('ORDER BY timestamp DESC', calls[1][-1])
    for field in ('url', 'etag', 'geo_ch', 'user_ch', 'ssp_tag_id'):
      self.assertIn(
        "replaceAll(replaceAll(ifNull(" + field +
        ", ''), char(13), ' '), char(10), ' ')",
        calls[1][-1])
    self.assertIn("char(10), ' ') AS SSP_Tag_ID", calls[1][-1])
    self.assertIn("ifNull(toString(ssp_ctr), '') AS SSP_CTR", calls[1][-1])
    self.assertIn(
      "ifNull(toString(ssp_viewability), '') AS SSP_Viewability",
      calls[1][-1])
    self.assertIn("ifNull(toString(ssp_vtr), '') AS SSP_VTR", calls[1][-1])
    self.assertIn('LIMIT 100 FORMAT CSVWithNames', calls[1][-1])

  def test_empty_table(self):
    result = subprocess.CompletedProcess([], 0, stdout='')
    exporter = RImpressionTrainExporter('')
    with mock.patch(
        'rtbserver_utils.RImpressionTrainExporter.subprocess.run',
        return_value=result,
    ):
      with self.assertRaisesRegex(RuntimeError, 'contains no rows'):
        exporter.export('/tmp/unused-RImpressionTrain.csv', 100, 86400)

  def test_failed_export_preserves_previous_sample(self):
    responses = [
      subprocess.CompletedProcess([], 0, stdout='2026-08-13\t100\n'),
      subprocess.CalledProcessError(1, []),
    ]
    exporter = RImpressionTrainExporter('')

    with tempfile.TemporaryDirectory() as temp_dir:
      output_file = pathlib.Path(temp_dir) / 'RImpressionTrain.csv'
      output_file.write_text('previous sample\n')
      with mock.patch(
          'rtbserver_utils.RImpressionTrainExporter.subprocess.run',
          side_effect=responses,
      ):
        with self.assertRaises(subprocess.CalledProcessError):
          exporter.export(output_file, 100, 86400)

      self.assertEqual(output_file.read_text(), 'previous sample\n')
      self.assertEqual(
        list(output_file.parent.glob('RImpressionTrain.csv.*.tmp')),
        [])

  def test_export_chunks_keeps_only_current_chunk(self):
    process = self.ProcessStub([
      b'0,Desktop\n',
      b'1,Mobile\n',
      b'0,Tablet\n',
    ])
    exporter = RImpressionTrainExporter('')

    with tempfile.TemporaryDirectory() as temp_dir, mock.patch(
        'rtbserver_utils.RImpressionTrainExporter.subprocess.Popen',
        return_value=process,
    ) as popen:
      chunks = exporter.export_chunks(
        temp_dir,
        'train',
        3,
        2,
        '2026-08-01',
        '2026-08-02',
        offset_rows=7)
      first_path, first_rows = next(chunks)
      self.assertEqual(2, first_rows)
      self.assertTrue(first_path.exists())
      second_path, second_rows = next(chunks)
      self.assertEqual(1, second_rows)
      self.assertFalse(first_path.exists())
      self.assertTrue(second_path.exists())
      with self.assertRaises(StopIteration):
        next(chunks)
      self.assertFalse(second_path.exists())
      self.assertEqual(0, process.return_code)
      query = popen.call_args.args[0][-1]
      self.assertIn('ORDER BY timestamp DESC, request_id DESC', query)
      self.assertLess(
        query.index('ORDER BY timestamp DESC'),
        query.index('LIMIT 3 OFFSET 7 FORMAT CSVWithNames'))

  def test_export_chunks_drains_unexpected_csv_tail_before_wait(self):
    process = self.ProcessStub([
      b'0,"first line\n',
      b'second line"\n',
    ], require_drained=True)
    exporter = RImpressionTrainExporter('')

    with tempfile.TemporaryDirectory() as temp_dir, mock.patch(
        'rtbserver_utils.RImpressionTrainExporter.subprocess.Popen',
        return_value=process,
    ):
      chunks = exporter.export_chunks(
        temp_dir,
        'train',
        1,
        1,
        '2026-08-01',
        '2026-08-02')
      with self.assertRaisesRegex(
          RuntimeError,
          'bytes after the expected 1 CSV rows'):
        next(chunks)

      self.assertEqual(0, process.return_code)
      self.assertEqual([], list(pathlib.Path(temp_dir).iterdir()))

  def test_training_partitions_are_disjoint(self):
    exporter = RImpressionTrainExporter('')
    source_partitions = set()
    for partition_index in range(30):
      condition = exporter.training_partition_condition(partition_index, 30)
      values = condition.split('IN (', 1)[1].rstrip(')')
      partitions = {int(value) for value in values.split(',')}
      self.assertFalse(source_partitions.intersection(partitions))
      source_partitions.update(partitions)

    self.assertEqual(set(range(2, 100)), source_partitions)

  def test_selects_active_campaigns_above_strict_impression_threshold(self):
    result = subprocess.CompletedProcess(
      [],
      0,
      stdout='123\t100001\t2041\n456\t250000\t5102\n')
    exporter = RImpressionTrainExporter('')
    with mock.patch(
        'rtbserver_utils.RImpressionTrainExporter.subprocess.run',
        return_value=result) as run:
      campaigns = exporter.eligible_campaigns(
        '2026-07-01',
        '2026-08-25 00:00:00',
        14 * 24 * 60 * 60,
        100000,
        training_extra_condition="timestamp < '2026-08-20'",
        validation_extra_condition="timestamp >= '2026-08-20'")

    self.assertEqual([
      (123, 100001, 2041),
      (456, 250000, 5102),
    ], campaigns)
    query = run.call_args.args[0][-1]
    self.assertIn('subtractSeconds(', query)
    self.assertIn('1209600', query)
    self.assertIn('HAVING training_impressions > 100000', query)
    self.assertIn('AS validation_impressions', query)
    self.assertIn("timestamp < '2026-08-20'", query)
    self.assertIn("timestamp >= '2026-08-20'", query)
    self.assertIn('campaign_id IN (', query)
    self.assertEqual(2, query.count('campaign_id > 0'))

  def test_campaign_condition_uses_database_campaign_id(self):
    self.assertEqual(
      'campaign_id = 236995',
      RImpressionTrainExporter.campaign_condition(236995))

  def test_ssp_ctr_export_uses_soft_label_and_only_requested_rows(self):
    query = RImpressionTrainExporter._export_query(
      '2026-08-01',
      '2026-08-02',
      100,
      RImpressionTrainExporter.ssp_ctr_condition(),
      7,
      'ssp_ctr')

    self.assertIn('assumeNotNull(ssp_ctr) AS label', query)
    self.assertIn('AND (ssp_ctr IS NOT NULL)', query)
    self.assertIn('ORDER BY timestamp DESC, request_id DESC', query)
    self.assertIn('LIMIT 100 OFFSET 7 FORMAT CSVWithNames', query)

  def test_ssp_ctr_logloss_uses_actual_clicks_on_ordered_slice(self):
    result = subprocess.CompletedProcess([], 0, stdout='0.012345\n')
    exporter = RImpressionTrainExporter('')
    with mock.patch(
        'rtbserver_utils.RImpressionTrainExporter.subprocess.run',
        return_value=result,
    ) as run:
      value = exporter.ssp_ctr_logloss(
        '2026-08-01',
        '2026-08-02',
        300,
        'sipHash64(request_id) % 100 IN (0,1)',
        600)

    self.assertEqual(0.012345, value)
    query = run.call_args.args[0][-1]
    self.assertIn('click_timestamp IS NOT NULL AS clicked', query)
    self.assertIn('assumeNotNull(ssp_ctr) AS score', query)
    self.assertIn('ssp_ctr IS NOT NULL', query)
    self.assertIn('sipHash64(request_id) % 100 IN (0,1)', query)
    self.assertIn('greatest(least(score, 1 - 1e-15), 1e-15)', query)
    self.assertIn('ORDER BY timestamp DESC, request_id DESC', query)
    self.assertIn('LIMIT 300 OFFSET 600', query)

  def test_required_source_rows_accounts_for_validation_partitions(self):
    self.assertEqual(
      396122449,
      RImpressionTrainExporter.required_source_rows(300000000, 1800000))

  def test_fit_row_counts_scales_training_and_validation_together(self):
    self.assertEqual(
      (150000000, 900000),
      RImpressionTrainExporter.fit_row_counts(
        300000000,
        1800000,
        198061225))

  def test_fit_row_counts_keeps_requested_sizes_when_source_is_sufficient(self):
    self.assertEqual(
      (300000000, 1800000),
      RImpressionTrainExporter.fit_row_counts(
        300000000,
        1800000,
        396122449))


if __name__ == '__main__':
  unittest.main()
