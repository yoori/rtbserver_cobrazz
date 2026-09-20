#!/usr/bin/env python3
"""Generate a root-owned frontend network manifest, without contacting any host."""
import argparse
import ipaddress
import json
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

NS = {'cfg': 'http://www.adintelligence.net/xsd/AdServer/Configuration'}


def ipv4(value):
  address = ipaddress.IPv4Address(value)
  if (address.is_unspecified or address.is_multicast or address.is_loopback or
      int(address) == 0xffffffff):
    raise ValueError('Not a unicast frontend address: ' + value)
  return str(address)


def generate(tree):
  if tree.get('enabled') != 'true':
    return None
  network = tree.find('cfg:frontendNetwork', NS)
  endpoints = {}
  for endpoint in tree.findall('endpoint'):
    address = ipv4(endpoint.get('address', ''))
    port, target = (int(endpoint.get(k)) for k in ('port', 'internal_port'))
    if not all(1 <= value <= 65535 for value in (port, target)):
      raise ValueError('Port outside 1..65535')
    key = (address, port)
    if key in endpoints and endpoints[key] != target:
      raise ValueError('Conflicting redirects for {}:{}'.format(*key))
    endpoints[key] = target
  if not endpoints:
    raise ValueError('frontendNetwork needs at least one enabled externalAddress')
  inventory = {h.get('name'): h.get('hostName') for h in tree.findall('inventory/host')}
  frontends = {h.text for h in tree.findall('frontends/host')}
  hosts = {}
  for host in network.findall('cfg:host', NS):
    name = inventory.get(host.get('name'))
    interface = host.get('interface', '')
    if not name or name not in frontends or not re.fullmatch(r'[A-Za-z0-9][\w.-]*', name):
      raise ValueError('Unknown/non-frontend host: ' + str(host.get('name')))
    if not re.fullmatch(r'[A-Za-z0-9_-]{1,15}', interface) or interface == 'lo':
      raise ValueError('Invalid frontend interface: ' + interface)
    if name in hosts:
      raise ValueError('Duplicate host: ' + name)
    source = ipv4(host.get('arp_source_ip', ''))
    if source in {address for address, port in endpoints}:
      raise ValueError('ARP source must not be a shared externalAddress')
    if any(h['arp_source_ip'] == source for h in hosts.values()):
      raise ValueError('Duplicate ARP source: ' + source)
    hosts[name] = {'interface': interface, 'arp_source_ip': source}
  if not hosts:
    raise ValueError('frontendNetwork needs explicitly selected hosts')
  result = {'version': 1, 'hosts': hosts, 'redirects': [
    {'address': address, 'port': port, 'internal_port': target}
    for (address, port), target in sorted(endpoints.items())]}
  route = network.find('cfg:defaultRoute', NS)
  if route is not None:
    if route.get('onlink', 'false') not in ('false', 'true', '0', '1'):
      raise ValueError('Invalid onlink boolean')
    result['default_route'] = {'gateway': ipv4(route.get('gateway', '')),
                              'onlink': route.get('onlink') in ('true', '1')}
  return result


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  for name in ('xml', 'xpath', 'plugin', 'output', 'colo'):
    parser.add_argument('--' + name, required=True)
  args = parser.parse_args()
  if not re.fullmatch(r'[a-z0-9][a-z0-9_-]*', args.colo):
    raise ValueError('Invalid colocation name')
  xml = subprocess.check_output(['xsltproc', '--stringparam', 'XPATH', args.xpath,
    str(Path(args.plugin) / 'xslt/Frontend/FrontendNetwork.xsl'), args.xml])
  result = generate(ET.fromstring(xml))
  if result is not None:
    destination = Path(args.output) / 'etc/foros/frontend-network' / (args.colo + '.json')
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    destination.chmod(0o644)


if __name__ == '__main__':
  try:
    main()
  except (ValueError, OSError, subprocess.CalledProcessError) as error:
    sys.exit('Frontend network generation failed: ' + str(error))
