package CsvUtils::Process::NotInFile;

use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'file'}) ||
    die "CsvUtils::Process::NotInFile: not defined 'file' argument";
  exists($params{'field'}) ||
    die "CsvUtils::Process::NotInFile: not defined 'field' argument";

  # parse file
  my %values;
  my $file = $params{'file'};

  my $csv = Text::CSV_XS->new({ binary => 1, eol => $/ });
  open FILE, $file or die "Can't open $file";
  while(<FILE>)
  {
    my $line = $_;
    chomp $line;
    $csv->parse($line);
    my @arr = $csv->fields();
    if(scalar(@arr) > 0)
    {
      #print "A: $line => <" . join(',', @arr) . ">\n";
      $values{$arr[0]} = 1;
    }
  }
  close FILE;

  my $fields = { values_ => \%values, field_ => $params{'field'} - 1 };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  if(CsvUtils::Utils::find_value_in_row($row->[$self->{field_}], $self->{values_}))
  {
    return undef;
  }

  return $row;
}

sub flush
{}

1;
