package CsvUtils::Process::Dispersion;

use Scalar::Util qw(looks_like_number);

sub new
{
  my $class = shift;
  my %params = @_;

  my $fields = {
    field_ => (exists($params{'field'}) ? $params{'field'} - 1 : 0),
    sum_ => 0.0,
    quad_sum_ => 0.0,
    count_ => 0
    };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my $value = $row->[$self->{field_}];

  $self->{count_} += 1;
  $self->{sum_} += $value;
  $self->{quad_sum_} += $value * $value;

  return [@$row, $avg];
}

sub flush
{
  my ($self) = @_;
  my $disp = 0;
  if($self->{count_} > 1)
  {
    $disp = ($self->{quad_sum_} - ($self->{sum_} / $self->{count_}) * ($self->{sum_} / $self->{count_})) / ($self->{count_} - 1);
  }
  print "Dispersion for field #" . $self->{field_} . ": " . sprintf("%.10f", $disp) . "\n";
}

1;
