package CsvUtils::Process::QuadLoss;

# eval quad loss

use strict;
use Class::Struct;
use List::Util qw(min max);

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::QuadLoss: not defined 'field' argument";

  my $fields = {
    values_ => {},
    label_ => (exists($params{'label'}) ? $params{'label'} - 1 : 0),
    field_ => $params{'field'} - 1,
    loss_ => 0.0
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];
  my $label = $row->[$self->{label_}];

  if(ref($value) eq 'ARRAY')
  {
    die "QuadLoss can't be applied to array row";
  }
  else
  {
    if($label == 0)
    {
      $self->{loss_} += $value * $value;
    }
    elsif($label == 1)
    {
      $self->{loss_} += (1 - $value) * (1 - $value);
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
  print "QuadLoss for field #" . $self->{field_} . ": " . $self->{loss_} . "\n";
}

1;
