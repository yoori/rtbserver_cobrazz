package CsvUtils::Utils;

sub prepare_row
{
  my ($row) = @_;

  my @res_row;
  foreach my $value(@$row)
  {
    if(ref($value) eq 'ARRAY')
    {
      push(@res_row, '|' . join('|', @$value) . '|');
    }
    else
    {
      push(@res_row, $value);
    }
  }

  return \@res_row;
}

sub find_value_in_row
{
  my ($value, $filter) = @_;
  if(ref($value) eq 'ARRAY')
  {
    foreach my $sub_val(@$value)
    {
      if(exists($filter->{$sub_val}))
      {
        return 1;
      }
    }
  }
  else
  {
    if(exists($filter->{$value}))
    {
      return 1;
    }
  }

  return undef;
}

1;
