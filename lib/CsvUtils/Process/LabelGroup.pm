package CsvUtils::Process::LabelGroup;

use strict;
use warnings;
use Class::Struct;
use Scalar::Util qw(looks_like_number);

struct(Value => [labels => '$', total => '$', rate => '$', value => '$']);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Columns: not defined 'field' argument";

  my $fields = {
    values_ => {},
    label_ => (exists($params{'label'}) ? $params{'label'} - 1 : 0),
    field_ => $params{'field'} - 1,
    min_total_ => $params{'min'},
    divs_ => $params{'divs'}
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

  return $row;
}

sub flush
{
  my ($self) = @_;

  my @res;
  my @und_res;
  my $full_total = 0;

  foreach my $key(keys %{$self->{values_}})
  {
    my $value = $self->{values_}->{$key};
    $value->value($key);
    if(defined($self->{min_total_}) && $value->total() < $self->{min_total_})
    {
      $value->rate(-1.0);
      push(@und_res, $value);
    }
    else
    {
      my $rate = 1.0 * $value->labels() / $value->total();
      $value->rate($rate);
      push(@res, $value);
      $full_total += $value->total();
    }
  }

  my $csv = Text::CSV->new({ binary => 1, eol => $/ });

  @res = sort {$a->rate() <=> $b->rate()} @res;

  if(!defined($self->{divs_}))
  {
    foreach my $res_val(@und_res)
    {
      print sprintf("%05.6f", $res_val->rate()) . "," . $res_val->value() . "\n";
    }

    foreach my $res_val(@res)
    {
      print sprintf("%05.6f", $res_val->rate()) . "," . $res_val->value() . "\n";
    }
  }
  else
  {
    foreach my $res_val(@und_res)
    {
      my @res_row;
      push(@res_row, $res_val->value());
      push(@res_row, -1);
      $csv->print(\*STDOUT, \@res_row);
    }

    my $arr_i = 0;
    my $cur_total = 0;
    my $prev_rate = -1;
    my $next_total = $full_total / $self->{divs_};

    for(my $i = 1; $i <= $self->{divs_}; ++$i)
    {
      while($arr_i < scalar(@res) && (
        $cur_total < $next_total || $res[$arr_i]->rate() != $prev_rate))
      {
        my @res_row;
        push(@res_row, $res[$arr_i]->value());
        push(@res_row, $i);
        $csv->print(\*STDOUT, \@res_row);

        $cur_total += $res[$arr_i]->total();
        $prev_rate = $res[$arr_i]->rate();

        ++$arr_i;
      }

      $next_total = $i < $self->{divs_} ?
        ($cur_total + ($full_total - $cur_total) / ($self->{divs_} - $i)) :
        $full_total;
    }
  }
}

1;
