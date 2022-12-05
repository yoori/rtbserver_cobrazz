#! /usr/bin/perl
# -*- cperl -*-

=head1 NAME

taskbot_db_clean.pl - remove obsolete tasks from taskbot DB.

=head1 SYNOPSIS

  taskbot_db_clean.pl OPTIONS

=head1 OPTIONS

=over

=item C<--days, -d how many days must stay in taskbot DB>

B<Required.>  Specify database size (in days) after cleaning.

=over

=item C<-v>

Only verbose information without deletion.

=item C<-vv>

Deletion with verbose.

=back

=cut



use Getopt::Long qw(:config gnu_getopt pass_through);
use Pod::Usage;

use warnings;
use strict;
use DBI;

my $verbose = 0;
my %option = (verbose => \$verbose);

if (! GetOptions(\%option, qw(days|d=i verbose|v+))
    || (grep { not defined } @option{qw(days)})) {
    pod2usage(1);
}


my $time = time - $option{days} * 24 * 60 * 60;

my $dbh = DBI->connect("DBI:mysql:taskbot;host=taskbot.ocslab.com", 
                       "taskbot", "taskbot",
                       { AutoCommit => 0, PrintError => 0, RaiseError => 1,
                         FetchHashKeyName => 'NAME_lc' });

my $remove_reports_sql = "DELETE FROM report WHERE task_id is null";

# Verbose
my ($tasks_count, $min_task_id, $max_task_id) = 
    $dbh->selectrow_array(q[
                            SELECT COUNT(id), IFNULL(MIN(id), 0), IFNULL(MAX(id), 0)
                            FROM task
                            WHERE start < ?
                            ], undef, $time);

my ($start_min, $start_max) = (0, 0);
if ($min_task_id)
{
  $start_min = 
      $dbh->selectrow_array(q[
                              SELECT start
                              FROM task
                              WHERE  id = ?
                              ], undef, $min_task_id);
}

if ($max_task_id)
{
  $start_max = 
      $dbh->selectrow_array(q[
                              SELECT start
                              FROM task
                              WHERE  id = ?
                              ], undef, $max_task_id);
}


my $t_min = time_str($start_min);
my $t_max = time_str($start_max);

 
my ($reports_count, $min_report_id, $max_report_id) = 
    $dbh->selectrow_array(q[
                            SELECT COUNT(id), IFNULL(MIN(id), 0), IFNULL(MAX(id), 0)
                            FROM report
                            WHERE task_id is null
                            ]);
# Remove data

for (my $i = $min_task_id; $i <= $max_task_id; ++$i)
{
  my $remove_task_sql = "DELETE FROM task where id = $i";
  if ($verbose > 0)
  {
    print "$remove_task_sql\n";
  }
  if ($verbose != 1)
  {
    my $stat = $dbh->prepare($remove_task_sql);
    $stat->execute();
    $dbh->commit();
  }
}

if ($verbose > 0)
{
  print "Remove $tasks_count tasks from $min_task_id ($t_min) to $max_task_id ($t_max)\n";
}

if ($verbose != 1)
{
  my $stat = $dbh->prepare($remove_reports_sql);
  $stat->execute();
  $dbh->commit();
};

if ($verbose > 0)
{
  print "$remove_reports_sql\n";
  print "Remove $reports_count tasks from $min_report_id to $max_report_id\n"; 
}
  

print "Data was successfully removed\n";

  
sub time_str {
    my ($time) = @_;

    return "unknown" unless $time;

    use POSIX qw(strftime);

    strftime("%Y-%m-%d %H:%M:%S", localtime($time))
      . sprintf(".%05d",
                int(($time - int($time)) * 10 ** 5));
}






