package Common::ModifierExecImpl;

use warnings;
use strict;
use Scalar::Util qw(blessed);
use Common::ModifierExec;

# fetch content of directory on host
sub list;

# create directory
sub mkdir;

# remove directory/file
sub remove;

# move directory/file between hosts
sub move;

# copy directory/file between hosts
sub copy;

# check directory existing
sub dir_exists;

sub chunks_root;

# execute custom command for folder/file
sub execute_command;


# implemementations
sub new
{
  my ($class, $chunks_root, $ssh_identity) = @_;
  my $self = {
    chunks_root_ => $chunks_root,
    ssh_identity_ => $ssh_identity
    };
  bless($self, $class);
  return $self;
}

# return list of files in dir (on host)
# without trailing slashes
sub list
{
  my ($this, $host, $path, $recursive) = @_;

  if(defined($recursive) && $recursive > 0)
  {
    return split(/\n/, $this->execute_host_command_for_output_(
      $host,
      "cd " . $this->get_path_($path) . " 2>/dev/null || exit 0 && " .
        "find ./ | tail -n +2 | sed 's|^./\\(.*\\)\$|\\1|'"));
  }
  else
  {
    return split(/\n/, $this->execute_host_command_for_output_(
      $host,
      "ls -1 " . $this->get_path_($path) . " 2>/dev/null || exit 0"));
  }
}

sub mkdir
{
  my ($this, $host, $path) = @_;

  return $this->execute_host_command_(
    $host, "mkdir -p " . $this->get_path_($path));
}

sub remove
{
  my ($this, $host, $path, $force) = @_;
  
  my $options = (defined($force) && $force eq 1) ? "-rf " : "-r ";

  return $this->execute_host_command_(
    $host, "rm ". $options . $this->get_path_($path));
}

sub move
{
  my ($this, $src_host, $src_path, $tgt_host, $tgt_path) = @_;

  my $res = $this->mkdir($tgt_host, '');

  if($res != 0)
  {
    return $res;
  }

  if($src_host ne $tgt_host)
  {
    $res = $this->copy($src_host, $src_path, $tgt_host, $tgt_path);

    if($res == 0)
    {
      $res = $this->execute_host_command_(
        $src_host,
        "rm -r " . $this->get_path_($src_path));
    }

    return $res;
  }
  else
  {
    return $this->execute_host_command_(
      $src_host,
      "mv " . $this->get_path_($src_path) . " " . $this->get_path_($tgt_path));
  }
}

sub copy
{
  my ($this, $src_host, $src_path, $tgt_host, $tgt_path) = @_;

  my $res = $this->mkdir($tgt_host, '');

  if($res != 0)
  {
    return $res;
  }

  if($src_host ne $tgt_host)
  {
    my ($src_folder, $src_file) = split_path_($this->get_path_($src_path));
    my ($tgt_folder, $tgt_file) = split_path_($this->get_path_($tgt_path));

    if(!defined($src_folder) || !defined($src_file) || !defined($tgt_folder))
    {
      die "Error: ModifierExecImpl::copy: undefined src_folder or src_file or tgt_folder";
    }

    my $copy_command = "set -o pipefail && " .
      "{ ssh -o 'BatchMode yes' -i " . $this->{ssh_identity_} . " $src_host " .
          "'tar -C $src_folder -c $src_file' | " .
        "ssh -o 'BatchMode yes' -i " . $this->{ssh_identity_} . " $tgt_host " .
          "'mkdir -p $tgt_folder/~$src_file && " .
           "tar -x -C $tgt_folder/~$src_file && " .
           ($src_file ne $tgt_file ?
             "mv $tgt_folder/~$src_file/$src_file $tgt_folder/~$src_file/$tgt_file && " : "") .
           "mv $tgt_folder/~$src_file/$tgt_file $tgt_folder && " .
           "rm -r $tgt_folder/~$src_file' ; " .
      "}";

#   print $copy_command . "\n";

    return system($copy_command);
  }
  else
  {
    return $this->execute_host_command_(
      $src_host,
      "cp -r " . $this->get_path_($src_path) . " " . $this->get_path_($tgt_path));
  }
}

sub chunks_root
{
  my($this) = @_;
  return $this->{chunks_root_};
}

sub dir_exists
{
  my($this, $host, $dir_path) = @_;

  my $command = "test -d " . $this->get_path_($dir_path);

  return !$this->execute_host_command_($host, $command);
}

sub execute_command
{
  my ($this, $host, @args) = @_;

  my $command = '';
  my $arg_i = 0;
  foreach my $arg(@args)
  {
    if(!defined($arg))
    {
      die "assert: execute_command: passed undefined object($arg_i)";
    }

    if(blessed($arg) && !exists($arg->{path}))
    {
      die "assert: execute_command: passed object($arg_i) that isn't result of path_wrapper";
    }

    my $arg_res = (blessed($arg) ? $this->get_path_($arg->{path}) : $arg);
    $command .= ($command eq '' ? "" : " ") . $arg_res;
    ++$arg_i;
  }

  return $this->execute_host_command_($host, $command);
}

sub execute_command_for_output
{
  my ($this, $host, @args) = @_;

  my $command = '';
  my $arg_i = 0;
  foreach my $arg(@args)
  {
    if(!defined($arg))
    {
      die "assert: execute_command: passed undefined object($arg_i)";
    }

    if(blessed($arg) && !exists($arg->{path}))
    {
      die "assert: execute_command: passed object($arg_i) that isn't result of path_wrapper";
    }

    my $arg_res = (blessed($arg) ? $this->get_path_($arg->{path}) : $arg);
    $command .= ($command eq '' ? "" : " ") . $arg_res;
    ++$arg_i;
  }

  return $this->execute_host_command_for_output_($host, $command);
}

sub execute_host_command_
{
  my ($this, $host, $command) = @_;
  #print STDERR "TO EXEC '" . $command . "'\n";
  return system("ssh", "-i", $this->{ssh_identity_}, $host, $command);
}

sub execute_host_command_for_output_
{
  my ($this, $host, $command) = @_;
  #print STDERR "TO EXEC '" . $command . "' FOR OUTPUT\n";
  my $ssh_command = "ssh -i " . $this->{ssh_identity_} . " $host \"$command\"";
  my $out = `$ssh_command`;
  if($?)
  {
    die "Error: can't execute '$ssh_command': error code = $?";
  }
  return $out;
}

sub get_path_
{
  my ($this, $path) = @_;
  my $res_path = $this->{chunks_root_} . "/$path";
  1 while $res_path =~ s#^\./##g;
  1 while $res_path =~ s#/\.(/|\Z)#$2#g;
  1 while $res_path =~ s#(\A|/)[^/]+/+\.\.(/|\Z)#$1$2#g;
  return $res_path;
}

sub split_path_
{
  my ($path) = @_;

  if($path =~ m|(.*/)([^/]+)/?|)
  {
    return ($1, $2);
  }

  return (undef, undef);
}

1;
