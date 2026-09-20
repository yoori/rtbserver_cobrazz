package AdServer::LogProcessing::STunnelServer;

use strict;
use Utils::Functions;
use AdServer::Functions;

my $rsync_pid = "\${workspace_root}/run/rsyncserver.pid";
my $stunnel_pid = "\${workspace_root}/run/stunnelserver.pid";

sub start
{

  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/log/STunnelServer && " .
    "{ " .
      "mkdir -p \${workspace_root}/run && " .
      "{ " .
        AdServer::Functions::pidfile_is_alive($rsync_pid, "/usr/bin/rsync") . " || " .
        "{ rm -f \${workspace_root}/run/rsyncserver.pid && " .
        "/usr/bin/rsync --daemon --config=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/rsync_server.conf" .
        " </dev/null 666>&1 2>&1 | RotateLog " .
        "--daemon \${workspace_root}/run/rotatelogstunnelserver.pid --size 100 --time 1440 --cron 00:00 " .
          "\${workspace_root}/log/STunnelServer/STunnelServer.log >/dev/null 2>&1; " .
        "} " .
      "} && " .
      "{ " .
      AdServer::Functions::pidfile_is_alive($stunnel_pid, "/usr/bin/stunnel") . " || " .
        "{ rm -f \${workspace_root}/run/stunnelserver.pid && ".
        "/usr/bin/stunnel \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/stunnel_server.conf 2>&1 " .
        "| RotateLog --size 100 --time 1440 --cron 00:00 \${workspace_root}/log/STunnelServer/STunnelServer.log; }" .
      "; } " .
    "} ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    AdServer::Functions::stop_pidfile_command($stunnel_pid, "/usr/bin/stunnel") . " && " .
    AdServer::Functions::stop_pidfile_command($rsync_pid, "/usr/bin/rsync");

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    AdServer::Functions::pidfile_is_alive($stunnel_pid, "/usr/bin/stunnel") . " && " .
    AdServer::Functions::pidfile_is_alive($rsync_pid, "/usr/bin/rsync");

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if ($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
