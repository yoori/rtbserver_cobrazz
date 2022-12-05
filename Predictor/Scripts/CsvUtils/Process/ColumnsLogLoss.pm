package CsvUtils::Process::ColumnsLogLoss;

# eval logloss

use strict;
use Class::Struct;
use List::Util qw(min max);

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::ColumnsLogLoss: not defined 'field' argument";

  my $fields = {
    values_ => {},
    label_ => (exists($params{'label'}) ? $params{'label'} - 1 : 0),
    field_ => $params{'field'} - 1
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
    my $logloss;

    if($label == 0)
    {
      $logloss = - log(1.0 - min($value, 1 - $eps));
    }
    elsif($label == 1)
    {
      $logloss = - log(max($value, $eps));
    }
    else
    {
      die "invalid label value: $label";
    }

    push(@$row, sprintf("%f", $logloss));
  }

  return $row;
}

sub flush
{}

1;
