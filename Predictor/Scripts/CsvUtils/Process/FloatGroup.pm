package CsvUtils::Process::FloatGroup;

# provide intervals that div
use strict;
use warnings;
use Class::Struct;
use Scalar::Util qw(looks_like_number);

struct(Value => [labels => '$', total => '$', rate => '$', value => '$']);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Columns: not defined 'field' argument";

  my $fields = {
    values_ => {},
    field_ => $params{'field'} - 1,
    divs_ => $params{'divs'},
    values_ => {},
    total_ => 0
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];

  if(!exists($self->{values_}->{$value}))
  {
    $self->{values_}->{$value} = 1;
  }
  else
  {
    $self->{values_}->{$value} += 1;
  }

  $self->{total_} += 1;

  return undef;
}

sub flush
{
  my ($self) = @_;

  my @div_res;
  my @div_counts;

  my $prev_total = 0;
  my $cur_total = 0;
  my $cur_index = 1;
  my $start_value = undef;

  foreach my $key(sort {$a <=> $b} keys %{$self->{values_}})
  {
    my $value = $self->{values_}->{$key};
    $cur_total += $value;

    if(scalar(@div_res) eq 0)
    {
      push(@div_res, $key);
    }

    if($cur_total >= $cur_index * $self->{total_} / $self->{divs_})
    {
      push(@div_counts, $cur_total - $prev_total);
      push(@div_res, $key);

      ++$cur_index;
      $prev_total = $cur_total;
    }
  }

  my @res;
  for(my $index = 1; $index < scalar(@div_res); ++$index)
  {
    push(@res, [
      sprintf("%05.6f", $div_res[$index - 1]),
      sprintf("%05.6f", $div_res[$index]),
      $index
      ]);
  }

  return \@res;
}

1;
