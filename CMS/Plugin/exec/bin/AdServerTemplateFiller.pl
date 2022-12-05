#!/usr/bin/perl -w

use strict;
use File::Basename;
use Getopt::Std;
use String::CRC32;

my $usage = "Usage: <installed services> | $0 -f <host,hosts,list> <Zenoss template file> " .
  "<path to save result files>\n";

my %opts;
my $fe_hosts = "";
getopt("fh", \%opts);
foreach (keys %opts)
{
  if ($_ eq "f")
  {
    $fe_hosts = $opts{$_};
  }
  elsif ($_ eq "h")
  {
    print $usage and exit 1;
  }
  else
  {
    die "$0: Unknown option '$_'!\n";
  }
}
if ($#ARGV != 1)
{
  print $usage;
  exit 1;
}

my $fe_oids = "";
my $fe_graphs = "";
my @hosts = split(',', $fe_hosts);
foreach (@hosts)
{
  my $fe_index = crc32($_);
  $fe_oids .= <<FE_OIDS;
        <oid name="rtbRequestSkipCount.$fe_index" type="COUNTER"/>
        <oid name="rtbRequestTimeoutCount.$fe_index" type="COUNTER"/>
        <oid name="rtbRequestCount.$fe_index" type="COUNTER"/>
        <oid name="rtbRequestTimeCounter.$fe_index" type="COUNTER"/>
FE_OIDS
  $fe_graphs .= <<FE_GRAPHS

        <graph name="RTB $_ requests timeouts" units="quantity">
          <oid name="rtbRequestSkipCount.$fe_index"/>
          <oid name="rtbRequestTimeoutCount.$fe_index"/>
        </graph>

        <graph name="RTB $_ latency" log="False" units="microseconds"
          custom="CDEF:oidtmp=rtbRequestTimeCounter_${fe_index}_%ID-raw,rtbRequestCount_${fe_index}_%ID-raw,/&#10;LINE2:oidtmp#ff00ff:$_ latency in millisec&#10;GPRINT:oidtmp:LAST:cur\\:%5.2lf%s&#10;GPRINT:oidtmp:AVERAGE:avg\\:%5.2lf%s&#10;GPRINT:oidtmp:MAX:max\\:%5.2lf%s\\j">
          <oid name="rtbRequestCount.$fe_index" type="DONTDRAW"/>
          <oid name="rtbRequestTimeCounter.$fe_index" type="DONTDRAW"/>
        </graph>
FE_GRAPHS
}

$|=1;

my %services;

while (<STDIN>)
{
  if ($_ =~ /\s+\[\'([^\']+)\', (\d+)/)
  {
    push @{$services{$1}}, $2;
  }
}

my $result;

  foreach my $service(sort keys %services)
  {
    my $connection_sizes_part_oids = '';
    my $connection_sizes_part_graphs = '';
    if ($service eq 'campaignManager' or $service eq 'channelServer' or $service eq 'userBindServer')
    {
      $connection_sizes_part_oids = <<ADD_SIZES_PART_OIDS;
<oid name="${service}AvgRecv" type="GAUGE"/>
      <oid name="${service}AvgSend" type="GAUGE"/>
      <oid name="${service}AvgMaxRecv" type="GAUGE"/>
      <oid name="${service}AvgMaxSend" type="GAUGE"/>
ADD_SIZES_PART_OIDS
      $connection_sizes_part_graphs = <<ADD_SIZES_PART_GRAPH
<graph name="$service connections IO sizes stats" units="Averages for sent, received bytes">
        <oid name="${service}AvgRecv" type="AREA" stacked="True"/>
        <oid name="${service}AvgSend" type="AREA" stacked="True"/>
        <oid name="${service}AvgMaxRecv" type="AREA" stacked="True"/>
        <oid name="${service}AvgMaxSend" type="AREA" stacked="True"/>
      </graph>
ADD_SIZES_PART_GRAPH
    }
    $result .= <<END;
  <template name="${service}Connections" prefix="SubAgent-Shell-Connections-MIB">
    <oids>
END
      foreach my $port(sort @{$services{$service}})
      {
        $result .= <<END;
      <oid name="$service.$port.unknown" type="GAUGE"/>
      <oid name="$service.$port.synSent" type="GAUGE"/>
      <oid name="$service.$port.synReceived" type="GAUGE"/>
      <oid name="$service.$port.established" type="GAUGE"/>
      <oid name="$service.$port.finWait1" type="GAUGE"/>
      <oid name="$service.$port.finWait2" type="GAUGE"/>
      <oid name="$service.$port.timeWait" type="GAUGE"/>
      <oid name="$service.$port.closeWait" type="GAUGE"/>
      <oid name="$service.$port.lastAck" type="GAUGE"/>
      <oid name="$service.$port.closing" type="GAUGE"/>
END
      }
      $result .= <<END;
      $connection_sizes_part_oids
    </oids>
    <graphs>
END
      foreach my $port(sort @{$services{$service}})
      {
        $result .= <<END;
      <graph name="$service port $port connections stats" units="Number of connection in state">
        <oid name="$service.$port.unknown" type="AREA" stacked="True"/>
        <oid name="$service.$port.synSent" type="AREA" stacked="True"/>
        <oid name="$service.$port.synReceived" type="AREA" stacked="True"/>
        <oid name="$service.$port.established" type="AREA" stacked="True"/>
        <oid name="$service.$port.finWait1" type="AREA" stacked="True"/>
        <oid name="$service.$port.finWait2" type="AREA" stacked="True"/>
        <oid name="$service.$port.timeWait" type="AREA" stacked="True"/>
        <oid name="$service.$port.closeWait" type="AREA" stacked="True"/>
        <oid name="$service.$port.lastAck" type="AREA" stacked="True"/>
        <oid name="$service.$port.closing" type="AREA" stacked="True"/>
      </graph>
END
      }
      $result .= <<END;
      $connection_sizes_part_graphs
      </graphs>
  </template>

END
  }

  open IN, "<", $ARGV[0] or die $!;
  open OUT, ">", "$ARGV[1]/" . fileparse($ARGV[0]) or die $!;

  while (<IN>)
  {
    if ($_ =~ /__CLUSTER_CONNECTIONS_TEMPLATE__/g)
    {
      print OUT $result;
    }
    elsif ($_ =~ /__FRONTEND_HOSTS_OIDS__/g)
    {
      print OUT $fe_oids;
    }
    elsif ($_ =~ /__FRONTEND_HOSTS_GRAPHS__/g)
    {
      print OUT $fe_graphs;
    }
    else
    {
      print OUT $_;
    }
  }
  close IN;
  close OUT;
