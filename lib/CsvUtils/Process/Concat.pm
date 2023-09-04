package CsvUtils::Process::Concat;

use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Concat: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::Concat: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  my $fields = {
    fields_ => \@res_indexes,
    res_field_ => exists($params{'res_field'}) ? $params{'res_field'} - 1 : $res_indexes[0]
    };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @res_vals;
  foreach my $index(@{$self->{fields_}})
  {
    #print STDERR "index: $index\n";
    push(@res_vals, $row->[$index]);
  }

  $row->[$self->{res_field_}] = join('/', @res_vals);

  return $row;
}

sub flush
{}

1;
