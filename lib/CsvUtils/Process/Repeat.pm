package CsvUtils::Process::Repeat;

use Storable qw(dclone);
use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Repeat: not defined 'field' argument";

  my $index = $params{'field'};
  if(!looks_like_number($index))
  {
    die "CsvUtils::Process::Repeat: incorrect column index: $index";
  }

  my $fields = {
    field_ => $index - 1,
    };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $repeat = $row->[$self->{field_}];
  if(!looks_like_number($repeat))
  {
    die "CsvUtils::Process::Repeat: incorrect repeat number: $repeat";
  }

  my @res_rows;
  for(my $i = 0; $i < $repeat; ++$i)
  {
    push(@res_rows, dclone($row));
  }

  return @res_rows;
}

sub flush
{}

1;
