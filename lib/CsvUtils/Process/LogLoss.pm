package CsvUtils::Process::LogLoss;

# eval logloss

use strict;
use Class::Struct;
use List::Util qw(min max);

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::LogLoss: not defined 'field' argument";

  my $fields = {
    values_ => {},
    label_ => (exists($params{'label'}) ? $params{'label'} - 1 : 0),
    field_ => $params{'field'} - 1,
    logloss_ => 0.0,
    count_ => 0
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];
  my $label = $row->[$self->{label_}];
  my $eps = 0.00001;

  if(ref($value) eq 'ARRAY')
  {
    die "LogLoss can't be applied to array row";
  }
  else
  {
    $self->{count_} += 1;

    if($label == 0)
    {
      $self->{logloss_} -= log(1.0 - min($value, 1 - $eps));
    }
    elsif($label == 1)
    {
      $self->{logloss_} -= log(max($value, $eps));
    }
    else
    {
      die "invalid label value: $label";
    }
  }

  return $row;
}

sub flush
{
  my ($self) = @_;
  print "LogLoss for field #" . $self->{field_} . ": " . ($self->{logloss_} / $self->{count_}) .
    "(sum = " . $self->{logloss_} . ")\n";
  return undef;
}

1;
