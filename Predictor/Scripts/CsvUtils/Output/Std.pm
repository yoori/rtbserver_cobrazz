package CsvUtils::Output::Std;

use utf8;
use Text::CSV_XS;
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;

  my $fields = { csv_ => Text::CSV_XS->new({
    binary => 1,
    eol => (exists($params{'eol'}) ? $params{'eol'} : $/),
    sep_char => (exists($params{'sep'}) ? $params{'sep'} : ","),
    quote_binary => (exists($params{'quote_binary'}) ? $params{'quote_binary'} : 1),
    quote_space => (exists($params{'quote_space'}) ? $params{'quote_space'} : 1),
    })
  };

  binmode(STDOUT, ":utf8");

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;
  my $res_row = CsvUtils::Utils::prepare_row($row);
  $self->{csv_}->print(\*STDOUT, $res_row);
  return $row;
}

sub flush
{}

1;
