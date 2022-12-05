package CsvUtils::Process::DeepTreeLearn;

# print table
#   <number of nodes>: <logloss>

use strict;
use Class::Struct;
use List::Util qw(min max);

struct(Value => [labels => '$', total => '$']);

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::StrictTreeLearn: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::Columns: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  my $fields = {
    values_ => {},
    label_ => (exists($params{'label'}) ? $params{'label'} - 1 : 0),
    total_ => Value->new(labels => 0, total => 0),
    fields_ => \@res_indexes,
    deep_ => $params{'deep'}
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $label = ($row->[$self->{label_}] ne '0' ? 1 : undef);

  foreach my $index(@{$self->{fields_}})
  {
    my $value = $row->[$index];

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

  my $i = 0;

  my $full_labels = $self->{total_}->labels();
  my $full_total = $self->{total_}->total();
  my $steps = $self->{steps_};

  while(%{$self->{values_}} && (!defined($steps) || $i < $steps)) # not empty values_
  {
    # find value that give minimal logloss
    # on next iteration will be used total, labels without row's with this feature
    my $min_value;
    my $min_logloss;
    my $min_p1;
    my $min_p2;
    my $true_part; 
    my $false_part;
    my $min_value_total;
    my $agg_logloss = 0;
    my $add_logloss = 0;

    foreach my $value(keys %{$self->{values_}})
    {
      my $val_ref = $self->{values_}->{$value};

      my $s1 = 1.0 * $full_total - $val_ref->total() - ($full_labels - $val_ref->labels());
      my $s2 = 1.0 * $full_labels - $val_ref->labels();

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

      $false_part = ($s1 > 0 ? $s1 * log(1.0 - $p1) + $s2 * log($p1) : 0.0);
      $true_part = ($s4 > 0 ? $s3 * log(1.0 - $p2) + $s4 * log($p2) : 0.0);

      my $logloss = $agg_logloss - ($false_part + $true_part);

      if(!defined($min_logloss) || $min_logloss > $logloss)
      {
        $min_value = $value;
        $min_logloss = $logloss;
        $min_p1 = $p1;
        $min_p2 = $p2;
        $min_value_total = $val_ref->total();
      }
    }

    my $val_ref = $self->{values_}->{$min_value};

    $full_labels -= $val_ref->labels();
    $full_total -= $val_ref->total();
    $agg_logloss -= $true_part;

    delete $self->{values_}->{$min_value};

    print sprintf("%03d", $i) . ": " . sprintf("%016.15f", $min_logloss) .
      ",value=$min_value, " .
      ",cover=" . sprintf("%05.3f", 1.0 * $val_ref->total() / $self->{total_}->total()) .
      ",rate=" . sprintf("%06.5f", 1.0 * $val_ref->labels() / $val_ref->total()) .
      ",other_rate=" .
        sprintf("%06.5f", $self->{total_}->total() - $val_ref->total() > 0 ?
          1.0 * ($self->{total_}->labels() - $val_ref->labels()) / ($self->{total_}->total() - $val_ref->total()) :
          0.0) .
      "(labels=" . $val_ref->labels() .
      ",total=" . $val_ref->total() .
      ")\n";

    ++$i;
  }
}

1;
