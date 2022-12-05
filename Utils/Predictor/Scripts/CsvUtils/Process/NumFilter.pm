package CsvUtils::Process::NumFilter;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::NumFilter: not defined 'field' argument";

  my $fields = { field_ => $params{'field'} - 1 };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  if(Scalar::Util::looks_like_number($row->[$self->{field_}]))
  {
    return $row;
  }

  return undef;
}

sub flush
{}

1;
