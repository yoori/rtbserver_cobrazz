#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile
import unittest
from unittest import mock

from rtbserver_utils.RImpressionTrainExporter import RImpressionTrainExporter


class RImpressionTrainExporterTest(unittest.TestCase):
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
      with mock.patch(
          'rtbserver_utils.RImpressionTrainExporter.subprocess.run',
          side_effect=run,
      ):
        date_from = exporter.export(output_file, 100)

      self.assertEqual(date_from, '2026-08-12')
      self.assertEqual(output_file.read_text(), 'label,Device\n1,Mobile\n')

    self.assertEqual(calls[0][:5], [
      'clickhouse-client',
      '-h',
      'click00',
      '--port',
      '9000',
    ])
    self.assertIn("WHERE timestamp >= '2026-08-12'", calls[1][-1])
    self.assertIn('LIMIT 100 FORMAT CSVWithNames', calls[1][-1])

  def test_empty_table(self):
    result = subprocess.CompletedProcess([], 0, stdout='')
    exporter = RImpressionTrainExporter('')
    with mock.patch(
        'rtbserver_utils.RImpressionTrainExporter.subprocess.run',
        return_value=result,
    ):
      with self.assertRaisesRegex(RuntimeError, 'contains no rows'):
        exporter.export('/tmp/unused-RImpressionTrain.csv', 100)

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
          exporter.export(output_file, 100)

      self.assertEqual(output_file.read_text(), 'previous sample\n')
      self.assertEqual(
        list(output_file.parent.glob('RImpressionTrain.csv.*.tmp')),
        [])


if __name__ == '__main__':
  unittest.main()
