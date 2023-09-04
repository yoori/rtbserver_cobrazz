package CsvUtils::Process::Linear;

use Scalar::Util;
use CsvUtils::Utils;
use Scalar::Util qw(looks_like_number);
use List::Util qw(min max);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Linear: not defined 'field' argument";
  exists($params{'coef'}) ||
    die "CsvUtils::Process::Linear: not defined 'coef' argument";

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
      die "CsvUtils::Process::Linear: incorrect column index: $index";
    }
  }

  my @coef = split(',', $params{'coef'});
  my @res_coef;
  foreach my $coef_i(@coef)
  {
    if(looks_like_number($coef_i))
    {
      push(@res_coef, $coef_i);
    }
    else
    {
      die "CsvUtils::Process::Linear: incorrect coef: $coef_i";
    }
  }

  my $fields = {
    field_ => \@res_indexes,
    coef_ => \@res_coef,
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = 0;

  for(my $field_i = 0; $field_i < scalar(@{$self->{field_}}); ++$field_i)
  {
    my $local_value = $row->[$self->{field_}->[$field_i]];
    my $local_coef = $self->{coef_}->[$field_i];

    $value += $local_value * $local_coef;
  }

  return [ @$row, $value ];
}

sub flush
{}

1;
