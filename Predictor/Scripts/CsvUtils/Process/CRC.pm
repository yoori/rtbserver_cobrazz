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
  if(exists($params{'mod'}))
  {
    $fields->{'mod_'} = $params{'mod'};
  }

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
      push(@res_arr, $self->crc32_($sub_val));
    }
    $row->[$self->{field_}] = \@res_arr;
  }
  else
  {
    $row->[$self->{field_}] = $self->crc32_($value);
  }

  return $row;
}

sub crc32_
{
  my ($self, $val) = @_;
  return exists($self->{'mod_'}) ? crc32($val) % $self->{'mod_'} : crc32($val);
}

sub flush
{}

1;
