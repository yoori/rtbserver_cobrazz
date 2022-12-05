package CsvUtils::Process::GainFloatGroup;

use strict;
use Class::Struct;
use List::Util qw(min max);

struct(Value => [labels => '$', total => '$']);

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::GainFloatGroup: not defined 'field' argument";

  if(exists($params{'cmp'}))
  {
    if($params{'cmp'} ne 'string' && $params{'cmp'} ne 'numeric')
    {
      die "incorrect cmp value '" . $params{'cmp'} . "'";
    }
  }

  my $fields = {
    values_ => {},
    label_ => (exists($params{'label'}) ? $params{'label'} - 1 : 0),
    field_ => $params{'field'} - 1,
    total_ => Value->new(labels => 0, total => 0),
    cmp_ => exists($params{'cmp'}) ? $params{'cmp'} : 'numeric',
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];

  my $label;
  my $orig_label = $row->[$self->{label_}];
  if($orig_label eq '0' || lc($orig_label) eq 'false')
  {
    $label = 0;
  }
  elsif($orig_label eq '1' || lc($orig_label) eq 'true')
  {
    $label = 1;
  }
  else
  {
    die "Invalid label value: $orig_label";
  }

  if($value ne '')
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

    $self->{total_}->total($self->{total_}->total() + 1);
    if($label)
    {
      $self->{total_}->labels($self->{total_}->labels() + 1);
    }
  }

  return undef;
}

sub flush
{
  my ($self) = @_;

  my $eps = 0.00001;
  my $full_labels = $self->{total_}->labels();
  my $full_total = $self->{total_}->total();

  my @keys;
  if($self->{cmp_} eq 'numeric')
  {
    @keys = sort{$a <=> $b}(keys(%{$self->{values_}}));
  }
  else
  {
    @keys = sort{$a cmp $b}(keys(%{$self->{values_}}));
  }

  # find value that give minimal logloss
  # on next iteration will be used total, labels without row's with this feature
  my $min_value;
  my $min_logloss;
  my $freq_before_point;
  my $freq_after_point;
  my $min_cur_total;

  my $cur_labels = 0;
  my $cur_total = 0;

  foreach my $value(@keys)
  {
    my $val_ref = $self->{values_}->{$value};

    my $s1 = 1.0 * $full_total - $cur_total - ($full_labels - $cur_labels);
    my $s2 = 1.0 * $full_labels - $cur_labels;

    my $s3 = 1.0 * $cur_total - $cur_labels;
    my $s4 = 1.0 * $cur_labels;

    my $p1 = $s2 > 0 ? $s2 / ($s1 + $s2) : 0.0; # freq after point
    my $p2 = $s4 > 0 ? $s4 / ($s3 + $s4) : 0.0; # freq before point

    if($s1 < 0 || $s2 < 0 || $s3 < 0 || $s4 < 0)
    {
      die "assert: s1=$s1,s2=$s2,s3=$s3,s4=$s4";
    }

    $p1 = min(max($p1, $eps), 1 - $eps);
    $p2 = min(max($p2, $eps), 1 - $eps);

    my $false_part = ($s1 > 0 ? $s1 * log(1.0 - $p1) + $s2 * log($p1) : 0.0);
    my $true_part = ($s4 > 0 ? $s3 * log(1.0 - $p2) + $s4 * log($p2) : 0.0);

    #print STDERR $value . ": $false_part $true_part (p1 = $p1, p2 = $p2, t = $cur_total, l = $cur_labels)\n";

    my $logloss = - ($false_part + $true_part);

    if(!defined($min_logloss) || $min_logloss > $logloss)
    {
      $min_value = $value;
      $min_logloss = $logloss;
      $freq_before_point = $p2; 
      $freq_after_point = $p1;
      $min_cur_total = $cur_total;
    }

    $cur_labels += $val_ref->labels();
    $cur_total += $val_ref->total();
  }

  if(defined($min_logloss))
  {
    my @res;

    $min_value = $self->{cmp_} eq 'numeric' ? sprintf("%05.6f", $min_value) : $min_value;

    push(@res, [
      '',
      $min_value,
      '<' . $min_value,
      sprintf("%05.6f", $freq_before_point), $min_cur_total]);
    push(@res, [
      $min_value,
      '',
      '>=' . $min_value,
      sprintf("%05.6f", $freq_after_point), $full_total - $min_cur_total]);

    return \@res;
  }

  return [];
}

1;
