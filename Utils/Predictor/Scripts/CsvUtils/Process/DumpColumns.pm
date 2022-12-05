package CsvUtils::Process::DumpColumns;

use Text::CSV;
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my $fields = { csv_ => Text::CSV->new({ binary => 1, eol => $/ }), dumped_ => undef };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  if(!defined($self->{dumped_}))
  {
    my $index = 1;
    foreach my $value(@$row)
    {
      print "$index: $value\n";
      ++$index;
    }

    $self->{dumped_} = 1;
  }

  return $row;
}

1;
