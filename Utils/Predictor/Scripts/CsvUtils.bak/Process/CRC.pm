package CsvUtils::Process::CRC;

use Digest::CRC qw(crc32);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::InFile: not defined 'field' argument";

  my $fields = { field_ => $params{'field'} - 1 };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];

  if(ref($value) eq 'ARRAY')
  {
    my @res_arr;
    foreach my $sub_val(@$value)
    {
      push(@res_arr, crc32($sub_val));
    }
    $row->[$self->{field_}] = \@res_arr;
  }
  else
  {
    $row->[$self->{field_}] = crc32($value);
  }

  return $row;
}

sub flush
{}

1;
