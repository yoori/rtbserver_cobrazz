package CsvUtils::Process::Group;

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
      die "CsvUtils::Process::Group: incorrect column index: $index";
    }
  }

  my $fields = {
    field_ => \@res_indexes,
    field_set_ => \%res_indexes_set,
    rows_ => {},
    csv_ => Text::CSV_XS->new({ binary => 1, eol => $/ }),
    group_empty_ => (exists($params{'group_empty'}) ? $params{'group_empty'} : 0),
    empty_absorb_ => (exists($params{'empty_absorb'}) ? $params{'empty_absorb'} : 0),
    ordered_ => (exists($params{'ordered'}) ? $params{'ordered'} : 0),
    prev_group_key_str_ => '',
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

  my $ret = undef;

  if($self->{ordered_} > 0)
  {
    if($group_key_str ne $self->{prev_group_key_str_})
    {
      my $flush_rows = $self->flush();

      if(scalar(@$flush_rows) > 1)
      {
        die "Group: assert, flush rows > 1";
      }
      elsif(scalar(@$flush_rows) > 0)
      {
        $ret = $flush_rows->[0];
      }

      $self->{rows_} = {};
    }

    $self->{prev_group_key_str_} = $group_key_str;
  }

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

  return $ret;
}

sub key_empty_
{
  my ($key) = @_;

  foreach my $field_val(@$key)
  {
    if($field_val ne '')
    {
      return 0;
    }
  }

  return 1;
}

sub flush
{
  my ($self) = @_;

=begin comment
  if($self->{empty_absorb_} != 0)
  {
    my @keys = keys %{$self->{rows_}};

    my @empty_key_rows;
    my %non_empty_key_rows;
    for(my $i = 0; $i < scalar(@keys); ++$i)
    {
      my $key_empty = key_empty_($key);
      if($key_empty > 0)
      {
        push(@empty_key_rows, @{$self->{rows_}});
      }
      else
      {
        $non_empty_key_rows{$key} 
      }
    }




    my %key_to_save_key;

    for(my $i = 0; $i < scalar(@keys); ++$i)
    {
      my @save_key;

      my $key1 = $keys->[$i];
      my $key_empty = key_empty_($key1);

      if($key_empty == 0)
      {
        for(my $j = $i + 1; $j < scalar(@keys); ++$i)
        {
          my $key2 = $keys->[$j];
          $key_empty = key_empty_($key2);

          if($key_empty == 0)
          {
            my @res_key;
            for(my $field_i = 0; $field_i < scalar(@$key1); ++$field_i)
            {
              if($key1->[$field_i] eq $key2->[$field_i] || $key1->[$field_i] eq '' || $key1->[$field_i] eq '')
              {
              }
            }
          }
        }
      }
    }

    my @rows;
    foreach my $group_key(keys %{$self->{rows_}})
    {
      push(@rows, $self->{rows_}->{$group_key});
    }
  }
=end comment
=cut

  my @rows;
  foreach my $group_key(keys %{$self->{rows_}})
  {
    push(@rows, $self->{rows_}->{$group_key});
  }

  return \@rows;
}

1;
