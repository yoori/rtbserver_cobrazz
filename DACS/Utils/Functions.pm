package Utils::Functions;

use LWP::Simple;
use strict;
use warnings;
use Cwd qw(abs_path);
use File::Basename qw(basename);
use File::Spec;
use Text::ParseWords qw(shellwords);

my $sys_mutex : shared;

sub resolve_executable
{
  my ($name) = @_;
  return undef unless defined($name) && length($name);
  my @paths = $name =~ m{/} ? ($name) :
    map { File::Spec->catfile(length($_) ? $_ : '.', $name) } split /:/, $ENV{PATH}, -1;
  foreach my $path (@paths)
  {
    return abs_path($path) if -f $path && -x $path;
  }
  return undef;
}

sub pid_matches_executable
{
  my ($pid, $expected) = @_;
  return 0 unless defined($pid) && $pid =~ /\A[1-9][0-9]*\z/ && kill(0, $pid);
  open(my $status, '<', "/proc/$pid/status") or return 0;
  my ($tgid) = map { /^Tgid:\s*(\d+)/ ? $1 : () } <$status>;
  close($status);
  return 0 unless defined($tgid) && $tgid == $pid;
  my $actual = readlink("/proc/$pid/exe");
  return 0 unless defined($actual);
  $actual =~ s/ \(deleted\)\z//;
  $expected = resolve_executable($expected);
  return 0 unless defined($expected);
  return 1 if $actual eq $expected;

  # A script's executable is its interpreter. Check both interpreter and script position,
  # never an arbitrary occurrence of the expected filename among application arguments.
  open(my $script, '<', $expected) or return 0;
  my $header = <$script>;
  close($script);
  return 0 unless defined($header) && $header =~ s/^#!\s*//;
  my @interpreter = shellwords($header);
  return 0 unless @interpreter;
  if (basename($interpreter[0]) eq 'env')
  {
    shift @interpreter;
    shift @interpreter if @interpreter && $interpreter[0] eq '-S';
  }
  return 0 unless @interpreter;
  my $interpreter = resolve_executable(shift @interpreter);
  return 0 unless defined($interpreter) && $actual eq $interpreter;
  open(my $cmdline, '<', "/proc/$pid/cmdline") or return 0;
  my $raw = do { local $/; <$cmdline> };
  close($cmdline);
  return 0 unless defined($raw);
  my @argv = split /\0/, $raw;
  shift @argv;
  foreach my $option (@interpreter)
  {
    return 0 unless @argv && shift(@argv) eq $option;
  }
  return 0 unless @argv;
  my $path = shift @argv;
  if (!File::Spec->file_name_is_absolute($path))
  {
    my $cwd = readlink("/proc/$pid/cwd");
    return 0 unless defined($cwd);
    $path = File::Spec->catfile($cwd, $path);
  }
  my $canonical = abs_path($path);
  return defined($canonical) && $canonical eq $expected ? 1 : 0;
}

sub safe_system
{
  lock($sys_mutex);
  return system(@_);
}

sub safe_system_for_output
{
  lock($sys_mutex);

  open(SSO, '-|', @_) or die "safe_system_for_output: can't open pipe.";
  my $output = "";

  $output = <SSO>;
  close(SSO);
  my $ret = $?;

  return ($ret, defined $output ? $output : "");
}

sub probe_http
{
  my ($url, $answer) = @_;
  my $content;
  unless (defined($content = get($url)) and ($content eq $answer))
  {
    return 0;
  }
  return 1;
}

sub make_corba_ref
{
  my ($host, $port, $service_name) = @_;
  return 'corbaloc:iiop:' . $host . ':' . $port . '/' . $service_name;
}

sub init_environment
{
  defined($ENV{"config_root"}) or
    die "Not defined environment variable config_root";

  my $environment_dir = $ENV{'config_root'};

  return $environment_dir;
}

sub process_control_any
{
  my ($host, $port_var, $current_config_dir, $doing, $descr) = @_;

  my $environment_dir = init_environment();

  my $ref = make_corba_ref(
       $host,
       "\${".$port_var."}",
       "ProcessControl");

  my $command =
    ". $environment_dir/environment.sh && " .
    "test \${workspace_root} || " .
      "{ echo \"Stop: Variable workspace_root isn't defined on $host\" && exit 1 ; } && " .
    "if [ -f  \$config_root/$current_config_dir/fe.sh ]; then . \$config_root/$current_config_dir/fe.sh 2>/dev/null; fi && " .
    "if [ -f \$config_root/$current_config_dir/be.sh ]; then . \$config_root/$current_config_dir/be.sh 2>/dev/null; fi && " .
    "if [ -f \$config_root/$current_config_dir/pbe.sh ]; then . \$config_root/$current_config_dir/pbe.sh 2>/dev/null; fi && " .
    "if [ -f \$config_root/$current_config_dir/ccluster.sh ]; then . \$config_root/$current_config_dir/ccluster.sh  2>/dev/null; fi && " .
    "test \${$port_var} || " .
      "{ echo \"Stop: Variable $port_var isn't defined on $host\" && exit 1 ; } && " .
    "ProbeObj $doing $ref";
  return safe_system($command);
}

sub process_control_stop
{
  my ($host, $port_var, $current_config_dir, $descr) = @_;
  my $res = process_control_any(
    $host, $port_var, $current_config_dir, "-shutdown", $descr);

  if ($res)
  {
    return 0;
  }
  return 1;
}

sub process_control_is_alive
{
  my ($host, $port_var, $current_config_dir, $descr) = @_;
  my $ret = process_control_any(
    $host, $port_var, $current_config_dir, "", $descr);

  if ($ret == -1)
  {
    $$descr =  "failed to run ProbeObj: $!";
    return undef;
  }
  else
  {
    my $probe_ret = ($? >> 8);
    if ($probe_ret == 1)
    {
      return 0;
    }
    elsif($probe_ret != 0)
    {
      $$descr =  "The command ProbeObj exited with code: " . $probe_ret;
      return undef;
    }
  }
  return 1;
}

sub ssh_invoke
{
  my ($host, $command, $ssh_identity) = @_;
  my $status;

  $status = safe_system("ssh",  $host, "-i", $ssh_identity, $command);
  return $status;
}

1;
