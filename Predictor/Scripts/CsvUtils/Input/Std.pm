package CsvUtils::Input::Std;

use utf8;
use Text::CSV_XS;

sub new
{
  my $class = shift;
  my %params = @_;

  my $fields = {
    csv_ => Text::CSV_XS->new({
      binary => 1,
      eol => undef,
      sep_char => (exists($params{'sep'}) ? $params{'sep'} : ",") }),
    ignore_errors_ => $params{'ignore_errors'}
  };

  binmode(STDIN, ":utf8");

  return bless($fields, $class);
}

sub get
{
  my ($self) = @_;
  my $ret = $self->{csv_}->getline(*STDIN);

  if(!defined($ret))
  {
    my @error = $self->{csv_}->error_diag();

    if($error[0] != 2012 && (!defined($self->{ignore_errors_}) || $self->{ignore_errors_} == 0))
    {
      die "Input::Std: " . join(',', $self->{csv_}->error_diag());
    }
  }

  return $ret;

=for comment
  my $line = <STDIN>;

  if(defined($line))
  {
    chomp $line;
    #$line =~ s/\r$//;
    $self->{csv_}->parse($line);
    my @arr = $self->{csv_}->fields();
    return \@arr;
  }

  return undef;
=cut
}

1;
