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


def select_host(config, hostname):
  names = [name for name in config['hosts'] if name == hostname]
  if not names:
    names = [name for name in config['hosts'] if name.split('.')[0] == hostname.split('.')[0]
             and ('.' not in name or '.' not in hostname)]
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


def merge_table(saved, chains, jumps, additions):
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
  return '\n'.join(headers + declarations + [':' + c + ' - [0:0]' for c in chains] +
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


def apply(config, host, addresses, current, defaults, images, state):
  backup = Path(tempfile.mkdtemp(prefix='backup-', dir=str(state)))
  backup.chmod(0o700)
  before = {tool: saved for tool, saved, desired in images}
  before.update({'addresses': current, 'default_routes': defaults,
                 'sysctl': {key: run('sysctl', '-n', key) for key in sysctls(host, current)}})
  (backup / 'before.json').write_text(json.dumps(before, indent=2))
  (backup / 'requested.json').write_text(json.dumps({'host': host, 'config': config}, indent=2))
  print('Backup: ' + str(backup), flush=True)
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
      route = config['default_route']
      args = ['default', 'via', route['gateway'], 'dev', interface]
      if defaults and 'metric' in shlex.split(defaults[0]):
        old = shlex.split(defaults[0])
        args.extend(['metric', old[old.index('metric') + 1]])
      if route['onlink']:
        args.append('onlink')
      route_attempted = True
      run('ip', '-4', 'route', 'replace', *args)
    temporary = state / 'applied.json.new'
    temporary.write_text(json.dumps({'host': host, 'config': config}, indent=2))
    temporary.replace(state / 'applied.json')
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
  parser.add_argument('action', choices=('check', 'diff', 'apply'))
  parser.add_argument('colo')
  args = parser.parse_args()
  if not re.fullmatch(r'[a-z0-9][a-z0-9_-]*', args.colo):
    raise ValueError('Invalid colocation name')
  if os.geteuid() != 0:
    raise ValueError('Run as root, including read-only firewall checks')
  os.environ['PATH'] = '/usr/sbin:/usr/bin:/sbin:/bin'
  os.umask(0o077)
  config_path = Path('/etc/foros/frontend-network') / (args.colo + '.json')
  if config_path.stat().st_uid != 0 or config_path.stat().st_mode & 0o022:
    raise ValueError('Manifest must be root-owned and not group/world writable')
  config = json.loads(config_path.read_text())
  host = select_host(config, socket.gethostname())
  addresses = validate(config, host)
  state = Path('/var/lib/foros/frontend-network') / args.colo
  state.mkdir(parents=True, exist_ok=True, mode=0o700)
  with open('/run/lock/foros-frontend-network.lock', 'w') as lock:
    fcntl.flock(lock, fcntl.LOCK_EX)
    for other in state.parent.glob('*/applied.json'):
      if other.parent != state:
        raise ValueError('Another colocation already manages frontend networking: ' + str(other))
    previous_file = state / 'applied.json'
    previous = json.loads(previous_file.read_text()) if previous_file.exists() else None
    current, defaults = preflight(config, host, addresses, previous)
    prefix = 'FFN' + hashlib.sha256(args.colo.encode()).hexdigest()[:8]
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
