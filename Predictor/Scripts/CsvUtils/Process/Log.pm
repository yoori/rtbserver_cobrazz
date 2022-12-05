package CsvUtils::Process::Log;

use Scalar::Util;
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
      push(@res_arr, get_log_($sub_val));
    }
    $row->[$self->{field_}] = \@res_arr;
  }
  else
  {
    $row->[$self->{field_}] = get_log_($value);
  }

  return $row;
}

sub get_log_
{
  my ($val) = @_;
  if(Scalar::Util::looks_like_number($val))
  {
    return $val > 0 ? int(log($val) / log(2)) : 0;
  }
  return $val;
}

sub flush
{}

1;
