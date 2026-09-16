#!/usr/bin/env python3

import importlib.util
import io
import pathlib
import tempfile
import unittest
from contextlib import redirect_stderr


REPOSITORY_ROOT = pathlib.Path(__file__).resolve().parents[2]


def load_stat_collector():
  module_path = REPOSITORY_ROOT / 'bin' / 'StatCollector.py'
  spec = importlib.util.spec_from_file_location('StatCollector', module_path)
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


STAT_COLLECTOR = load_stat_collector()


class StatCollectorTest(unittest.TestCase):
  def config_for(self, queue_path):
    return {
      'measurement': 'rtb_queue',
      'period_seconds': 60,
      'queues': [{
        'service': 'ExpressionMatcher',
        'name': 'RequestBasicChannels',
        'path': str(queue_path),
      }],
    }

  def test_collects_direct_and_intermediate_queue_metrics(self):
    with tempfile.TemporaryDirectory() as temporary_directory:
      queue_path = pathlib.Path(temporary_directory) / 'RequestBasicChannels'
      intermediate_path = queue_path / 'Intermediate' / 'batch'
      intermediate_path.mkdir(parents = True)
      (queue_path / 'direct.log').write_bytes(b'direct')
      (intermediate_path / 'queued.log').write_bytes(b'queued')

      metrics = STAT_COLLECTOR.collect(self.config_for(queue_path), now = 1000)

    self.assertEqual(len(metrics), 1)
    metric = metrics[0]
    self.assertIn('rtb_queue,service=ExpressionMatcher,queue=RequestBasicChannels ', metric)
    self.assertIn('file_count=1i', metric)
    self.assertIn('intermediate_file_count=1i', metric)
    self.assertIn('disk_bytes=', metric)
    self.assertIn('intermediate_disk_bytes=', metric)
    self.assertIn('collection_success=1i', metric)

  def test_reports_failed_queue_without_fabricating_zero_values(self):
    errors = io.StringIO()
    config = self.config_for(pathlib.Path('/missing/queue'))

    with redirect_stderr(errors):
      metrics = STAT_COLLECTOR.collect(config)

    self.assertEqual(
      metrics,
      ['rtb_queue,service=ExpressionMatcher,queue=RequestBasicChannels collection_success=0i'])
    self.assertIn('queue directory does not exist', errors.getvalue())

  def test_escapes_influx_tag_values(self):
    line = STAT_COLLECTOR.format_metric(
      'rtb_queue',
      {'service': 'Request Info', 'queue': 'Click,Retry'},
      {'file_count': 1})

    self.assertEqual(
      line,
      'rtb_queue,service=Request\\ Info,queue=Click\\,Retry file_count=1i')

  def test_rejects_invalid_period(self):
    with tempfile.TemporaryDirectory() as temporary_directory:
      config_path = pathlib.Path(temporary_directory) / 'config.json'
      config_path.write_text(
        '{"period_seconds": 0, "queues": [{"service": "service", '
        '"name": "queue", "path": "/tmp"}]}')

      with self.assertRaisesRegex(STAT_COLLECTOR.ConfigError, 'positive integer'):
        STAT_COLLECTOR.load_config(str(config_path))

  def test_collects_process_cpu_and_host_iowait_counters(self):
    with tempfile.TemporaryDirectory() as temporary_directory:
      proc_root = pathlib.Path(temporary_directory) / 'proc'
      process_path = proc_root / '1000'
      process_path.mkdir(parents = True)
      (process_path / 'task' / '1000').mkdir(parents = True)
      (process_path / 'cmdline').write_bytes(
        b'/opt/foros/server/bin/RequestInfoManager\0--config\0')
      (process_path / 'stat').write_text(
        '1000 (RequestInfoManager) S 0 0 0 0 0 0 0 0 0 0 25 10')
      proc_stat_path = proc_root / 'stat'
      proc_stat_path.write_text('cpu  100 0 50 100 25 0 0 0 0 0\n')

      metrics = STAT_COLLECTOR.collect_cpu_metrics(
        {'processes': [{'service': 'RequestInfoManager', 'command': 'RequestInfoManager'}]},
        proc_root = str(proc_root),
        proc_stat_path = str(proc_stat_path),
        clock_ticks = 10)

    self.assertEqual(metrics, [
      'rtb_cpu_time,scope=process,service=RequestInfoManager,mode=user '
        'process_count=1i,collection_success=1i,cpu_time_seconds_total=2.5',
      'rtb_cpu_time,scope=process,service=RequestInfoManager,mode=system '
        'process_count=1i,collection_success=1i,cpu_time_seconds_total=1.0',
      'rtb_threads,scope=process,service=RequestInfoManager '
        'process_count=1i,thread_count=1i,collection_success=1i',
      'rtb_cpu_time,scope=host,mode=iowait '
        'cpu_time_seconds_total=2.5,collection_success=1i',
    ])

  def test_collects_managed_process_cpu_and_threads(self):
    with tempfile.TemporaryDirectory() as temporary_directory:
      server_root = pathlib.Path(temporary_directory) / 'server'
      binary_path = server_root / 'bin' / 'FCGIServer'
      binary_path.parent.mkdir(parents = True)
      binary_path.write_bytes(b'')
      instance_root = pathlib.Path(temporary_directory) / 'config' / 'colo' / 'cluster' / 'host'
      instance_root.mkdir(parents = True)
      config_path = instance_root / 'FCGIAdServerConfig.xml'
      config_path.write_bytes(b'')

      proc_root = pathlib.Path(temporary_directory) / 'proc'
      process_path = proc_root / '1001'
      process_path.mkdir(parents = True)
      (process_path / 'cmdline').write_bytes(
        str(binary_path).encode() + b'\0' + str(config_path).encode() + b'\0')
      (process_path / 'exe').symlink_to(binary_path)
      (process_path / 'task' / '1001').mkdir(parents = True)
      (process_path / 'task' / '1002').mkdir(parents = True)
      (process_path / 'stat').write_text(
        '1001 (FCGIServer) S 0 0 0 0 0 0 0 0 0 0 30 20')
      other_instance_root = pathlib.Path(temporary_directory) / 'config' / 'other-colo'
      other_instance_root.mkdir(parents = True)
      other_config_path = other_instance_root / 'FCGITrackServerConfig.xml'
      other_config_path.write_bytes(b'')
      other_process_path = proc_root / '1002'
      other_process_path.mkdir(parents = True)
      (other_process_path / 'cmdline').write_bytes(
        str(binary_path).encode() + b'\0' + str(other_config_path).encode() + b'\0')
      (other_process_path / 'exe').symlink_to(binary_path)
      (other_process_path / 'task' / '1002').mkdir(parents = True)
      (other_process_path / 'stat').write_text(
        '1002 (FCGIServer) S 0 0 0 0 0 0 0 0 0 0 10 20')
      proc_stat_path = proc_root / 'stat'
      proc_stat_path.write_text('cpu  100 0 50 100 25 0 0 0 0 0\n')

      metrics = STAT_COLLECTOR.collect_cpu_metrics(
        {
          'processes': [],
          'process_roots': [str(server_root)],
          'instance_roots': [str(instance_root)],
        },
        proc_root = str(proc_root),
        proc_stat_path = str(proc_stat_path),
        clock_ticks = 10)

    self.assertEqual(metrics, [
      'rtb_cpu_time,scope=process,service=FCGIAdServer,mode=user '
        'process_count=1i,collection_success=1i,cpu_time_seconds_total=3.0',
      'rtb_cpu_time,scope=process,service=FCGIAdServer,mode=system '
        'process_count=1i,collection_success=1i,cpu_time_seconds_total=2.0',
      'rtb_threads,scope=process,service=FCGIAdServer '
        'process_count=1i,thread_count=2i,collection_success=1i',
      'rtb_cpu_time,scope=host,mode=iowait '
        'cpu_time_seconds_total=2.5,collection_success=1i',
    ])


if __name__ == '__main__':
  unittest.main()
