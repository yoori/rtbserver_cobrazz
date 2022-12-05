package CsvUtils::Process::Ensemble;

use strict;
use warnings;
use Encode;
use Scalar::Util qw(looks_like_number);
use Class::Struct;
use List::Util qw(min max);
use CsvUtils::Utils;

struct(EnsembleValue => [labels => '$', total => '$']);
struct(EnsembleGroup => [predict_keys => '$', value => '$']);

use constant TRACE => 0;
use constant NET_DIV => 10;
use constant DEPTH => 10;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Ensemble: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  my %res_indexes_set;
  foreach my $index(@indexes)
  {
    if(looks_like_number($index))
    {
      $res_indexes_set{$index - 1} = 1;
      push(@res_indexes, $index - 1);
    }
    else
    {
      die "CsvUtils::Process::Ensemble: incorrect column index: $index";
    }
  }

  my $fields = {
    label_ => (exists($params{'label'}) ? $params{'label'} - 1 : 0),
    field_ => \@res_indexes,
    field_set_ => \%res_indexes_set,
    rows_ => {},
    csv_ => Text::CSV_XS->new({ binary => 1, eol => $/ }),
    div_ => (exists($params{'div'}) ? $params{'div'} : NET_DIV),
    depth_ => (exists($params{'depth'}) ? $params{'depth'} : DEPTH),
    global_ => EnsembleValue->new(labels => 0, total => 0),
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @group_key;
  my $group_key_str;

  foreach my $field_index(@{$self->{field_}})
  {
    my $p = - log(1 / $row->[$field_index] - 1);
    push(@group_key, $p);
    #push(@group_key, $row->[$field_index]);
  }

  {
    my $res_group_key = CsvUtils::Utils::prepare_row(\@group_key);
    $self->{csv_}->combine(@$res_group_key);
    $group_key_str = $self->{csv_}->string();
  }

  Encode::_utf8_on($group_key_str);

  if(!exists($self->{rows_}->{$group_key_str}))
  {
    $self->{rows_}->{$group_key_str} = EnsembleValue->new(
      labels => $row->[$self->{label_}],
      total => 1);
  }
  else
  {
    my $ens_value = $self->{rows_}->{$group_key_str};
    $ens_value->labels($ens_value->labels() + $row->[$self->{label_}]);
    $ens_value->total($ens_value->total() + 1);
  }

  $self->{global_}->labels($self->{global_}->labels() + $row->[$self->{label_}]);
  $self->{global_}->total($self->{global_}->total() + 1);

  return $row;
}

sub flush
{
  my ($self) = @_;

  my ($min_ll, $min_point) = $self->eval_();

  print "LOGLOSS = " . ($min_ll / $self->{global_}->total()) . ", POINT = [" . join(',', @$min_point) . "]\n";

  return undef;
}

sub eval_
{
  my ($self) = @_;

  my @groups;

  foreach my $group_key(keys %{$self->{rows_}})
  {
    $self->{csv_}->parse($group_key);
    my @group_fields = $self->{csv_}->fields();

    push(@groups, EnsembleGroup->new(
      predict_keys => \@group_fields, value => $self->{rows_}->{$group_key}));
  }

  if(scalar(@groups) > 0)
  {
    my $min_val;
    my $min_point;

    my @corner1_arr;
    my @corner2_arr;
    my $group0 = $groups[0];
    foreach my $i(@{$group0->predict_keys()})
    {
      push(@corner1_arr, 0);
      push(@corner2_arr, 1);
    }

    my $cur_corner1 = \@corner1_arr;
    my $cur_corner2 = \@corner2_arr;

    for(my $step = 0; $step < $self->{depth_}; ++$step)
    {
      ($min_val, $min_point) = $self->eval_for_rectangle_(
        \@groups,
        [],
        $cur_corner1,
        $cur_corner2);

      # reeval corners
      my @new_corner1;
      my @new_corner2;
      for(my $i = 0; $i < @$min_point; ++$i)
      {
        #push(@new_corner1, $min_point->[$i] - ($cur_corner2->[$i] - $cur_corner1->[$i]) / 10);
        #push(@new_corner2, $min_point->[$i] + ($cur_corner2->[$i] - $cur_corner1->[$i]) / 10);
        push(@new_corner1, max($min_point->[$i] - ($cur_corner2->[$i] - $cur_corner1->[$i]) / 10, 0));
        push(@new_corner2, min($min_point->[$i] + ($cur_corner2->[$i] - $cur_corner1->[$i]) / 10, 1));
      }

      if(TRACE)
      {
        print "TOPTOPEVAL [" . join(',', @$min_point) . "] = " . $min_val .
          ", c1 = [" . join(',', @new_corner1) . "]" .
          ", c2 = [" . join(',', @new_corner2) . "]" .
          "\n";
      }

      $cur_corner1 = \@new_corner1;
      $cur_corner2 = \@new_corner2;

      print "step #" . $step . ": p = [" . join(',', @$min_point) . "], ll = $min_val" .
        ", c1 = [" . join(',', @new_corner1) . "]" .
        ", c2 = [" . join(',', @new_corner2) . "]" .
        "\n";
    }

    return ($min_val, $min_point);
  }

  return (0, [0]);
}

sub eval_for_rectangle_
{
  my ($self, $groups, $fix_point, $corner1, $corner2) = @_;

  my $fetch_coord_i = scalar(@$fix_point);

  my $eval_min;
  my $eval_min_point;

  if(scalar(@$corner1) > 0)
  {
    my @corner1_copy = @$corner1;
    shift(@corner1_copy);
    my @corner2_copy = @$corner2;
    shift(@corner2_copy);

    if(TRACE)
    {
      print "EVAL_REC [" . join(',', @$fix_point) . "]" .
        ", c1 = [" . join(',', @$corner1) . "]" .
        ", c2 = [" . join(',', @$corner2) . "]" .
        ", cc1 = [" . join(',', @corner1_copy) . "]" .
        ", cc2 = [" . join(',', @corner2_copy) . "]" .
        "\n";
    }

    my $first_coord_step = ($corner2->[0] - $corner1->[0]) / $self->{div_};
    my @sub_arr = (@$fix_point, 0);

    for(my $first_coord = $corner1->[0];
      $first_coord <= $corner2->[0] + 0.00001; $first_coord += $first_coord_step)
    {
      $sub_arr[$fetch_coord_i] = $first_coord;

      my ($local_min, $local_min_point) = $self->eval_for_rectangle_(
        $groups,
        \@sub_arr,
        \@corner1_copy,
        \@corner2_copy);

      if(!defined($eval_min) || $local_min < $eval_min)
      {
        $eval_min = $local_min;
        $eval_min_point = $local_min_point;
      }
    }
  }
  else
  {
    # eval logloss
    my $eps = 0.00001;
    $eval_min_point = [@$fix_point];
    my $ll = 0;

    for(my $i = 0; $i < scalar(@$groups); ++$i)
    {
      my $predict_keys = $groups->[$i]->predict_keys();
      my $value = 0;

      for(my $j = 0; $j < scalar(@$predict_keys); ++$j)
      {
        $value += $predict_keys->[$j] * $fix_point->[$j];
      }

      $value = 1 / (1 + exp(- $value));
      #$value = min(max($value, 0), 1);

      my $l0 = - log(1.0 - min($value, 1 - $eps));
      my $l1 = - log(max($value, $eps));

      if($groups->[$i]->value()->total() < $groups->[$i]->value()->labels())
      {
        die "assert";
      }

      $ll += ($groups->[$i]->value()->total() - $groups->[$i]->value()->labels()) * $l0 +
        $groups->[$i]->value()->labels() * $l1;

      #if($ll < 0)
      #{
      #  die "assert: ll < 0, l0 = $l0, l1 = $l1";
      #}
    }

    if(TRACE)
    {
      print "EVAL [" . join(',', @$fix_point) . "] = " . $ll .
        ", c1 = [" . join(',', @$corner1) . "]" .
        ", c2 = [" . join(',', @$corner2) . "]" .
        "\n";
    }

    $eval_min = $ll;
  }

  return ($eval_min, $eval_min_point);
}

1;
