package CsvUtils::Process::NumFilter;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::NumFilter: not defined 'field' argument";

  my $fields = {
    field_ => $params{'field'} - 1,
    min_ => (exists($params{'min'}) ? $params{'min'} : undef)
  };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;
  my $value = $row->[$self->{field_}];

  if(Scalar::Util::looks_like_number($value))
  {
    if(defined($self->{min_}) && ($value < $self->{min_}))
    {
      return undef;
    }

    return $row;
  }

  return undef;
}

sub flush
{}

1;
