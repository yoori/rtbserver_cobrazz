package AdServer::LogProcessing::CleanupLogs;

use strict;
use warnings;
use Utils::Functions;
use AdServer::Functions;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/log/CleanupLogs && "
    ."date  +'%a %d %m %Y %H:%M:%S:%N : start logs cleanup' "
    .">> \${workspace_root}/log/CleanupLogs/CleanupLogs.log && "
    ."{ CleanupLogs.pl -conf \${config_root}/${AdServer::Path::XML_FILE_BASE}$host/conf/cleanup_logs.conf "
    .">> \${workspace_root}/log/CleanupLogs/CleanupLogs.log 2>&1 &}";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/log/CleanupLogs && "
    ."date  +'%a %d %m %Y %H:%M:%S:%N : stop logs cleanup' "
    .">> \${workspace_root}/log/CleanupLogs/CleanupLogs.log && "
    ."test \${workspace_root}/run/cleanup_logs.pid || "
    ."{ exit 0; } && "
    ."kill `cat \${workspace_root}/run/cleanup_logs.pid`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "{ test -e \${workspace_root}/run/cleanup_logs.pid || exit 1 ; } && "
    ."kill -0 \`cat \${workspace_root}/run/cleanup_logs.pid\` 2>/dev/null || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  if ($res != 0)
  {
    return 1;
  }
  return 0;
}

1;
