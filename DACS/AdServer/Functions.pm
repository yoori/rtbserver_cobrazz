package AdServer::Functions;

use Utils::Functions;
use AdServer::Path;

sub process_control_stop
{
  my ($host, $port_var, $descr) = @_;
  Utils::Functions::process_control_stop(
    $host,
    $port_var,
    $host . "/" . ${AdServer::Path::ADSERVER_CURRENTENV_DIR},
    $descr);
}

sub process_control_is_alive
{
  my ($host, $port_var, $descr) = @_;
  return Utils::Functions::process_control_is_alive(
    $host,
    $port_var,
    $host . "/" . ${AdServer::Path::ADSERVER_CURRENTENV_DIR},
    $descr);
}

sub stop_by_pidfile
{
  my ($host, $descr, $pid_file) = @_;

  my $command =
    "test -e $pid_file || exit 0 && " .
    "pid=`cat $pid_file` && " .
    "{ kill -0 \$pid 2>/dev/null || { rm -f $pid_file; exit 0; }; } && " .
    "kill -TERM \$pid 2>/dev/null || true; " .
    "for i in `seq 1 60`; do " .
      "test -e $pid_file || exit 0; " .
      "kill -0 \$pid 2>/dev/null || { rm -f $pid_file; exit 0; }; " .
      "sleep 1; " .
    "done; " .
    "kill -9 \$pid 2>/dev/null || true; " .
    "test -e $pid_file && rm -f $pid_file || true";

  return execute_command($host, $descr, $command);
}

sub execute_command
{
  my ($host, $descr, $exe_command) = @_;
  my $environment_dir = Utils::Functions::init_environment();

  my $command =
      "source $environment_dir/environment.sh && " .
      "test \${workspace_root}  || " .
        "{ echo \"Variable workspace_root isn't defined on $host\" && exit -1 ; } && " .
      "test \${config_root} || " .
        "{ echo \"Variable config_root isn't defined on $host\" && exit -1 ; } && " .
      "test \${log_root} || " .
         "{ echo \"variable log_root isn't defined on $host\" && exit -1 ; } && " .
      "mkdir -p \${workspace_root}/${AdServer::Path::OUT_FILE_BASE} && " .
      "cd \${workspace_root}/${AdServer::Path::OUT_FILE_BASE} && " .
      "$exe_command ";

  my $res =  Utils::Functions::safe_system($command);
  if ($res)
  {
    return 0;
  }
  else
  {
    if ($res>>8 == 0)
    {
      return 1;
    }
    else
    {
      return $res>>8;
    }
  }
}

sub prepare_ram_log_dirs
{
  my ($service, @log_names) = @_;

  my $command =
    "if test -n \"\${ram_fs_root:-}\"; then " .
      "mountpoint -q \"\${ram_fs_root}\" && " .
      "ram_log_root=\"\${ram_fs_root}/log/$service/Out\" && " .
      "mkdir -p \"\${log_root}/$service/Out\" \"\$ram_log_root\" && ";

  foreach my $log_name (@log_names)
  {
    $command .=
      "mkdir -p \"\$ram_log_root/$log_name\" \"\${log_root}/$service/Out/$log_name.ram_\" && " .
      "ln -sfnT \"\$ram_log_root/$log_name\" \"\${log_root}/$service/Out/$log_name.ram\" && ";
  }

  return $command . "true; fi";
}

sub thread_affinity_env
{
  my ($config_file, $config_element) = @_;

  return
    "ADS_THREAD_AFFINITY=round_robin_by_name " .
    "ADS_THREAD_AFFINITY_CPUS=\"(n:\$(sed -n 's/.*<cfg:" .
      $config_element .
      "[^>]*numa_node=\"\\([0-9][0-9]*\\)\".*/\\1/p' " .
      $config_file .
      " | head -1))\" ";
}

1;
