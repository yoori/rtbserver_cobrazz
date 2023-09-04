package CsvUtils::Process::AddIndex;

use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my $fields = { index_ => 0 };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;
  push(@$row, $self->{index_});
  $self->{index_} += 1;

  return $row;
}

sub flush
{}

1;
