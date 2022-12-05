package CsvUtils::Process::ReplaceByFile;

use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'file'}) ||
    die "CsvUtils::Process::InFile: not defined 'file' argument";
  exists($params{'field'}) ||
    die "CsvUtils::Process::InFile: not defined 'field' argument";

  # parse file
  my %values;
  my $add_empty_size = 0;

  {
    my $file = $params{'file'};
    my $csv = Text::CSV->new({ binary => 1, eol => $/ });
    open FILE, $file or die "Can't open $file";
    while(<FILE>)
    {
      my $line = $_;
      chomp $line;
      $csv->parse($line);
      my @arr = $csv->fields();
      my @replace_arr = $arr[1 .. -1];
      $add_empty_size = (length(@replace_arr) > 0 ? length(@replace_arr) - 1 : 0);
      $values{$arr[0]} = \@replace_arr;
    }
    close FILE;
  }

  my $fields = {
    values_ => \%values,
    field_ => $params{'field'} - 1,
    add_empty_size_ => $add_empty_size
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];
  my $replace = $self->{values_};

  if(ref($value) eq 'ARRAY')
  {
    my @res_arr;
    foreach my $sub_val(@$value)
    {
      if(exists($replace->{$sub_val}))
      {
        push(@res_arr, @{$replace->{$sub_val}});
      }
      else
      {
        push(@res_arr, $sub_val);
        push(@res_arr, undef foreach (0 .. $self->{add_empty_size_}));
      }
    }
    $row->[$self->{field_}] = \@res_arr;
  }
  else
  {
    if(exists($replace->{$value}))
    {
      $row->[$self->{field_}] = $replace->{$value};
    }
  }

  return $row;
}

sub flush
{}

1;
