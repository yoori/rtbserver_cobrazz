package AdServer::PSConfigurator;

use strict;
use Utils::Functions;
use AdServer::Functions;

sub start
{
  my ($host, $descr) = @_;

  my $command = 
    "test \${ps_data_root} || " .
      "{ echo \"variable ps_data_root isn't defined on $host\" && exit -1 ; } && " .
    "mkdir -p \${workspace_root}/run/PS && " .
    "rm -rf \${data_root}/PageSense/http && " .
    "rm -rf \${data_root}/PageSense/https && " .
    "mkdir -p \${data_root}/PageSense/http && " .
    "mkdir -p \${data_root}/PageSense/https && " .
    "{ test -e \${ps_data_root}/ && " .
      "ptempl.pl -i \${ps_data_root}/ " .
        "-o \${data_root}/PageSense/http/ " .
        "-p \${config_root}/$host/PS/ps_http_vars || echo \"WARNING: \${ps_data_root}/tag don't exist\" ; } && " .
    "{ test -e \${config_root}/$host/PS/ps_https_vars && " .
      "{ test -e \${ps_data_root}/ && " .
        "ptempl.pl -i \${ps_data_root}/ " .
          "-o \${data_root}/PageSense/https/ " .
          "-p \${config_root}/$host/PS/ps_https_vars || echo \"WARNING: \${ps_data_root}/tag don't exist\" ; " .
      "} ;" .
    "} && " .
    "exit 0";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub none
{
  return 1;
}

1;
