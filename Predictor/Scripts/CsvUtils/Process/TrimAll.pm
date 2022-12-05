package CsvUtils::Process::TrimAll;

sub new
{
  my $class = shift;
  return bless({}, $class);
}

sub process
{
  my ($self, $row) = @_;

  for(my $index = 0; $index < scalar(@$row); ++$index)
  {
    my $value = $row->[$index];
    if(defined($value))
    {
      if(ref($value) eq 'ARRAY')
      {
        my @res_arr;
        foreach my $sub_val(@$value)
        {
          $sub_val =~ s/^\s+|\s+$//g;
          $sub_val =~ s/\n/ /g;
          push(@res_arr, $sub_val);
        }
        $row->[$index] = \@res_arr;
      }
      else
      {
        $value =~ s/^\s+|\s+$//g;
        $value =~ s/\n/ /g;
        $row->[$index] = $value;
      }
    }
  }

  return $row;
}

sub flush
{}

1;
