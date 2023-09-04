package CsvUtils::Process::TextSVM;

use Digest::CRC qw(crc32);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::TextSVM: not defined 'field' argument";

  my $fields = { field_ => $params{'field'} - 1 };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];

  if(ref($value) eq 'ARRAY')
  {
    my @res_arr;
    foreach my $sub_val(@$value)
    {
      push(@res_arr, svm_($sub_val));
    }
    $row->[$self->{field_}] = \@res_arr;
  }
  else
  {
    $row->[$self->{field_}] = svm_($value);
  }

  return $row;
}

sub flush
{}

sub svm_
{
  my ($text) = @_;
  my @words = split(' ', $text);
  my $res = '';
  foreach my $word(@words)
  {
    $res = $res . (length($res) > 0 ? ' ' : '') . crc32($word) . ':1';
  }
  return $res;
}

1;
