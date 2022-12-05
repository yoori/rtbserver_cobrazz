package CsvUtils::Process::DateLineJSON;

use strict;
use warnings;
use Encode;
use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'group_field'}) ||
    die "CsvUtils::Process::DateLineJSON: not defined 'group_field' argument";
  exists($params{'date_field'}) ||
    die "CsvUtils::Process::DateLineJSON: not defined 'date_field' argument";
  exists($params{'value_field'}) ||
    die "CsvUtils::Process::DateLineJSON: not defined 'value_field' argument";

  my @indexes = split(',', $params{'group_field'});
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
      die "CsvUtils::Process::DateLineJSON: incorrect group column index: $index";
    }
  }

  my $date_index = $params{'date_field'};
  my $value_index = $params{'value_field'};

  if(!looks_like_number($date_index))
  {
    die "CsvUtils::Process::DateLineJSON: invalid 'date_field' argument";
  }

  if(!looks_like_number($value_index))
  {
    die "CsvUtils::Process::DateLineJSON: invalid 'value_field' argument";
  }

  my $fields = {
    field_ => \@res_indexes,
    field_set_ => \%res_indexes_set,
    date_field_ => $date_index - 1,
    value_field_ => $value_index - 1,
    csv_ => Text::CSV_XS->new({ binary => 1, eol => $/ }),
    group_empty_ => (exists($params{'group_empty'}) ? $params{'group_empty'} : 0),
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @group_key;
  my $group_key_str;
  my $all_empty = 1;

  foreach my $field_index(@{$self->{field_}})
  {
    if($row->[$field_index] ne '')
    {
      $all_empty = 0;
    }
    push(@group_key, $row->[$field_index]);
  }

  {
    my $res_group_key = CsvUtils::Utils::prepare_row(\@group_key);
    $self->{csv_}->combine(@$res_group_key);
    $group_key_str = $self->{csv_}->string();
  }

  Encode::_utf8_on($group_key_str);

  if($all_empty != 0 && $self->{group_empty_} == 0)
  {
    return $row;
  }
  elsif(!exists($self->{rows_}->{$group_key_str}))
  {
    my @res;
    for(my $i = 0; $i < scalar(@$row); ++$i)
    {
      if(exists($self->{field_set_}->{$i}))
      {
        push(@res, $row->[$i]);
      }
      elsif(ref($row->[$i]) eq 'ARRAY')
      {
        push(@res, $row->[$i]);
      }
      else
      {
        push(@res, [ $row->[$i] ]);
      }
    }
    $self->{rows_}->{$group_key_str} = \@res;

    #print "I: " . $res[$self->{field_}] . "\n";
  }
  else
  {
    my $res = $self->{rows_}->{$group_key_str};

    for(my $i = 0; $i < scalar(@$row); ++$i)
    {
      if(exists($self->{field_set_}->{$i}))
      {}
      elsif(ref($row->[$i]) eq 'ARRAY')
      {
        push(@{$res->[$i]}, @{$row->[$i]});
      }
      else
      {
        push(@{$res->[$i]}, $row->[$i]);
      }
    }

    #print "I: " . $res->[$self->{field_}] . "\n";
  }

  return undef;
}

sub flush
{
  my ($self) = @_;

  my @rows;
  foreach my $group_key(keys %{$self->{rows_}})
  {
    # parse date & value arrays
    my $row = $self->{rows_}->{$group_key};
    my $date_array = $row->[$self->{date_field_}];
    my $value_array = $row->[$self->{value_field_}];

    if(scalar(@$date_array) != scalar(@$value_array))
    {
      die "CsvUtils::Process::DateLineJSON: date array size not equal to value array size (" .
        scalar(@$date_array) . " dates, " . scalar(@$value_array) . " values)";
    }

    my $json = "[";
    for(my $i = 0; $i < scalar(@$date_array); ++$i)
    {
      $json .= ($i > 0 ? "," : "") . '{' .
        '"date":"' . $date_array->[$i] . '",' .
        '"pdz":' . ($value_array->[$i] > 0 ? 1 : 0) . ',' .
        '"pdzSum":' .  $value_array->[$i] .
      "}";
    }
    $json .= "]";

    my @res_row = @$row;
    push(@res_row, $json);
    push(@rows, \@res_row);
  }

  return \@rows;
}

1;
