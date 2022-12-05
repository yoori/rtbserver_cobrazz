package AdServer::DBAccess;

use strict;
use Utils::Functions;
use AdServer::Functions;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/log/DBAccess && ".
    "date  +'%a %d %m %Y %H:%M:%S:%N : start DB' >> \${workspace_root}/log/DBAccess/DBAccess.log && " .
    "ProbeObjGroup.pl \${config_root}/$host/DBAccess.pm -control DB 1 >>\${workspace_root}/log/DBAccess/DBAccess.log 2>&1; " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/log/DBAccess && ".
    "date  +'%a %d %m %Y %H:%M:%S:%N : stop DB' >> \${workspace_root}/log/DBAccess/DBAccess.log && " .
    "ProbeObjGroup.pl \${config_root}/$host/DBAccess.pm -control DB 0 2>>\${workspace_root}/log/DBAccess/DBAccess.log ; " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub none
{
  return 1;
}

1;
