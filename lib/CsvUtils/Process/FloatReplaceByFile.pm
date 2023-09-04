package CsvUtils::Process::FloatReplaceByFile;

use Class::Struct;
use Text::CSV_XS;
use CsvUtils::Utils;

struct(Rule => [min => '$', max => '$', replace => '$']);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'file'}) ||
    die "CsvUtils::Process::InFile: not defined 'file' argument";
  exists($params{'field'}) ||
    die "CsvUtils::Process::InFile: not defined 'field' argument";

  # parse file
  my @values;
  my $add_empty_size = 0;

  {
    my $file = $params{'file'};
    my $csv = Text::CSV_XS->new({ binary => 1, eol => $/ });
    open FILE, $file or die "Can't open $file";
    while(<FILE>)
    {
      my $line = $_;
      chomp $line;
      $csv->parse($line);
      my @arr = $csv->fields();
      push(@values, new Rule(
        min => ($arr[0] ne '' ? $arr[0] : ''),
        max => ($arr[1] ne '' ? $arr[1] + 0.00001 : ''),
        replace => $arr[2]));
    }
    close FILE;
  }

  my $fields = {
    values_ => \@values,
    field_ => $params{'field'} - 1,
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];

  if($value ne '')
  {
    my @res_values;

    for(my $i = 0; $i < scalar @{$self->{values_}}; ++$i)
    {
      if(($self->{values_}->[$i]->min() eq '' || $self->{values_}->[$i]->min() <= $value) && (
         $self->{values_}->[$i]->max() eq '' || $value < $self->{values_}->[$i]->max()))
      {
        push(@res_values, $self->{values_}->[$i]->replace());
      }
    }

    if(scalar(@res_values) == 1)
    {
      $row->[$self->{field_}] = $res_values[0];
    }
    elsif(scalar(@res_values) > 1)
    {
      $row->[$self->{field_}] = \@res_values;
    }
  }

  return $row;
}

sub flush
{}

1;
