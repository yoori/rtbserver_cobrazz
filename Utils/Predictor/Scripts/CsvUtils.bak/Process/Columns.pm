package CsvUtils::Process::Columns;

use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Columns: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::Columns: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  my $fields = { fields_ => \@res_indexes };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @res_row;
  foreach my $index(@{$self->{fields_}})
  {
    #print STDERR "index: $index\n";
    push(@res_row, $row->[$index]);
  }

  return \@res_row;
}

sub flush
{}

1;
