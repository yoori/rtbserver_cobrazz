package CsvUtils::Process::EmptyAbsorb;

use strict;
use warnings;
use Encode;
use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::EmptyAbsorb: not defined 'field' argument";
  exists($params{'absorb_field'}) ||
    die "CsvUtils::Process::EmptyAbsorb: not defined 'absorb_field' argument";

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
      die "CsvUtils::Process::EmptyAbsorb: incorrect column index: $index";
    }
  }

  my $fields = {
    field_ => \@res_indexes,
    field_set_ => \%res_indexes_set,
    rows_ => {},
    csv_ => Text::CSV_XS->new({ binary => 1, eol => $/ }),
    group_empty_ => (exists($params{'group_empty'}) ? $params{'group_empty'} : 0),
    absorb_field_ => $params{'absorb_field'} - 1
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
    my @res_row = @$row;
    $self->{rows_}->{$group_key_str} = [ \@res_row ];
  }
  else
  {
    push(@{$self->{rows_}->{$group_key_str}}, $row);
  }

  return undef;
}

sub flush
{
  my ($self) = @_;

  my @rows;

  foreach my $group_key(keys %{$self->{rows_}})
  {
    my %absorb_keys;

    foreach my $row(@{$self->{rows_}->{$group_key}})
    {
      my $absorb_val = $row->[$self->{absorb_field_}];
      $absorb_keys{$absorb_val} = 1;
    }

    my $save_absorb;

    if(exists($absorb_keys{''}))
    {
      if(scalar(keys(%absorb_keys)) == 2)
      {
        my @keys = keys(%absorb_keys);
        $save_absorb = $keys[1];
      }
    }

    if(defined($save_absorb))
    {
      my $group_rows = $self->{rows_}->{$group_key};

      my @res_row;

      {
        my @first_row = @{$group_rows->[0]};
        $first_row[$self->{absorb_field_}] = $save_absorb;

        for(my $i = 0; $i < scalar(@first_row); ++$i)
        {
          if($i != $self->{absorb_field_})
          {
            if(exists($self->{field_set_}->{$i}))
            {
              push(@res_row, $first_row[$i]);
            }
            elsif(ref($first_row[$i]) eq 'ARRAY')                                                                                        
            {
              push(@res_row, $first_row[$i]);
            }
            else
            {
              push(@res_row, [ $first_row[$i] ]);
            }
          }
          else
          {
            push(@res_row, $save_absorb);
          }
        }
      }

      for(my $i = 1; $i < scalar(@$group_rows); ++$i)
      {
        my $row = $group_rows->[$i];

        for(my $field_i = 1; $field_i < scalar(@res_row); ++$field_i)
        {
          if($field_i != $self->{absorb_field_})
          {
            if(exists($self->{field_set_}->{$field_i}))
            {}
            elsif(ref($row->[$field_i]) eq 'ARRAY')                                                                                 
            {
              push(@{$res_row[$field_i]}, @{$row->[$field_i]});
            }
            else
            {
              push(@{$res_row[$field_i]}, $row->[$field_i]);
            }
          }
        }
      }

      push(@rows, \@res_row);
    }
    else # !defined($save_absorb)
    {
      foreach my $row(@{$self->{rows_}->{$group_key}})
      {
        push(@rows, $row);
      }
    }
  }

  return \@rows;
}

1;
