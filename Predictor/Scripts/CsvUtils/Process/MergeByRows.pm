package CsvUtils::Process::MergeByRows;

use Text::CSV_XS;
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'file'}) ||
    die "CsvUtils::Process::MergeByRows: not defined 'file' argument";

  my $file = $params{'file'};
  open my $file_descr, $file or die "Can't open $file";
  my $fields = {
    file_ => $file_descr,
    csv_ => Text::CSV_XS->new({ binary => 1, eol => undef })
    };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  # read file row
  my $line = readline($self->{file_});
  if(defined($line))
  {
    chomp $line;
    $self->{csv_}->parse($line);
    my @arr = $self->{csv_}->fields();
    return [@$row, @arr];
  }

  return $row;
}

sub flush
{}

1;
