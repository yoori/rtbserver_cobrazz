package CsvUtils::Process::GE;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::NumFilter: not defined 'field' argument";
  exists($params{'min'}) ||
    die "CsvUtils::Process::NumFilter: not defined 'min' argument";

  if(!looks_like_number($params{'field'}))
  {
    die "CsvUtils::Process::Columns: incorrect column index: " . $params{'field'};
  }

  my $fields = {
    field_ => $params{'field'} - 1,
    min_ => $params{'min'},
    numeric_ => $params{'numeric'}
  };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  if(defined($self->{numeric_}) && $self->{numeric_} > 0)
  {
    if($row->[$self->{field_}] >= $self->{min_})
    {
      return $row;
    }
  }
  else
  {
    if($row->[$self->{field_}] ge $self->{min_})
    {
      return $row;
    }
  }

  return undef;
}

sub flush
{}

1;
