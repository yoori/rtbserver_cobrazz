package CsvUtils::Process::LowCase;

use Encode;
use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::RETransform: not defined 'field' argument";

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
        die "CsvUtils::Process::LowCase: incorrect column index: $index";
      }
    }

    $fields->{field_} = \@res_indexes;
  }

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @res_row = @$row;

  if(defined($self->{field_}))
  {
    foreach my $field_index(@{$self->{field_}})
    {
      my $value = $res_row[$field_index];
      my $res_value = lc($value);
      $res_row[$field_index] = $res_value;
    }
  }
  else
  {
    for(my $index = 0; $index < scalar(@res_row); ++$index)
    {
      my $value = $res_row[$index];
      my $res_value = lc($value);
      $res_row[$index] = $res_value;
    }
  }


  return \@res_row;
}

sub flush
{}

1;
