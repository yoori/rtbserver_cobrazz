package CsvUtils::Process::CompanyNameNorm;

use Encode;
use List::MoreUtils qw/uniq/;
use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;
use CsvUtils::CompanyUtils;
use utf8;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::CompanyNameNorm: not defined 'field' argument";

  my $fields = {};

  my @indexes = split(',', $params{'field'});
  my %res_indexes;
  foreach my $index(@indexes)
  {
    if(looks_like_number($index))
    {
      $res_indexes{$index - 1} = 1;
    }
    else
    {
      die "CsvUtils::Process::LowCase: incorrect column index: $index";
    }
  }

  $fields->{field_} = \%res_indexes;

  if(exists($params{'split'}) && $params{'split'} > 0)
  {
    $fields->{split_} = 1;
  }

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @res_row;
  my @ntypes;

  for(my $field_index = 0; $field_index < scalar(@$row); ++$field_index)
  {
    if(exists($self->{field_}->{$field_index}))
    {
      my $value = $row->[$field_index];
      my ($ntype, $nname) = norm_name_($value);
      if(defined($self->{split_}))
      {
        push(@ntypes, $ntype);
        push(@res_row, $nname);
      }
      else
      {
        push(@res_row, ($ntype ne '' ? ($ntype . ' ' . $nname) : $nname));
      }

    }
    else
    {
      push(@res_row, $row->[$field_index]);
    }
  }

  push(@res_row, @ntypes);

  return \@res_row;
}

sub norm_name_
{
  my ($name) = @_;
  return CsvUtils::CompanyUtils::normalize_company_name($name);
}

sub flush
{}

1;
