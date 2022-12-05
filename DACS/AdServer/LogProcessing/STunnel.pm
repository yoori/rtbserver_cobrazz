package AdServer::LogProcessing::STunnel;

use strict;
use Utils::Functions;
use AdServer::Functions;

sub start
{
  my ($host, $descr) = @_;

  my $command =
       "{ " .
         "mkdir -p \${workspace_root}/log/STunnelClient && " .
         "/usr/bin/stunnel \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/stunnel_client.conf 2>&1 " .
         "| RotateLog --size 100 --time 1440 --cron 00:00 \${workspace_root}/log/STunnelClient/STunnelClient.log; " .
       "} ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
       "test \${workspace_root}/run/stunnel.pid || " .
         "{ exit 0; } && " .
       "kill `cat \${workspace_root}/run/stunnel.pid`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;


  my $command =
    "{ test -e \${workspace_root}/run/stunnel.pid || exit 1 ; } && " .
    "kill -0 \`cat \${workspace_root}/run/stunnel.pid\` 2>/dev/null || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
