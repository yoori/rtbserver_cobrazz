package CsvUtils::Process::Sum;

use strict;
use Class::Struct;

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::Sum: not defined 'field' argument";

  my $fields = {
    values_ => {},
    field_ => $params{'field'} - 1,
    sum_ => 0.0
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];

  if(ref($value) eq 'ARRAY')
  {
    die "Sum can't be applied to array row";
  }
  else
  {
    $self->{sum_} += $value;
  }

  return $row;
}

sub flush
{
  my ($self) = @_;
  print "Sum for field #" . $self->{field_} . ": " . ($self->{sum_}) . "\n";
}

1;
