package CsvUtils::Process::LE;

use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::LE: not defined 'field' argument";
  exists($params{'max'}) ||
    die "CsvUtils::Process::LE: not defined 'max' argument";

  if(!looks_like_number($params{'field'}))
  {
    die "CsvUtils::Process::Columns: incorrect column index: " . $params{'field'};
  }

  my $fields = {
    field_ => $params{'field'} - 1,
    max_ => $params{'max'},
    num_ => $params{'num'}
  };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  if(defined($self->{num_}) && $self->{num_} > 0)
  {
    if($row->[$self->{field_}] <= $self->{max_})
    {
      return $row;
    }
  }
  else
  {
    if($row->[$self->{field_}] le $self->{max_})
    {
      return $row;
    }
  }

  return undef;
}

sub flush
{}

1;
