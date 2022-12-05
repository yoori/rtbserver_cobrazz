package CsvUtils::Process::ReplaceByFile;

use Text::CSV_XS;
use Encode;
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'file'}) ||
    die "CsvUtils::Process::ReplaceByFile: not defined 'file' argument";
  exists($params{'field'}) ||
    die "CsvUtils::Process::ReplaceByFile: not defined 'field' argument";

  # parse file
  my %values;
  my $add_empty_size = 0;

  {
    my $file = $params{'file'};
    my $csv = Text::CSV_XS->new({ binary => 1, eol => $/ });
    open(FILE, '<', $file) or die "Can't open $file";
    while(<FILE>)
    {
      my $line = $_;
      Encode::_utf8_on($line);
      chomp $line;
      $csv->parse($line);
      my @arr = $csv->fields();
      if(scalar(@arr) > 0)
      {
        my @replace_arr; # = $arr[1 .. -1];
        for(my $index = 1; $index < scalar(@arr); ++$index)
        {
          push(@replace_arr, $arr[$index]);
          Encode::_utf8_on($replace_arr[$index]);
        }
        $add_empty_size = (scalar(@replace_arr) > 0 ? scalar(@replace_arr) - 1 : 0);

        Encode::_utf8_on($arr[0]);
        #print "INS '" . $arr[0] . "' => " . (scalar(@replace_arr)) . ":" . join(',', @replace_arr) . "\n";
        $values{$arr[0]} = \@replace_arr;
      }
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

  if(!defined($value))
  {}
  elsif(ref($value) eq 'ARRAY')
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
      }
      #else
      #{
      #  push(@res_arr, $sub_val);
      #  foreach (0 .. $self->{add_empty_size_})
      #  {
      #    push(@res_arr, '');
      #  }
      #}
    }
    $row->[$self->{field_}] = \@res_arr;
  }
  else
  {
    #print "CHECK '$value'\n";
    if(exists($replace->{$value}))
    {
      if(scalar(@{$replace->{$value}}) == 1)
      {
        $row->[$self->{field_}] = $replace->{$value}->[0];
      }
      elsif(scalar(@{$replace->{$value}}) == 0)
      {
        $row->[$self->{field_}] = '';
      }
      else
      {
        $row->[$self->{field_}] = $replace->{$value};
      }
    }
  }

  return $row;
}

sub flush
{}

1;
