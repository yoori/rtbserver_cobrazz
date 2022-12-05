package CsvUtils::Process::Array;

use strict;
use warnings;
use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Array: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if(looks_like_number($index))
    {
      push(@res_indexes, $index - 1);
    }
    else
    {
      die "CsvUtils::Process::Array: incorrect column index: $index";
    }
  }

  my $fields = { field_ => \@res_indexes };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  foreach my $field_index(@{$self->{field_}})
  {
    my $value = $row->[$field_index];
    if(defined($value) && (ref($value) ne 'ARRAY'))
    {
      my @arr = split('\|', $value);
      @arr = grep { $_ ne '' } @arr;
      $row->[$field_index] = \@arr;
    }
  }

  return $row;
}

sub flush
{}

1;
