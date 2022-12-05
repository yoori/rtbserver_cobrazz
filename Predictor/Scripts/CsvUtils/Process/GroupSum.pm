package CsvUtils::Process::GroupSum;

use strict;
use warnings;
use Encode;
use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Group: not defined 'field' argument";

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
      die "CsvUtils::Process::GroupSum: incorrect column index: $index";
    }
  }

  my $fields = {
    field_ => \@res_indexes,
    field_set_ => \%res_indexes_set,
    rows_ => {},
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
      else
      {
        if(!looks_like_number($row->[$i]))
        {
          die "CsvUtils::Process::GroupSum: field isn't number(field #" . $i . "): " . $row->[$i];
        }

        push(@res, $row->[$i]);
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
      else
      {
        if(!looks_like_number($row->[$i]))
        {
          die "CsvUtils::Process::GroupSum: field isn't number(field #" . $i . "): " . $row->[$i];
        }

        $res->[$i] += $row->[$i];
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
    #print "EKEY: $group_key\n";
    #print "F: " . $self->{rows_}->{$group_key}->[$self->{field_}] . "\n";
    push(@rows, $self->{rows_}->{$group_key});
  }

  return \@rows;
}

1;
