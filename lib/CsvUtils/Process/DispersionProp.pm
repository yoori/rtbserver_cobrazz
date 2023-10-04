package CsvUtils::Process::DispersionProp;

use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'val1'}) ||
    die "CsvUtils::Process::DispersionProp: not defined 'val1' argument";
  exists($params{'disp1'}) ||
    die "CsvUtils::Process::DispersionProp: not defined 'disp1' argument";
  exists($params{'val2'}) ||
    die "CsvUtils::Process::DispersionProp: not defined 'val2' argument";
  exists($params{'disp2'}) ||
    die "CsvUtils::Process::DispersionProp: not defined 'disp2' argument";

  my $fields = {
    val1_ => $params{'val1'} - 1,
    disp1_ => $params{'disp1'} - 1,
    val2_ => $params{'val2'} - 1,
    disp2_ => $params{'disp2'} - 1
    };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $res =
   ($row->[$self->{disp1_}] + $row->[$self->{disp2_}]) > 0.0000001 ?
     ($row->[$self->{val1_}] * $row->[$self->{disp2_}] +
      $row->[$self->{val2_}] * $row->[$self->{disp1_}]) /
     ($row->[$self->{disp1_}] + $row->[$self->{disp2_}]) :
   ($row->[$self->{val1_}] + $row->[$self->{val2_}]) / 2;

  return [@$row, $res];
}

sub flush
{}

1;
