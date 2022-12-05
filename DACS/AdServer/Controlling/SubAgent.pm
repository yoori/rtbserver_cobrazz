package AdServer::Controlling::SubAgent;

use strict;
use warnings;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $command =
    "mkdir -p \${log_root}/SubAgent && " .
    "{ rm -f \${workspace_root}/run/subagent-shell.pid && " .
      "PERL5LIB=\${PERL5LIB}:\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/subagent && " .
      "export PERL5LIB && " .
      "/usr/bin/subagent-shell --daemon " .
        "--pid-file=\${workspace_root}/run/subagent-shell.pid " .
        "--base-dir=\${config_root}/${AdServer::Path::XML_FILE_BASE}$host/subagent " .
        "--mib-dir=\${mib_root}/ " .
        "--log-file=/dev/stdout 2>&1 " .
        "| RotateLog --daemon \${workspace_root}/run/subagentrotatelog.pid " .
        "--size 100 --time 1440 --cron 00:00 " .
        "\${workspace_root}/log/SubAgent/SubAgent.error; }";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub stop
{
  my ($host, $descr) = @_;

  my $command =
    "test \${workspace_root}/run/subagent-shell.pid || exit 0 &&".
    "kill `cat \${workspace_root}/run/subagent-shell.pid`";

  return AdServer::Functions::execute_command($host, $descr, $command);
}

sub is_alive
{
  my ($host, $descr) = @_;

  my $command =
    "test -e \${workspace_root}/run/subagent-shell.pid || exit 1 && " .
    "{ PID=\`cat \${workspace_root}/run/subagent-shell.pid\` 2>/dev/null; " .
    "[[ \`ps --no-heading -o ucmd \${PID}\` == \"subagent-shell\" ]]; }" .
    " || exit 1 && exit 0 ";

  my $res = AdServer::Functions::execute_command($host, $descr, $command);

  return ($res != 0 ? 1 : 0);
}

1;
