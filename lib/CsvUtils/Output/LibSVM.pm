package CsvUtils::Output::LibSVM;

use CsvUtils::Utils;

sub new
{
  my $class = shift;
  return bless({}, $class);
}

sub process
{
  my ($self, $row) = @_;
  my @arr = @$row;
  my $res_value = shift(@arr);
  foreach my $value(@arr)
  {
    if(ref($value) eq 'ARRAY')
    {
      foreach my $sub_value(@$value)
      {
        $res_value = $res_value . ' ' . $sub_value . ":1";
      }
    }
    else
    {
      $res_value = $res_value . ' ' . $value . ":1";
    }
  }
  print STDOUT $res_value . "\n";
  return $row;
}

sub flush
{}

1;
