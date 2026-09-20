#!/usr/bin/python3
"""Apply an explicitly enabled frontend network manifest; never flush shared rule tables."""
import argparse
import difflib
import fcntl
import hashlib
import ipaddress
import json
import os
from pathlib import Path
import re
import shlex
import socket
import subprocess
import sys
import tempfile


def run(*args, **kwargs):
  result = subprocess.run(args, input=kwargs.get('data'), stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, universal_newlines=True)
  if result.returncode and not kwargs.get('optional', False):
    raise RuntimeError('{}: {}'.format(' '.join(args), result.stderr.strip()))
  return result.stdout.strip() if result.returncode == 0 else None


def select_host(config, hostname, optional=False):
  names = [name for name in config['hosts'] if name == hostname]
  if not names:
    names = [name for name in config['hosts'] if name.split('.')[0] == hostname.split('.')[0]
             and ('.' not in name or '.' not in hostname)]
  if not names and optional:
    return None
  if len(names) != 1:
    raise ValueError('Host is absent or ambiguous in frontendNetwork: ' + hostname)
  return config['hosts'][names[0]]


def validate(config, host):
  if config.get('version') != 1:
    raise ValueError('Unsupported manifest version')
  interface = host['interface']
  if not re.fullmatch(r'[A-Za-z0-9_-]{1,15}', interface) or interface == 'lo':
    raise ValueError('Invalid interface')
  source = str(ipaddress.IPv4Address(host['arp_source_ip']))
  addresses, redirects = set(), {}
  for rule in config['redirects']:
    address = str(ipaddress.IPv4Address(rule['address']))
    port, target = rule['port'], rule['internal_port']
    if any(type(p) is not int or not 1 <= p <= 65535 for p in (port, target)):
      raise ValueError('Invalid redirect port')
    key = (address, port)
    if key in redirects and redirects[key] != target:
      raise ValueError('Conflicting redirects')
    redirects[key] = target
    addresses.add(address)
  if not addresses or source in addresses:
    raise ValueError('Missing addresses or ARP source equals externalAddress')
  if 'default_route' in config:
    ipaddress.IPv4Address(config['default_route']['gateway'])
    if type(config['default_route']['onlink']) is not bool:
      raise ValueError('Invalid onlink')
  return sorted(addresses)


def rules(config, host, prefix):
  source = host['arp_source_ip']
  incoming, outgoing, nat = prefix + 'I', prefix + 'O', prefix + 'N'
  arp_rules, nat_rules = [], []
  for address in sorted({r['address'] for r in config['redirects']}):
    arp_rules.extend([
      '-A {} --opcode Request -d {} -j DROP'.format(incoming, address),
      '-A {} --opcode Request -s {} -j mangle --mangle-ip-s {}'.format(
        outgoing, address, source),
      '-A {} --opcode Reply -s {} -j DROP'.format(outgoing, address)])
  for rule in config['redirects']:
    if rule['port'] != rule['internal_port']:
      nat_rules.append('-A {} -d {}/32 -p tcp --dport {} -j REDIRECT --to-ports {}'.format(
        nat, rule['address'], rule['port'], rule['internal_port']))
  # ARP suppression must cover every interface: Linux can answer for another interface's address.
  # LVS-DR packets can arrive on the private interface, not the interface owning the VIP.
  return (([incoming, outgoing], ['-A INPUT -j ' + incoming, '-A OUTPUT -j ' + outgoing],
           arp_rules), ([nat], ['-A PREROUTING -j ' + nat], nat_rules))


def merge_table(saved, chains, jumps, additions, remove=False):
  """Replace only our chains in a complete save image, retaining unrelated rules and policies."""
  headers, declarations, existing = [], [], []
  for line in saved.splitlines():
    if not line or line.startswith('#') or line == 'COMMIT':
      continue
    if line.startswith('*'):
      headers.append(line)
    elif line.startswith(':'):
      if line.split()[0][1:] not in chains:
        declarations.append(line)
    else:
      words = shlex.split(line)
      owned = len(words) > 1 and words[0] == '-A' and words[1] in chains
      owned |= any(words[i] in ('-j', '-g') and words[i + 1] in chains
                   for i in range(len(words) - 1))
      if not owned:
        existing.append(line)
  if len(headers) != 1:
    raise ValueError('Expected one saved firewall table')
  owned = [] if remove else [':' + c + ' - [0:0]' for c in chains]
  return '\n'.join(headers + declarations + owned +
                   jumps + existing + additions + ['COMMIT', ''])


def sysctls(host, current=()):
  interface = host['interface']
  values = {'net.ipv4.conf.all.arp_ignore': 2, 'net.ipv4.conf.all.arp_announce': 2,
            'net.ipv4.conf.all.rp_filter': 0, 'net.ipv4.conf.default.rp_filter': 0,
            'net.ipv4.conf.' + interface + '.arp_ignore': 2,
            'net.ipv4.conf.' + interface + '.arp_announce': 2,
            'net.ipv4.conf.' + interface + '.rp_filter': 2}
  for link in current:
    if any(a['local'] == host['arp_source_ip'] for a in link.get('addr_info', [])):
      values['net.ipv4.conf.' + link['ifname'] + '.rp_filter'] = 2
  return values


def preflight(config, host, addresses, previous):
  interface = host['interface']
  if '(nf_tables)' not in run('arptables', '--version'):
    raise ValueError('This implementation requires the transactional arptables nft backend')
  current = json.loads(run('ip', '-j', '-4', 'address', 'show'))
  if not any(link['ifname'] == interface for link in current):
    raise ValueError('Missing interface: ' + interface)
  local = [(link['ifname'], addr) for link in current for addr in link.get('addr_info', [])]
  if sum(addr['local'] == host['arp_source_ip'] for device, addr in local) != 1:
    raise ValueError('ARP source must be assigned to exactly one interface on this host')
  for device, addr in local:
    if addr['local'] in addresses and (device != interface or addr['prefixlen'] != 32):
      raise ValueError('externalAddress already exists on another interface or with another mask')
  # Existing boot/shutdown writers must be migrated explicitly, never disabled by the RPM.
  for service in ('arptables', 'iptables', 'firewalld'):
    active = run('systemctl', 'is-active', service, optional=True)
    enabled = run('systemctl', 'is-enabled', service, optional=True)
    if active == 'active' or enabled in ('enabled', 'enabled-runtime', 'linked', 'linked-runtime'):
      raise ValueError('Migrate the competing firewall service first: ' + service)
  for path in (Path('/etc/rc.local'), Path('/etc/rc.d/rc.local')):
    if path.exists():
      for line in path.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith('#'):
          if 'arptables' in line or any(address in line for address in addresses):
            raise ValueError('Migrate legacy frontend commands from ' + str(path))
          if 'ip route' in line and 'default' in line and interface in line:
            raise ValueError('Migrate legacy default route from ' + str(path))
  if previous:
    if previous['host'] != host:
      raise ValueError('Interface/ARP source changes require a separate migration')
    old = {r['address'] for r in previous['config']['redirects']}
    if not old.issubset(addresses):
      raise ValueError('Removing live external addresses requires a separate migration')
    if 'default_route' in previous['config'] and 'default_route' not in config:
      raise ValueError('Removing a managed default route requires a separate migration')
  defaults = run('ip', '-4', 'route', 'show', 'default').splitlines()
  if 'default_route' in config and (len(defaults) > 1 or any('nexthop' in d for d in defaults)):
    raise ValueError('Multiple/multipath default routes require a separate migration')
  return current, defaults


def write_state(path, value):
  temporary = path.with_name(path.name + '.new')
  with temporary.open('w') as stream:
    json.dump(value, stream, indent=2)
    stream.flush()
    os.fsync(stream.fileno())
  temporary.replace(path)


def route_args(config, host, defaults):
  if 'default_route' not in config:
    return []
  route = config['default_route']
  args = ['default', 'via', route['gateway'], 'dev', host['interface']]
  if defaults and 'metric' in shlex.split(defaults[0]):
    old = shlex.split(defaults[0])
    args.extend(['metric', old[old.index('metric') + 1]])
  if route['onlink']:
    args.append('onlink')
  return args


def route_identity(line):
  """Only recognize simple routes we write; preserve routes with extra attributes."""
  words = shlex.split(line)
  if not words or words.pop(0) != 'default':
    return None
  values = {'metric': '0', 'proto': 'boot'}
  while words:
    key = words.pop(0)
    if key == 'linkdown':
      continue
    if key == 'onlink':
      values[key] = True
    elif key in ('via', 'dev', 'metric', 'proto') and words:
      values[key] = words.pop(0)
    else:
      return None
  return values


def remember_ownership(config, host, current, defaults, before, state):
  path = state / 'ownership.json'
  if path.exists():
    owned = json.loads(path.read_text())
    if owned['host'] != host:
      raise ValueError('Pending network ownership requires a separate migration')
  else:
    legacy = (state / 'applied.json').exists()
    owned = {'host': host, 'config': config, 'routes': [], 'sysctl_written': {},
             'sysctl_restore': {} if legacy else before['sysctl'], 'default_restore': []}
    expected = route_args(config, host, defaults)
    if not legacy and expected:
      identity = route_identity(' '.join(expected))
      owned['default_restore'] = [r for r in defaults if route_identity(r) != identity]
    if legacy:
      old = json.loads((state / 'applied.json').read_text())
      old_route = route_args(old['config'], old['host'], defaults)
      if old_route:
        owned['routes'].append(' '.join(old_route))
      print('Adopting legacy state; original sysctl/default route values are unknown.',
            file=sys.stderr)
  previous_addresses = {r['address'] for r in owned['config']['redirects']}
  if not previous_addresses.issubset({r['address'] for r in config['redirects']}):
    raise ValueError('Pending external addresses require a separate migration')
  if 'default_route' in owned['config'] and 'default_route' not in config:
    raise ValueError('Pending default route requires a separate migration')
  owned['config'] = config
  owned['sysctl_written'].update({k: str(v) for k, v in sysctls(host, current).items()})
  expected = route_args(config, host, defaults)
  if expected and not owned['routes']:
    identity = route_identity(' '.join(expected))
    owned['default_restore'] = [r for r in defaults if route_identity(r) != identity]
  if expected and ' '.join(expected) not in owned['routes']:
    owned['routes'].append(' '.join(expected))
  # Journal before any mutation, including a first apply that may fail halfway through.
  write_state(path, owned)
  return owned


def remove(state, prefix):
  ownership = state / 'ownership.json'
  applied = state / 'applied.json'
  if not ownership.exists() and not applied.exists():
    print('No managed frontend network state; nothing to remove.')
    return
  previous = json.loads((ownership if ownership.exists() else applied).read_text())
  config, host = previous['config'], previous['host']
  addresses = validate(config, host)
  current = json.loads(run('ip', '-j', '-4', 'address', 'show'))
  defaults = run('ip', '-4', 'route', 'show', 'default').splitlines()
  if not ownership.exists():
    previous = remember_ownership(config, host, current, defaults, {}, state)
  local = [(link['ifname'], addr) for link in current for addr in link.get('addr_info', [])
           if addr['local'] in addresses]
  if any(dev != host['interface'] or addr['prefixlen'] != 32 for dev, addr in local):
    raise ValueError('Managed VIP moved or changed mask; retaining ARP protection')
  if '(nf_tables)' not in run('arptables', '--version'):
    raise ValueError('Removal requires the transactional arptables nft backend')
  for dev, addr in local:
    run('ip', 'address', 'del', addr['local'] + '/32', 'dev', dev)
  current = json.loads(run('ip', '-j', '-4', 'address', 'show'))
  if any(a['local'] in addresses for link in current for a in link.get('addr_info', [])):
    raise ValueError('Managed VIP still present; retaining ARP protection')
  identities = [route_identity(r) for r in previous['routes']]
  # Removing the last IPv4 address can already remove this interface's default route.
  for route in run('ip', '-4', 'route', 'show', 'default').splitlines():
    identity = route_identity(route)
    if identity is not None and identity in identities:
      run('ip', '-4', 'route', 'del', *shlex.split(route))
  if previous['default_restore']:
    remaining = run('ip', '-4', 'route', 'show', 'default').splitlines()
    if not remaining:
      for route in previous['default_restore']:
        run('ip', '-4', 'route', 'add', *shlex.split(route))
  # Do not restore old firewall snapshots: they may contain obsolete third-party rules.
  arp, nat = rules(config, host, prefix)
  for tool, arguments, entries in [('iptables', ['-t', 'nat'], nat), ('arptables', [], arp)]:
    saved = run(tool + '-save', *arguments) + '\n'
    desired = merge_table(saved, entries[0], [], [], remove=True)
    if tool == 'iptables':
      run('iptables-restore', '--test', data=desired)
    run(tool + '-restore', data=desired)
  for key, value in previous['sysctl_restore'].items():
    current_value = run('sysctl', '-n', key, optional=True)
    if current_value == previous['sysctl_written'].get(key):
      run('sysctl', '-w', '{}={}'.format(key, value))
  # Keep audit backups, but no active ownership marker. Retries after partial removal are safe.
  for path in (applied, ownership):
    if path.exists():
      path.unlink()
  print('Frontend network removed; audit backups retained in ' + str(state))


def apply(config, host, addresses, current, defaults, images, state):
  backup = Path(tempfile.mkdtemp(prefix='backup-', dir=str(state)))
  backup.chmod(0o700)
  before = {tool: saved for tool, saved, desired in images}
  before.update({'addresses': current, 'default_routes': defaults,
                 'sysctl': {key: run('sysctl', '-n', key) for key in sysctls(host, current)}})
  (backup / 'before.json').write_text(json.dumps(before, indent=2))
  (backup / 'requested.json').write_text(json.dumps({'host': host, 'config': config}, indent=2))
  print('Backup: ' + str(backup), flush=True)
  remember_ownership(config, host, current, defaults, before, state)
  existing = {addr['local'] for link in current for addr in link.get('addr_info', [])}
  added, route_attempted = [], False
  interface = host['interface']
  try:
    for tool, saved, desired in images:
      run(tool + '-restore', data=desired)
    for key, value in sysctls(host, current).items():
      run('sysctl', '-w', '{}={}'.format(key, value))
    run('ip', 'link', 'set', 'dev', interface, 'up')
    for address in addresses:
      if address not in existing:
        run('ip', 'address', 'add', address + '/32', 'dev', interface)
        added.append(address)
    if 'default_route' in config:
      route_attempted = True
      run('ip', '-4', 'route', 'replace', *route_args(config, host, defaults))
    write_state(state / 'applied.json', {'host': host, 'config': config})
  except Exception:
    # Retain ARP protection and safe sysctls, including if rollback itself fails.
    if route_attempted:
      if defaults:
        run('ip', '-4', 'route', 'replace', *shlex.split(defaults[0]))
      else:
        run('ip', '-4', 'route', 'del', 'default', 'via',
            config['default_route']['gateway'], 'dev', interface, optional=True)
    for address in added:
      run('ip', 'address', 'del', address + '/32', 'dev', interface)
    run('iptables-restore', data=before['iptables'])
    print('Apply failed; ARP protection retained. Inspect backup: ' + str(backup), file=sys.stderr)
    raise


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('action', choices=('check', 'diff', 'apply', 'install', 'remove'))
  parser.add_argument('colo')
  args = parser.parse_args()
  if not re.fullmatch(r'[a-z0-9][a-z0-9_-]*', args.colo):
    raise ValueError('Invalid colocation name')
  if os.geteuid() != 0:
    raise ValueError('Run as root, including read-only firewall checks')
  os.environ['PATH'] = '/usr/sbin:/usr/bin:/sbin:/bin'
  os.umask(0o077)
  state = Path('/var/lib/foros/frontend-network') / args.colo
  prefix = 'FFN' + hashlib.sha256(args.colo.encode()).hexdigest()[:8]
  if args.action == 'remove':
    with open('/run/lock/foros-frontend-network.lock', 'w') as lock:
      fcntl.flock(lock, fcntl.LOCK_EX)
      markers = list(state.parent.glob('*/applied.json'))
      markers.extend(state.parent.glob('*/ownership.json'))
      if any(other.parent != state for other in markers):
        raise ValueError('Another colocation has network state; migrate explicitly')
      remove(state, prefix)
    return
  config_path = Path('/etc/foros/frontend-network') / (args.colo + '.json')
  if config_path.stat().st_uid != 0 or config_path.stat().st_mode & 0o022:
    raise ValueError('Manifest must be root-owned and not group/world writable')
  config = json.loads(config_path.read_text())
  host = select_host(config, socket.gethostname(), optional=args.action == 'install')
  if host is None:
    if (state / 'applied.json').exists() or (state / 'ownership.json').exists():
      raise ValueError('Previously managed host removed from manifest; migrate explicitly')
    return
  addresses = validate(config, host)
  if args.action == 'install':
    # Do not hold the runtime lock while systemd starts another instance of this tool.
    run(sys.executable, os.path.realpath(__file__), 'check', args.colo)
    unit = 'foros-frontend-network-' + args.colo + '.service'
    run('systemctl', 'daemon-reload')
    run('systemctl', 'enable', unit)
    run('systemctl', 'reload-or-restart', unit)
    return
  state.mkdir(parents=True, exist_ok=True, mode=0o700)
  with open('/run/lock/foros-frontend-network.lock', 'w') as lock:
    fcntl.flock(lock, fcntl.LOCK_EX)
    markers = list(state.parent.glob('*/applied.json'))
    markers.extend(state.parent.glob('*/ownership.json'))
    for other in markers:
      if other.parent != state:
        raise ValueError('Another colocation already manages frontend networking: ' + str(other))
    previous_file = state / 'applied.json'
    previous = json.loads(previous_file.read_text()) if previous_file.exists() else None
    current, defaults = preflight(config, host, addresses, previous)
    arp, nat = rules(config, host, prefix)
    images = []
    for tool, table, entries in (('arptables', 'filter', arp), ('iptables', 'nat', nat)):
      saved = run(tool + '-save', *(['-t', table] if tool == 'iptables' else [])) + '\n'
      images.append((tool, saved, merge_table(saved, *entries)))
    run('iptables-restore', '--test', data=images[1][2])
    for key in sysctls(host, current):
      run('sysctl', '-n', key)
    if args.action == 'diff':
      for tool, saved, desired in images:
        print(''.join(difflib.unified_diff(saved.splitlines(True), desired.splitlines(True),
                                         tool + ':current', tool + ':desired')))
      print(json.dumps({'addresses': addresses, 'interface': host['interface'],
                        'sysctl': sysctls(host, current),
                        'default_route': config.get('default_route'),
                        'current_default_routes': defaults}, indent=2))
    elif args.action == 'apply':
      apply(config, host, addresses, current, defaults, images, state)
    print('Frontend network {}: OK'.format(args.action))


if __name__ == '__main__':
  try:
    main()
  except (ValueError, KeyError, OSError, RuntimeError) as error:
    sys.exit('Frontend network failed: ' + str(error))
