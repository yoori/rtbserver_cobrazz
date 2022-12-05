package CsvUtils::Process::ArrayMin;

use Encode;
use List::Util qw(min max);
use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::ArrayMin: not defined 'field' argument";

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

  $fields->{numeric_} = $params{'numeric'};

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

      if((ref($value) eq 'ARRAY') && scalar(@$value) > 0)
      {
        my @r;

        if(defined($self->{numeric_}) && $self->{numeric_} > 0)
        {
          @r = sort {$a <=> $b} (@$value);
        }
        else
        {
          @r = sort(@$value);
        }

        $row->[$field_index] = $r[0];
      }
    }
  }
  else
  {
    for(my $field_index = 0; $field_index < scalar(@$row); ++$field_index)
    {
      my $value = $row->[$field_index];

      if((ref($value) eq 'ARRAY') && scalar(@$value) > 0)
      {
        my @r;

        if(defined($self->{numeric_}) && $self->{numeric_} > 0)
        {
          @r = sort {$a <=> $b} (@$value);
        }
        else
        {
          @r = sort(@$value);
        }

        $row->[$field_index] = $r[0];
      }
    }
  }

  return $row;
}

sub flush
{}

1;
