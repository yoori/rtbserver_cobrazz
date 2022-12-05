package CsvUtils::Process::Mul;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Mul: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::Mul: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  my $fields = { fields_ => \@res_indexes };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $sum = 1;
  foreach my $index(@{$self->{fields_}})
  {
    $sum = $sum * $row->[$index];
  }

  return [@$row, $sum];
}

sub flush
{}

1;
