package CsvUtils::Process::ColumnDistribution;

# print label probability distribution for each value

use strict;
use Class::Struct;
use List::Util qw(min max);

struct(Value => [labels => '$', total => '$']);

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::ColumnDistribution: not defined 'field' argument";

  my $fields = {
    values_ => {},
    label_ => (exists($params{'label'}) ? $params{'label'} - 1 : 0),
    field_ => $params{'field'} - 1,
    total_ => Value->new(labels => 0, total => 0),
    print_value_logloss_ => (exists($params{'logloss'}) ? $params{'logloss'} : 0)
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];
  my $label = ($row->[$self->{label_}] ne '0' ? 1 : undef);

  if(ref($value) eq 'ARRAY')
  {
    foreach my $sub_val(@$value)
    {
      if(!exists($self->{values_}->{$sub_val}))
      {
        $self->{values_}->{$sub_val} = Value->new(labels => 0, total => 0);
      }

      my $val_ref = $self->{values_}->{$sub_val};
      $val_ref->total($val_ref->total() + 1);
      if($label)
      {
        $val_ref->labels($val_ref->labels() + 1);
      }
    }
  }
  else
  {
    if(!exists($self->{values_}->{$value}))
    {
      $self->{values_}->{$value} = Value->new(labels => 0, total => 0);
    }

    my $val_ref = $self->{values_}->{$value};
    $val_ref->total($val_ref->total() + 1);
    if($label)
    {
      $val_ref->labels($val_ref->labels() + 1);
    }
  }

  $self->{total_}->total($self->{total_}->total() + 1);
  if($label)
  {
    $self->{total_}->labels($self->{total_}->labels() + 1);
  }

  return $row;
}

sub flush
{
  my ($self) = @_;
  my $eps = 0.00000001;

  print "AVG RATE = " . (1.0 * $self->{total_}->labels() / $self->{total_}->total()) . "\n";

  foreach my $value(keys %{$self->{values_}})
  {
    my $val_ref = $self->{values_}->{$value};
    my $logloss = 0;
    if($self->{print_value_logloss_} > 0)
    {
      my $s1 = 1.0 * $self->{total_}->total() - $val_ref->total() - ($self->{total_}->labels() - $val_ref->labels());
      my $s2 = 1.0 * $self->{total_}->labels() - $val_ref->labels();
      my $s3 = 1.0 * $val_ref->total() - $val_ref->labels();
      my $s4 = 1.0 * $val_ref->labels();
      my $p1 = $s2 > 0 ? $s2 / ($s1 + $s2) : 0.0;
      my $p2 = $s4 > 0 ? $s4 / ($s3 + $s4) : 0.0;
      if($s1 < 0 || $s2 < 0 || $s3 < 0 || $s4 < 0)
      {
        die "assert: s1=$s1,s2=$s2,s3=$s3,s4=$s4";
      }
      $p1 = min(max($p1, $eps), 1 - $eps);
      $p2 = min(max($p2, $eps), 1 - $eps);

      my $false_part = ($s1 > 0 ? $s1 * log(1.0 - $p1) + $s2 * log($p1) : 0.0);
      my $true_part = ($s4 > 0 ? $s3 * log(1.0 - $p2) + $s4 * log($p2) : 0.0);

      $logloss = - ($false_part + $true_part);
      # less logloss is good
      #print STDERR "p1 = $p1, p2 = $p2, s1 = $s1, s2 = $s2, s3 = $s3, s4 = $s4, logloss = $logloss\n";
    }

    print STDOUT "" .
     ($self->{print_value_logloss_} > 0 ? sprintf("%016.15f", $logloss) . "," : "") .
     sprintf("%f", $val_ref->labels() * 1.0 / $val_ref->total()) . "," .
     $val_ref->total() . "," .
     $value . "\n";
  }
}

1;
