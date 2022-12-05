package CsvUtils::Process::ArraySortUniq;

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
    die "CsvUtils::Process::ArraySortUniq: not defined 'field' argument";

  my $fields = {};

  if($params{'field'} eq '*')
  {
    $fields->{field_} = undef;
  }
  else
  {
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
        die "CsvUtils::Process::Columns: incorrect column index: $index";
      }
    }

    $fields->{field_} = \@res_indexes;
  }

  if(defined($params{'skip_empty'}))
  {
    $fields->{skip_empty_} = 1;
  }

  return bless($fields, $class);
}

sub process
{
  my ($self, $row_param) = @_;

  my $row = [ @$row_param ];

  if(defined($self->{field_}))
  {
    foreach my $field_index(@{$self->{field_}})
    {
      my $value = $row->[$field_index];

      if(ref($value) eq 'ARRAY')
      {
        my @res_arr = uniq(sort(@$value));

        if(defined($self->{skip_empty_}))
        {
          @res_arr = grep { $_ ne '' } @res_arr;
        }

        if(scalar(@res_arr) == 1)
        {
          $row->[$field_index] = $res_arr[0];
        }
        elsif(scalar(@res_arr) == 0)
        {
          $row->[$field_index] = '';
        }
        else
        {
          $row->[$field_index] = \@res_arr;
        }
      }
    }
  }
  else
  {
    for(my $field_index = 0; $field_index < scalar(@$row); ++$field_index)
    {
      my $value = $row->[$field_index];

      if(ref($value) eq 'ARRAY')
      {
        my @res_arr = uniq(sort(@$value));

        if(defined($self->{skip_empty_}))
        {
          @res_arr = grep { $_ ne '' } @res_arr;
        }

        if(scalar(@res_arr) == 1)
        {
          $row->[$field_index] = $res_arr[0];
        }
        elsif(scalar(@res_arr) == 0)
        {
          $row->[$field_index] = '';
        }
        else
        {
          $row->[$field_index] = \@res_arr;
        }
      }
    }
  }

  return $row;
}

sub flush
{}

1;
