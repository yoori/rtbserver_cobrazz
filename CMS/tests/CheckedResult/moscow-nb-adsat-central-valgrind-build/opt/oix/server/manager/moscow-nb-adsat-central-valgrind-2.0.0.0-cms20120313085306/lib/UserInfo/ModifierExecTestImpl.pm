package UserInfo::ModifierExecTestImpl;

use warnings;
use strict;

sub new
{
  my ($class, $chunks_root, $test_dir) = @_;
  my $self = {
    chunks_root_ => $chunks_root,
    test_dir_ => $test_dir,
#   verbose_execute_ => 1,
    };
  bless($self, $class);
  return $self;
}

sub list
{
  my ($this, $host, $path, $recursive) = @_;

  $path =~ s|/+$||;
  if(defined($recursive) && $recursive > 0)
  {
    my $out = $this->execute_command_for_output_(
      "cd " . $this->get_path_($host, $path) . " 2>/dev/null && " .
        "find ./ | sed 's|^./\\(.*\\)\$|\\1|' | grep -v -E '^\$' || echo");
    return split(/\n/, $out);
  }
  else
  {
    return split(/\n/, $this->execute_command_for_output_(
      "ls -1 " . $this->get_path_($host, $path) . " 2>/dev/null || echo"));
  }
}

sub mkdir
{
  my ($this, $host, $path) = @_;
  return system("mkdir -p " . $this->get_path_($host, $path));
}

sub remove
{
  my ($this, $host, $path) = @_;
  return system("rm -r " . $this->get_path_($host, $path));
}

sub move
{
  my ($this, $src_host, $src_path, $tgt_host, $tgt_path) = @_;

  return system(
    "mkdir -p " . $this->get_path_($tgt_host, '') . " && " .
    "mv " . $this->get_path_($src_host, $src_path) . " " . $this->get_path_($tgt_host, $tgt_path));
}

sub copy
{
  my ($this, $src_host, $src_path, $tgt_host, $tgt_path) = @_;

  return system(
    "mkdir -p " . $this->get_path_($tgt_host, '') . " && " .
    "cp -r " . $this->get_path_($src_host, $src_path) . " " . $this->get_path_($tgt_host, $tgt_path));
}

sub chunks_root
{
  my($this) = @_;
  return $this->{chunks_root_};
}

sub execute_command_for_output_
{
  my ($this, $command) = @_;
  print "DEBUG(execute_command_for_output_): $command\n";

  my $out = `$command`;
  if($?)
  {
    die "Error: can't execute '$command': error code = $?";
  }
  return $out;
}

sub get_path_
{
  my ($this, $host, $path) = @_;
  my $res_path = $this->{test_dir_} . "/$host/" . $this->{chunks_root_} . "/$path";
  1 while $res_path =~ s#^\./##g;
  1 while $res_path =~ s#/\.(/|\Z)#$2#g;
  1 while $res_path =~ s#(\A|/)[^/]+/+\.\.(/|\Z)#$1$2#g;
  return $res_path;
}

1;
