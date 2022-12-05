#!/usr/bin/perl
use warnings;
use strict;

=head1 NAME

error_log_show.pl - script for monitoring ErrorLog DB table.

=head1 SYNOPSIS

  error_log_show.pl OPTIONS

=head1 OPTIONS

=over

=item C<--db, -d db_scheme>

B<Required>. Specifies DB scheme, where stored AdServer statistic.

=item C<--user, -u username>

B<Required>. Specifies username for connecting with DB.

=item C<--pswd, -p password>

B<Required>. Specifies password for indicated user for connecting with DB.

=item C<--last_id>

B<Optional>. Prints max erl_id (last record of ErrorLog table).
Note, that you can't use C<last_id> and C<id> options together.

=item C<--id erl_id>

B<Optional>. Prints all raws from ErrorLog table
that have erl_id field greater than indicated value.
By default erl_id = 0.
Output format:
[<ERL_DT>] [<ERL_ERM_ID>] [ERROR] [<ERL_INTERFACE_ID>] <ERL_MESSAGE>, <ERL_STACK>.

=item C<--log, -l logfile>

B<Optional>. Specifies file to output result.
By default output result to STDOUT.

=back

=cut

use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my %options = (id => 0);

my $getopt_result = GetOptions(\%options,
                               qw(last_id id=i db|d=s user|u=s pswd|p=s log|l=s help|h));

if ($getopt_result && $options{help})
{
  pod2usage( { -exitval => 0,
               -verbose => 2 } );
}

if (!$getopt_result
    || (grep { not defined } @options{qw(db user pswd)}))
{
  pod2usage(1);
}

if ($options{log}) {
  open(OUT, '>', $options{log}) or die $!;
}
else {
  open(OUT, ">&STDOUT") or die $!;
}

#open(STDOUT, '>', $options{log}) or die $! if $options{log};

use DBI;

my $dbh = DBI->connect("DBI:Oracle:$options{db}",
                       $options{user}, $options{pswd},
                       { AutoCommit => 0, PrintError => 0, RaiseError => 1,
                         LongReadLen => 4096 });

my $FORMAT = "[%s] [%s] [ERROR] [%s] %s.\nStack:\n%s";

sub get_max_erl_id
{
  my @res = $dbh->selectrow_array(qq[
    SELECT max(erl_id)
    FROM ErrorLog
    ]) or die $dbh->errstr;

  if ($res[0]) { print OUT "$res[0]\n"; }
  else {print OUT "0\n"; }
}

sub get_erl_ids
{
  my ($erl_id) = @_;
  my $query = $dbh->prepare(qq[
    SELECT ERL_DT,
           ERL_ERM_ID,
           ERL_INTERFACE_ID,
           ERL_MESSAGE,
           ERL_STACK
    FROM errorlog
    WHERE erl_id > $erl_id
  ]);
  $query->execute();

  my $empty = 1;
  while (my $error = $query->fetchrow_hashref)
  {
    printf OUT
           $FORMAT,
           $error->{ERL_DT},
           $error->{ERL_ERM_ID},
           $error->{ERL_INTERFACE_ID},
           $error->{ERL_MESSAGE},
           $error->{ERL_STACK};
    $empty = 0;
  }
  print "There is no errors with id greater than '$erl_id'.\n" if $empty;
  $query->finish();
}

if ($options{last_id})
{
  get_max_erl_id();
}
else
{
  get_erl_ids($options{id});
};

close(OUT);
exit(0);
