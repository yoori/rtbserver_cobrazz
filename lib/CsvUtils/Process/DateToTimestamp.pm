package CsvUtils::Process::DateToTimestamp;

use strict;
use DateTime;
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::DateToTimestamp: not defined 'field' argument";

  my $fields = { field_ => $params{'field'} - 1};

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $time_str = $row->[$self->{field_}];
  my $time_epoch = '';

  if($time_str =~ m|^(\d{4})-(\d{2})-(\d{2})$|)
  {
    $time_epoch = timelocal_nocheck(0, 0, 0, $3, $2, $1);
  }

  push(@$row, $time_epoch);

  return $row;
}

sub flush
{}

1;
