package CsvUtils::Input::Std;

use Text::CSV;

sub new
{
  my $class = shift;
  my $fields = { csv_ => Text::CSV->new({ binary => 1, eol => $/ }) };
  return bless($fields, $class);
}

sub get
{
  my ($self) = @_;

  my $line = <STDIN>;
  if(defined($line))
  {
    chomp $line;
    $self->{csv_}->parse($line);
    my @arr = $self->{csv_}->fields();
    return \@arr;
  }

  return undef;
}

1;
