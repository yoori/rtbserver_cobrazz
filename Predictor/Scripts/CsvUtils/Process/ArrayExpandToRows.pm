package CsvUtils::Process::ArrayExpandToRows;

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
    die "CsvUtils::Process::ArrayExpandToRows: not defined 'field' argument";

  return bless({field_ => $params{'field'} - 1}, $class);
}

sub process
{
  my ($self, $row_param) = @_;

  my $row = [ @$row_param ];
  my @res_rows;

  if(ref($row->[$self->{field_}]) eq 'ARRAY')
  {
    if(scalar(@{$row->[$self->{field_}]}) > 0)
    {
      foreach my $val(@{$row->[$self->{field_}]})
      {
        my $row_copy = [ @$row ];
        $row_copy->[$self->{field_}] = $val;
        push(@res_rows, $row_copy);
      }
    }
    else
    {
      my $row_copy = [ @$row ];
      $row_copy->[$self->{field_}] = '';
      push(@res_rows, $row_copy);
    }
  }
  else
  {
    push(@res_rows, $row);
  }

  return @res_rows;
}

sub flush
{}

1;
