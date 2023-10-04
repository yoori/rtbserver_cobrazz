package CsvUtils::Process::Domain;

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
      push(@res_arr, get_domain_($sub_val));
    }
    $row->[$self->{field_}] = \@res_arr;
  }
  else
  {
    $row->[$self->{field_}] = get_domain_($value);
  }

  return $row;
}

sub get_domain_
{
  my ($url) = @_;
  if($url =~ m/(?:http:\/\/|https:\/\/)?(?:www\.)?([^\/]+)/)
  {
    return $1;
  }
  return $url;
}

sub flush
{}

1;
