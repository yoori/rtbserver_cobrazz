package AdServer::LogProcessing::STunnelServer;

use strict;
use Utils::Functions;
use AdServer::Functions;

sub start
{

  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/log/STunnelServer && " .
    "{ " .
      "mkdir -p \${workspace_root}/run && " .
      "{ " .
        "{ test -e \${workspace_root}/run/rsyncserver.pid && " .
          "kill -0 \`cat \${workspace_root}/run/rsyncserver.pid\` 2>/dev/null; } || " .
        "{ rm -f \${workspace_root}/run/rsyncserver.pid && " .
        "/usr/bin/rsync --daemon --config=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/rsync_server.conf" .
        " </dev/null 666>&1 2>&1 | RotateLog " .
        "--daemon \${workspace_root}/run/rotatelogstunnelserver.pid --size 100 --time 1440 --cron 00:00 " .
          "\${workspace_root}/log/STunnelServer/STunnelServer.log >/dev/null 2>&1; " .
        "} " .
      "} && " .
      "{ " .
      "{ test -e \${workspace_root}/run/stunnelserver.pid && " .
        "kill -0 \`cat \${workspace_root}/run/stunnelserver.pid\` 2>/dev/null; } || " .
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
    "test \${workspace_root}/run/stunnelserver.pid || ".
    "test \${workspace_root}/run/rsyncserver.pid || " .
      "{ exit 0; } && " .
    "kill `cat \${workspace_root}/run/stunnelserver.pid` ; ".
    "kill `cat \${workspace_root}/run/rsyncserver.pid`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "{ test -e \${workspace_root}/run/rsyncserver.pid || exit 1 ; } && " .
    "{ test -e \${workspace_root}/run/stunnelserver.pid || exit 1 ; } && " .
    " { kill -0 \`cat \${workspace_root}/run/stunnelserver.pid\` 2>/dev/null &&" .
    " kill -0 \`cat \${workspace_root}/run/rsyncserver.pid\` 2>/dev/null ;}  ".
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
