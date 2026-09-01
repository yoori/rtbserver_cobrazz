package AICluster::AIAgent;

use strict;
use AICluster::Functions;

my $pid_file = "\${workspace_root}/run/AIAgent.pid";

sub config_file
{
  my ($host) = @_;
  return "\${config_root}/$host/AIAgentConfig.sh";
}

sub start
{
  my ($host, $description) = @_;
  my $config_file = config_file($host);
  my $command =
    "test -r $config_file || { echo 'AIAgent config is not readable: " .
      "$config_file' >&2; exit 1; }; " .
    ". $config_file; " .
    "if test -e $pid_file; then " .
      "pid=`cat $pid_file`; " .
      "kill -0 \$pid 2>/dev/null && exit 1 || rm -f $pid_file; " .
    "fi; " .
    "setsid -f AIAgent.sh \"$config_file\" \"\${workspace_root}\" " .
      "> \${workspace_root}/run/AIAgent.out 2>&1 < /dev/null; " .
    "for i in `seq 1 120`; do " .
      "test -e $pid_file && pid=`cat $pid_file` && " .
        "kill -0 \$pid 2>/dev/null && " .
        "NO_PROXY=127.0.0.1,localhost no_proxy=127.0.0.1,localhost " .
          "OLLAMA_HOST=http://127.0.0.1:\$AI_PORT " .
          "\"\$AI_OLLAMA_BIN\" show \"\$AI_MODEL\" >/dev/null 2>&1 && " .
          "curl -fsS http://127.0.0.1:\$AI_AGENT_PORT/health " .
            ">/dev/null 2>&1 && " .
          "curl -fsS http://127.0.0.1:\$AI_CLUSTER_AGENT_PORT/health " .
            ">/dev/null 2>&1 && exit 0; " .
      "sleep 1; " .
    "done; " .
    "cat \${workspace_root}/run/AIAgent.out >&2 2>/dev/null || true; " .
    "exit 1";
  return AICluster::Functions::execute_command($command);
}

sub stop
{
  my ($host, $description) = @_;
  return AICluster::Functions::stop_by_pidfile($pid_file);
}

sub is_alive
{
  my ($host, $description) = @_;
  my $config_file = config_file($host);
  return AICluster::Functions::execute_command(
    "test -r $config_file || exit 1; " .
    ". $config_file; " .
    "test -e $pid_file || exit 1; " .
    "pid=`cat $pid_file`; " .
    "kill -0 \$pid 2>/dev/null || exit 1; " .
    "NO_PROXY=127.0.0.1,localhost no_proxy=127.0.0.1,localhost " .
      "OLLAMA_HOST=http://127.0.0.1:\$AI_PORT " .
      "\"\$AI_OLLAMA_BIN\" show \"\$AI_MODEL\" >/dev/null 2>&1 && " .
      "curl -fsS http://127.0.0.1:\$AI_AGENT_PORT/health >/dev/null 2>&1 && " .
      "curl -fsS http://127.0.0.1:\$AI_CLUSTER_AGENT_PORT/health >/dev/null 2>&1");
}

1;
