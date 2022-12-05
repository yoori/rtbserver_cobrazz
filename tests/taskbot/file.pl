#! /usr/bin/perl
# -*- perl -*-
#

=head1 NAME

file.pl - insert lof files to 'file' DB table.

=head1 SYNOPSIS

  find.pl CONFIG OPTIONS

=head2 OPTIONS

=over

=item C<--task=TASK>

B<Optional>. C<TASK> is the task ID to link the file to.
If not defined, uses root task id of current taskbot run.

=item C<--path=PATH>

B<Required>. Path where to find source files.

=item C<--mask=MASK>

B<Optional>. Regex mask for saving files.
Script will save files that match MASK regular expression only.

=item C<--help, -h>

B<Optional>. Print this help and exit.

=back

=head2 ENVIRONMENT

Environment variables I<TASKBOT_PARENT_PID> and I<HOSTNAME>
should be set by taskbot, if --task option not defined.

=cut

use warnings;
use strict;

use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;
use File::Find;
use File::Copy;
use Cwd;

use constant MAX_CONTENT_SIZE => 16777215; # 16Mb - 1 byte;

my %options = ();

my $getopt_result = GetOptions(\%options, qw(
  task=i
  path=s
  mask=s@
  empty
  help|h));

if ($getopt_result && $options{help})
{
  pod2usage( { -exitval => 0,
               -verbose => 2 } );
}

if (! $getopt_result ||
    @ARGV < 1 ||
    grep {not defined} @options{qw(path)})
{
  pod2usage(2);
}

push @{$options{mask}}, ".*" unless $options{mask};

my $config_file = shift @ARGV;

my $config = do $config_file;
unless ($config) {
    die "While reading $config_file: $!" if ($!);
    die "While compiling $config_file: $@" if ($@);
    die "While executing $config_file: returned '$config'";
}

use DBI;

my $dbh = DBI->connect(@{$config->{taskbot_dbi}}[0..2],
                       { AutoCommit => 0, PrintError => 0, RaiseError => 1,
                         FetchHashKeyName => 'NAME_lc' });

sub get_db_hostname
{
  my $config = shift;

  my $db_host;

  my ($mysql_cnf) = @{$config->{taskbot_dbi}}[0] =~ m/^.*mysql_read_default_file=([^;]+)/;

  if ($mysql_cnf)
  {
    open(my $mysql_cnf_fh, $mysql_cnf) || die "can't open $mysql_cnf file:$!";
    while (my $line = <$mysql_cnf_fh>)
    {
      ($db_host) = $line =~ m/^host=(.+)$/;
      last if $db_host;
    }
    close($mysql_cnf_fh);
  }
  else
  {
    ($db_host) = @{$config->{taskbot_dbi}}[0] =~ m/^.*;?host=([^;]+);?.*$/;
    ($db_host) = @{$config->{taskbot_dbi}}[0] =~ m/^.*@([^:]+):?.*$/
      unless $db_host;
  }
  $db_host = "localhost" unless $db_host;
  return $db_host;
}

my $db_hostname = get_db_hostname($config);

my $task_id;

if (exists $options{task} and defined $options{task})
{
  $task_id = $options{task};
}
else # Get current task
{
  ($task_id) = $dbh->selectrow_array(q[
    SELECT MAX(task_id)
    FROM running_task
    WHERE host = ? AND pid = ? ],
    undef, $ENV{HOSTNAME}, $ENV{TASKBOT_PARENT_PID});
}

my $full_path = $options{path};
$full_path = cwd . "/" . $full_path if substr($full_path, 0, 1) ne "/";

my @files;

sub fill_files_list
{
  foreach my $mask (@{$options{mask}})
  {
    if ($File::Find::name =~ /$mask/i)
    {
      next if (!$options{empty} and (-z $File::Find::name)) or
              (-d $File::Find::name);
      my $full_name = $File::Find::name;
      my $file_size = -s $full_name;

      # Save only last 16 Mb of the file content
      if ($file_size > MAX_CONTENT_SIZE)
      {
        my $tmp_file = "$full_name." . time . ".tmp";
        open(my $src, "<", $full_name)
          or die "can't open $full_name file:$!";
        open(my $dst, ">", $tmp_file)
          or die "can't open $tmp_file file:$!";
        seek($src, $file_size - MAX_CONTENT_SIZE + 1024, 0);
        print $dst "warning: stored only last 16 Mb of the file!\n\n...\n";
        binmode $src;
        binmode $dst;
        copy($src, $dst);
        close $src;
        close $dst;
        $full_name = $tmp_file;
      }

      push @files, {name => $_,
                    path => $File::Find::dir,
                    full_name => $full_name};
      last;
    }
  }
}

find(\&fill_files_list, $full_path);

# LOAD_FILE func works fine only if source file is located on the same host as db server
my @RSYNC = ('/usr/bin/rsync',
             '-az',
             "--rsync-path",
             "mkdir -p $full_path && /usr/bin/rsync",
             '--partial',
             '-t',
             "$full_path/",
             "$db_hostname:$full_path");

system(@RSYNC) == 0
  or die "system @RSYNC failed: $?";

my $insert_hd = $dbh->prepare(q[
  INSERT INTO file (task_id, name, fullpath, content)
  VALUES (?, ?, ?, LOAD_FILE(?))
]);

foreach my $file (@files)
{
  $insert_hd->execute($task_id,             # task_id
                      $file->{name},        # name
                      $file->{path},        # fullpath
                      $file->{full_name});  # content
}

$dbh->commit;
