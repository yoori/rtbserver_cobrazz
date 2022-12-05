package AdServer::LogProcessing::StatReceiver;

use strict;
use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/StatReceiver/Out/ExtStat && " .
    "mkdir -p \${workspace_root}/run && " .
    "{ rm -f \${workspace_root}/run/StatReceiver.pid && ".
    "/usr/bin/rsync --daemon --config=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/StatReceiver.conf " .
    " 2>>\${workspace_root}/run/StatReceiver.out 1>&2 & } ";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test \${workspace_root}/run/StatReceiver.pid || exit 0 &&".
    "kill `cat \${workspace_root}/run/StatReceiver.pid`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e \${workspace_root}/run/StatReceiver.pid || exit 1 && " . 
    "kill -0 \`cat \${workspace_root}/run/StatReceiver.pid\` 2>/dev/null" .
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
