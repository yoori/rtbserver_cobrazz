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
    def __init__(self, rows):
      self.args = ['clickhouse-client']
      self.stdout = io.BytesIO(
        b'label,Device\n' + b''.join(rows))
      self.return_code = None
      self.terminated = False

    def wait(self, timeout=None):
      del timeout
      self.return_code = 0
      return self.return_code

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
        '2026-08-02')
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
      self.assertIn('ORDER BY timestamp DESC', query)
      self.assertLess(
        query.index('ORDER BY timestamp DESC'),
        query.index('LIMIT 3 FORMAT CSVWithNames'))

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
        100000)

    self.assertEqual([
      (123, 100001, 2041),
      (456, 250000, 5102),
    ], campaigns)
    query = run.call_args.args[0][-1]
    self.assertIn('subtractSeconds(', query)
    self.assertIn('1209600', query)
    self.assertIn('HAVING training_impressions > 100000', query)
    self.assertIn('AS validation_impressions', query)
    self.assertIn('campaign_id IN (', query)
    self.assertEqual(2, query.count('campaign_id > 0'))

  def test_campaign_condition_uses_database_campaign_id(self):
    self.assertEqual(
      'campaign_id = 236995',
      RImpressionTrainExporter.campaign_condition(236995))

  def test_required_source_rows_accounts_for_validation_partitions(self):
    self.assertEqual(
      306122449,
      RImpressionTrainExporter.required_source_rows(300000000, 1800000))


if __name__ == '__main__':
  unittest.main()
