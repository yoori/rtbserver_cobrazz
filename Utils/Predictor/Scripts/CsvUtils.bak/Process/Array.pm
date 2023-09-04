package CsvUtils::Process::Array;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::InFile: not defined 'field' argument";

  my $fields = { field_ => $params{'field'} - 1 };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];
  if(defined($value))
  {
    my @arr = split('\|', $value);
    @arr = grep { $_ ne '' } @arr;
    $row->[$self->{field_}] = \@arr;
  }

  return $row;
}

sub flush
{}

1;
