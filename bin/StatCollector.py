#!/usr/bin/env python3

import argparse
import json
import os
import signal
import stat
import sys
import time


DEFAULT_PERIOD_SECONDS = 60
STOP_REQUESTED = False


class ConfigError(Exception):
  pass


def escape_identifier(value):
  return (str(value).replace('\\', '\\\\').replace(' ', '\\ ').replace(',', '\\,')
    .replace('=', '\\='))


def format_metric(measurement, tags, fields):
  encoded_tags = ','.join(
    escape_identifier(name) + '=' + escape_identifier(value)
    for name, value in tags.items())
  encoded_fields = ','.join(
    escape_identifier(name) + '=' + format_field(value)
    for name, value in fields.items())
  return escape_identifier(measurement) + ',' + encoded_tags + ' ' + encoded_fields


def format_field(value):
  if isinstance(value, int) and not isinstance(value, bool):
    return str(value) + 'i'
  return str(value)


def load_config(path):
  try:
    with open(path, 'r') as source:
      config = json.load(source)
  except (OSError, ValueError) as error:
    raise ConfigError('cannot read config ' + path + ': ' + str(error))

  if not isinstance(config, dict):
    raise ConfigError('config must be an object')

  measurement = config.get('measurement', 'rtb_queue')
  if not isinstance(measurement, str) or not measurement:
    raise ConfigError('measurement must be a non-empty string')

  period_seconds = config.get('period_seconds', DEFAULT_PERIOD_SECONDS)
  if not isinstance(period_seconds, int) or isinstance(period_seconds, bool) or period_seconds <= 0:
    raise ConfigError('period_seconds must be a positive integer')

  queues = config.get('queues')
  if not isinstance(queues, list):
    raise ConfigError('queues must be an array')

  validated_queues = []
  for queue in queues:
    if not isinstance(queue, dict):
      raise ConfigError('queue must be an object')

    service = queue.get('service')
    name = queue.get('name')
    queue_path = queue.get('path')
    if not all(isinstance(value, str) and value for value in (service, name, queue_path)):
      raise ConfigError('queue service, name and path must be non-empty strings')

    validated_queues.append({
      'service': service,
      'name': name,
      'path': queue_path,
    })

  processes = config.get('processes', [])
  if not isinstance(processes, list):
    raise ConfigError('processes must be an array')

  validated_processes = []
  for process in processes:
    if not isinstance(process, dict):
      raise ConfigError('process must be an object')

    service = process.get('service')
    command = process.get('command')
    if not all(isinstance(value, str) and value for value in (service, command)):
      raise ConfigError('process service and command must be non-empty strings')

    validated_processes.append({
      'service': service,
      'command': command,
    })

  process_roots = config.get('process_roots', [])
  if not isinstance(process_roots, list):
    raise ConfigError('process_roots must be an array')
  if not all(isinstance(root, str) and root for root in process_roots):
    raise ConfigError('process_roots must contain non-empty strings')

  instance_roots = config.get('instance_roots', [])
  if not isinstance(instance_roots, list):
    raise ConfigError('instance_roots must be an array')
  if not all(isinstance(root, str) and root for root in instance_roots):
    raise ConfigError('instance_roots must contain non-empty strings')

  excluded_commands = config.get('excluded_commands', [])
  if not isinstance(excluded_commands, list):
    raise ConfigError('excluded_commands must be an array')
  if not all(isinstance(command, str) and command for command in excluded_commands):
    raise ConfigError('excluded_commands must contain non-empty strings')

  return {
    'measurement': measurement,
    'period_seconds': period_seconds,
    'queues': validated_queues,
    'processes': validated_processes,
    'process_roots': process_roots,
    'instance_roots': instance_roots,
    'excluded_commands': excluded_commands,
  }


def file_stats(path, recursive, now):
  file_count = 0
  oldest_file_age_seconds = 0

  for entry in os.scandir(path):
    entry_stat = entry.stat(follow_symlinks = False)
    if stat.S_ISREG(entry_stat.st_mode):
      file_count += 1
      oldest_file_age_seconds = max(
        oldest_file_age_seconds,
        max(0, int(now - entry_stat.st_mtime)))
    elif recursive and stat.S_ISDIR(entry_stat.st_mode):
      child_count, child_oldest_age = file_stats(entry.path, True, now)
      file_count += child_count
      oldest_file_age_seconds = max(oldest_file_age_seconds, child_oldest_age)

  return file_count, oldest_file_age_seconds


def disk_usage_bytes(path, seen_inodes = None):
  if seen_inodes is None:
    seen_inodes = set()

  path_stat = os.stat(path, follow_symlinks = False)
  inode = (path_stat.st_dev, path_stat.st_ino)
  disk_bytes = 0
  if inode not in seen_inodes:
    seen_inodes.add(inode)
    disk_bytes = path_stat.st_blocks * 512

  if stat.S_ISDIR(path_stat.st_mode):
    for entry in os.scandir(path):
      disk_bytes += disk_usage_bytes(entry.path, seen_inodes)

  return disk_bytes


def collect_queue(queue, now):
  queue_path = queue['path']
  if not os.path.isdir(queue_path):
    raise OSError('queue directory does not exist: ' + queue_path)

  file_count, oldest_file_age_seconds = file_stats(queue_path, False, now)
  intermediate_path = os.path.join(queue_path, 'Intermediate')
  intermediate_file_count = 0
  intermediate_oldest_file_age_seconds = 0
  intermediate_disk_bytes = 0
  if os.path.isdir(intermediate_path):
    intermediate_file_count, intermediate_oldest_file_age_seconds = file_stats(
      intermediate_path, True, now)
    intermediate_disk_bytes = disk_usage_bytes(intermediate_path)

  return {
    'file_count': file_count,
    'intermediate_file_count': intermediate_file_count,
    'disk_bytes': disk_usage_bytes(queue_path),
    'intermediate_disk_bytes': intermediate_disk_bytes,
    'oldest_file_age_seconds': max(
      oldest_file_age_seconds,
      intermediate_oldest_file_age_seconds),
    'collection_success': 1,
  }


def process_cpu_ticks_and_rss_pages(pid_path):
  with open(os.path.join(pid_path, 'stat'), 'r') as source:
    process_stat = source.read()

  close_parenthesis = process_stat.rfind(')')
  if close_parenthesis == -1:
    raise OSError('invalid process stat data')

  fields = process_stat[close_parenthesis + 1:].split()
  if len(fields) <= 21:
    raise OSError('incomplete process stat data')

  return int(fields[11]), int(fields[12]), int(fields[21])


def normalize_command(command):
  normalized = os.path.basename(command)
  if normalized.endswith('.py'):
    return normalized[:-3]
  return normalized


def process_arguments(pid_path):
  with open(os.path.join(pid_path, 'cmdline'), 'rb') as source:
    command_line = source.read()

  return [argument.decode('utf-8', 'replace')
    for argument in command_line.split(b'\0') if argument]


def process_command(pid_path):
  arguments = process_arguments(pid_path)
  if not arguments:
    raise OSError('empty process command line')
  return normalize_command(arguments[0])


def is_under_root(path, root):
  if not os.path.isabs(path):
    return False

  normalized_path = os.path.realpath(path)
  normalized_root = os.path.realpath(root)
  try:
    return os.path.commonpath((normalized_path, normalized_root)) == normalized_root
  except ValueError:
    return False


def managed_process_service(pid_path, process_roots, instance_roots):
  executable = None
  try:
    executable = os.readlink(os.path.join(pid_path, 'exe'))
  except OSError:
    pass

  try:
    arguments = process_arguments(pid_path)
  except OSError:
    arguments = []

  executable_candidates = [candidate for candidate in [executable] + arguments if candidate]
  command = next((
    normalize_command(candidate)
    for candidate in executable_candidates
    if any(is_under_root(candidate, root) for root in process_roots)), None)
  if command is None:
    return None

  instance_arguments = [
    argument for argument in arguments
    if any(is_under_root(argument, root) for root in instance_roots)]
  if instance_roots and not instance_arguments:
    return None

  for argument in instance_arguments:
    config_name = os.path.basename(argument)
    if config_name.endswith('Config.xml'):
      return config_name[:-len('Config.xml')]

  return command


def process_thread_count(pid_path):
  return sum(1 for entry in os.scandir(os.path.join(pid_path, 'task')) if entry.is_dir())


def collect_process_metrics(config, proc_root, clock_ticks, page_size):
  process_stats = {}
  services_by_command = {}
  for process in config['processes']:
    service = process['service']
    command = normalize_command(process['command'])
    process_stats.setdefault(service, {
      'process_count': 0,
      'rss_pages': 0,
      'system_cpu_ticks': 0,
      'thread_collection_success': 1,
      'thread_count': 0,
      'user_cpu_ticks': 0,
    })
    services_by_command.setdefault(command, set()).add(service)

  excluded_commands = set(
    normalize_command(command) for command in config.get('excluded_commands', []))
  process_roots = config.get('process_roots', [])
  instance_roots = config.get('instance_roots', [])

  for entry in os.scandir(proc_root):
    if not entry.name.isdigit() or not entry.is_dir(follow_symlinks = False):
      continue

    try:
      command = process_command(entry.path)
      user_ticks, system_ticks, rss_pages = process_cpu_ticks_and_rss_pages(entry.path)
    except (OSError, ValueError):
      continue

    services = set(services_by_command.get(command, []))
    managed_service = managed_process_service(entry.path, process_roots, instance_roots)
    if managed_service and managed_service not in excluded_commands:
      services.add(managed_service)
    if not services:
      continue

    try:
      thread_count = process_thread_count(entry.path)
    except OSError:
      thread_count = None

    for service in services:
      stats = process_stats.setdefault(service, {
        'process_count': 0,
        'rss_pages': 0,
        'system_cpu_ticks': 0,
        'thread_collection_success': 1,
        'thread_count': 0,
        'user_cpu_ticks': 0,
      })
      stats['process_count'] += 1
      stats['rss_pages'] += rss_pages
      stats['system_cpu_ticks'] += system_ticks
      stats['user_cpu_ticks'] += user_ticks
      if thread_count is None:
        stats['thread_collection_success'] = 0
      else:
        stats['thread_count'] += thread_count

  metrics = []
  for service in sorted(process_stats):
    stats = process_stats[service]
    fields = {
      'process_count': stats['process_count'],
      'collection_success': 1,
    }
    if stats['process_count']:
      fields['cpu_time_seconds_total'] = stats['user_cpu_ticks'] / float(clock_ticks)
    metrics.append(format_metric(
      'rtb_cpu_time',
      {'scope': 'process', 'service': service, 'mode': 'user'},
      fields))

    if stats['process_count']:
      fields['cpu_time_seconds_total'] = stats['system_cpu_ticks'] / float(clock_ticks)
    metrics.append(format_metric(
      'rtb_cpu_time',
      {'scope': 'process', 'service': service, 'mode': 'system'},
      fields))

    metrics.append(format_metric(
      'rtb_threads',
      {'scope': 'process', 'service': service},
      {
        'process_count': stats['process_count'],
        'thread_count': stats['thread_count'],
        'collection_success': stats['thread_collection_success'],
      }))

    metrics.append(format_metric(
      'rtb_rss',
      {'scope': 'process', 'service': service},
      {
        'process_count': stats['process_count'],
        'rss_bytes': stats['rss_pages'] * page_size,
        'collection_success': 1,
      }))

  return metrics


def host_iowait_cpu_ticks(proc_stat_path):
  with open(proc_stat_path, 'r') as source:
    for line in source:
      fields = line.split()
      if fields and fields[0] == 'cpu':
        if len(fields) <= 5:
          raise OSError('incomplete host cpu stat data')
        return int(fields[5])

  raise OSError('host cpu stat data not found')


def collect_cpu_metrics(
    config,
    proc_root = '/proc',
    proc_stat_path = '/proc/stat',
    clock_ticks = None,
    page_size = None):
  if clock_ticks is None:
    clock_ticks = os.sysconf(os.sysconf_names['SC_CLK_TCK'])
  if page_size is None:
    page_size = os.sysconf('SC_PAGE_SIZE')

  metrics = collect_process_metrics(config, proc_root, clock_ticks, page_size)

  try:
    iowait_cpu_ticks = host_iowait_cpu_ticks(proc_stat_path)
    iowait_fields = {
      'cpu_time_seconds_total': iowait_cpu_ticks / float(clock_ticks),
      'collection_success': 1,
    }
  except (OSError, ValueError):
    iowait_fields = {'collection_success': 0}

  metrics.append(format_metric(
    'rtb_cpu_time',
    {'scope': 'host', 'mode': 'iowait'},
    iowait_fields))
  return metrics


def collect(config, now = None, error_stream = None):
  if now is None:
    now = time.time()
  if error_stream is None:
    error_stream = sys.stderr

  metrics = []
  for queue in config['queues']:
    tags = {
      'service': queue['service'],
      'queue': queue['name'],
    }
    try:
      fields = collect_queue(queue, now)
    except OSError as error:
      print(
        'StatCollector: cannot collect ' + queue['service'] + '/' + queue['name'] + ': ' +
        str(error),
        file = error_stream)
      fields = {'collection_success': 0}

    metrics.append(format_metric(config['measurement'], tags, fields))

  if config.get('processes') or config.get('process_roots'):
    metrics.extend(collect_cpu_metrics(config))
  return metrics


def request_stop(signum, frame):
  del signum, frame
  global STOP_REQUESTED
  STOP_REQUESTED = True


def run_once(config, output_stream, error_stream):
  for metric in collect(config, error_stream = error_stream):
    print(metric, file = output_stream)
  output_stream.flush()


def run_loop(config, output_stream, error_stream):
  signal.signal(signal.SIGINT, request_stop)
  signal.signal(signal.SIGTERM, request_stop)

  while not STOP_REQUESTED:
    started_at = time.monotonic()
    run_once(config, output_stream, error_stream)
    remaining_seconds = config['period_seconds'] - (time.monotonic() - started_at)
    if remaining_seconds > 0:
      time.sleep(remaining_seconds)


def main(arguments = None):
  parser = argparse.ArgumentParser(
    description = 'Collect ExpressionMatcher and RequestInfoManager input queue metrics.')
  parser.add_argument('-c', '--config', required = True, help = 'JSON configuration path')
  parser.add_argument(
    '--loop',
    action = 'store_true',
    help = 'write metrics repeatedly using period_seconds from the configuration')
  options = parser.parse_args(arguments)

  try:
    config = load_config(options.config)
  except ConfigError as error:
    parser.error(str(error))

  if options.loop:
    run_loop(config, sys.stdout, sys.stderr)
  else:
    run_once(config, sys.stdout, sys.stderr)


if __name__ == '__main__':
  main()
