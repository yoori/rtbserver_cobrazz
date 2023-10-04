package CsvUtils::Process::NE;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::NE: not defined 'field' argument";
  exists($params{'value'}) ||
    die "CsvUtils::Process::NE: not defined 'value' argument";

  if(!looks_like_number($params{'field'}))
  {
    die "CsvUtils::Process::Columns: incorrect column index: " . $params{'field'};
  }

  my $fields = {
    field_ => $params{'field'} - 1,
    value_ => $params{'value'}
  };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  if($row->[$self->{field_}] ne $self->{value_})
  {
    return $row;
  }

  return undef;
}

sub flush
{}

1;
