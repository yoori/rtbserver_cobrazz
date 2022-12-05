package connections;
use strict;
use warnings;

our @service_port = (
__SERVICENAME_PORT_VALUES__  );

our $ports = '';
# sizes for ChannelServer, UserBindServer, CampaignManager
our $size_scan_ports = '';
# httpd to UIM, ChannelServer, UserBindServer, CampaignManager ports
our $httpd_peer_ports = '';
our %port_service;

foreach my $srv(@service_port)
{
  $ports .= (length $ports > 0 ? '|' : '') . @$srv[1];
  $port_service{@$srv[1]} = @$srv[0];

  if (@$srv[0] eq 'channelServer' or @$srv[0] eq 'userBindServer' or @$srv[0] eq 'campaignManager')
  {
    $size_scan_ports .= (length $size_scan_ports > 0 ? '|' : '') . @$srv[1];
  }
  elsif (@$srv[0] eq 'userInfoManager')
  {
    $httpd_peer_ports = (length $httpd_peer_ports > 0 ? '|' : '') . @$srv[1];
  }
}

$httpd_peer_ports .= (length $httpd_peer_ports > 0 ? '|' : '') . $size_scan_ports;

our %state_codes = (
    'UNKNOWN' => 0,
    'SYN-SENT' => 1,
    'SYN-RECV' => 2,
    'ESTAB' => 3,
    'FIN-WAIT-1' => 4,
    'FIN-WAIT-2' => 5,
    'TIME-WAIT' => 6,
    'CLOSE-WAIT' => 7,
    'LAST-ACK' => 8,
    'CLOSING' => 9
  );

1;
