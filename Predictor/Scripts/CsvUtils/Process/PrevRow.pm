package CsvUtils::Process::PrevRow;

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
    if(!looks_like_number($index))
    {
      die "CsvUtils::Process::PrevRow: incorrect column index: $index";
    }
    push(@res_indexes, $index - 1);
  }

  if(defined($params{'key_field'}))
  {
    if(!looks_like_number($params{'key_field'}))
    {
      die "CsvUtils::Process::PrevRow: incorrect key_field index: " . $params{'key_field'};
    }
  }

  my $fields = {
    fields_ => \@res_indexes,
    key_field_ => (defined($params{'key_field'}) ? $params{'key_field'} - 1 : undef),
    default_ => $params{'default'},
    prev_row_ => undef
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  if(defined($self->{key_field_}))
  {
    my $cur_key = $row->[$self->{key_field_}];

    if(defined($self->{prev_row_}))
    {
      if($self->{prev_row_}->[$self->{key_field_}] ne $cur_key)
      {
        $self->{prev_row_} = undef;
      }
    }
  }

  my @res_row = @$row;

  foreach my $index(@{$self->{fields_}})
  {
    if(defined($self->{prev_row_}))
    {
      push(@res_row, $self->{prev_row_}->[$index]);
    }
    else
    {
      push(@res_row, $self->{default_});
    }
  }

  # save prev row
  my @row_copy = @$row;
  $self->{prev_row_} = \@row_copy;

  return \@res_row;
}

sub flush
{}

1;
