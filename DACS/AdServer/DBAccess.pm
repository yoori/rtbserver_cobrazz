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
    "DBAccess.py \"\${config_root}/$host/DBAccess.json\" 1 " .
      ">>\${workspace_root}/log/DBAccess/DBAccess.log 2>&1";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${workspace_root}/log/DBAccess && ".
    "date  +'%a %d %m %Y %H:%M:%S:%N : stop DB' >> \${workspace_root}/log/DBAccess/DBAccess.log && " .
    "DBAccess.py \"\${config_root}/$host/DBAccess.json\" 0 " .
      ">>\${workspace_root}/log/DBAccess/DBAccess.log 2>&1";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub db_status
{
  my ($host, $descr, $service) = @_;
  my $filter = "";
  if (defined $service)
  {
    die "Invalid DBAccess service name" unless $service =~ /\A[A-Za-z0-9_.-]+\z/;
    $filter = " $service";
  }
  return AdServer::Functions::execute_command($host, $descr,
    "DBAccess.py \"\${config_root}/$host/DBAccess.json\" -1$filter");
}

sub none
{
  return 1;
}

1;
