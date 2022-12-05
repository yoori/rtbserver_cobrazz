package CsvUtils::Process::Columns;

use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::Columns: not defined 'field' argument";

  my @indexes = split(',', $params{'field'});
  my @res_indexes;
  foreach my $index(@indexes)
  {
    if($index =~ m/^[<](.*)[>]$/)
    {
      my $val = $1;
      Encode::_utf8_on($val);
      push(@res_indexes, [ $val ]);
    }
    elsif($index =~ m/^(\d+)-(\d+)$/)
    {
      my $first = $1;
      my $last = $2;
      for(my $i = $first; $i <= $last; ++$i)
      {
        push(@res_indexes, $i - 1);
      }
    }
    elsif(looks_like_number($index))
    {
      push(@res_indexes, $index - 1);
    }
    else
    {
      die "CsvUtils::Process::Columns: incorrect column index: $index";
    }
  }

  my $fields = { fields_ => \@res_indexes };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @res_row;
  foreach my $index(@{$self->{fields_}})
  {
    #print STDERR "index: $index\n";
    if(ref($index) eq 'ARRAY')
    {
      push(@res_row, $index->[0]);
    }
    else
    {
      push(@res_row, $row->[$index]);
    }
  }

  return \@res_row;
}

sub flush
{}

1;
