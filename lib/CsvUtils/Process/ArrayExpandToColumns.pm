package CsvUtils::Process::ArrayExpandToColumns;

use Text::CSV_XS;
use Encode;
use List::MoreUtils qw/uniq/;
use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'field'}) ||
    die "CsvUtils::Process::ArrayExpandToColumns: not defined 'field' argument";

  return bless({field_ => $params{'field'} - 1}, $class);
}

sub process
{
  my ($self, $row_param) = @_;

  my $row = [ @$row_param ];
  my @res_row;
  for(my $i = 0; $i < $self->{field_}; ++$i)
  {
    push(@res_row, $row->[$i]);
  }

  if(ref($row->[$self->{field_}]) eq 'ARRAY')
  {
    push(@res_row, @{$row->[$self->{field_}]});
  }
  else
  {
    push(@res_row, $row->[$self->{field_}]);
  }

  for(my $i = $self->{field_} + 1; $i < scalar(@$row); ++$i)
  {
    push(@res_row, $row->[$i]);
  }

  return \@res_row;
}

sub flush
{}

1;
