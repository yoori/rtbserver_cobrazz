package CsvUtils::Process::CompanyJoin;

use Text::CSV_XS;
use Encode;
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'file'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'file' argument";

  exists($params{'source_name_field'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'source_name_field' argument";
  exists($params{'source_short_name_field'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'source_short_name_field' argument";
  exists($params{'source_inn_field'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'source_inn_field' argument";
  exists($params{'source_ogrn_field'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'source_ogrn_field' argument";

  exists($params{'join_name_field'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'join_name_field' argument";
  exists($params{'join_short_name_field'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'join_short_name_field' argument";
  exists($params{'join_inn_field'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'join_inn_field' argument";
  exists($params{'join_ogrn_field'}) ||
    die "CsvUtils::Process::CompanyJoin: not defined 'join_ogrn_field' argument";

  # parse file
  my %values;
  my $add_empty_size = 0;

  {
    my $file = $params{'file'};
    my $csv = Text::CSV_XS->new({ binary => 1, eol => $/ });
    open FILE, $file or die "Can't open $file";
    while(<FILE>)
    {
      my $line = $_;
      Encode::_utf8_on($line);
      chomp $line;
      $csv->parse($line);
      my @arr = $csv->fields();
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
    close FILE;
  }

  my $fields = {
    values_ => \%values,
    source_name_field_ => $params{'source_name_field'},
    source_short_name_field_ => $params{'source_short_name_field'},
    source_inn_field_ => $params{'source_inn_field'},
    source_ogrn_field_ => $params{'source_ogrn_field'},
    add_empty_size_ => $add_empty_size
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $company_name = ;
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
