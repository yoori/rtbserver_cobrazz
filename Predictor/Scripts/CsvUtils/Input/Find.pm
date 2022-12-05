package CsvUtils::Input::Find;

use File::Find;
use Path::Iterator::Rule;
use Text::CSV_XS;

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{'dir'}) ||
    die "CsvUtils::Input::Find: not defined 'dir' argument";
  exists($params{'mask'}) ||
    die "CsvUtils::Input::Find: not defined 'mask' argument";

  my @files = find_files($params{'dir'}, $params{'mask'});
  my $fields = {
    csv_ => Text::CSV_XS->new({ binary => 1, eol => $/ }),
    files_ => \@files,
    cur_file_ => undef
  };

  return bless($fields, $class);
}

sub find_files
{
  my ($dir, $mask) = @_;

  my @res_files;

  my $rule = Path::Iterator::Rule->new;
  $rule->name($mask);

  my $it = $rule->iter($dir);
  while(my $file = $it->())
  {
    push(@res_files, $file);
  }

  return @res_files;
}

sub get
{
  my ($self) = @_;

  while(1)
  {
    if(!defined($self->{cur_file_}))
    {
      my $file = shift @{$self->{files_}};
      if(defined($file))
      {
        open my $fh, $file;
        $self->{cur_file_} = $fh;
      }
      else
      {
        return undef;
      }
    }

    if(< $self->{cur_file_} >)
    {
      my $line = $_;
      chomp $line;
      $self->{csv_}->parse($line);
      my @arr = $self->fields();
      return \@arr;
    }

    $self->{cur_file_} = undef;
  }
}

1;
