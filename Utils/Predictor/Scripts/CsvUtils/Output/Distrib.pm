package CsvUtils::Output::Distrib;

use Digest::CRC qw(crc32);
use Class::Struct;
use Hash::MultiKey;
use CsvUtils::Utils;

struct(Rule => [token => '$', column => '$', chunks => '$']);

sub new
{
  my $class = shift;
  my %params = @_;

  exists($params{file}) ||
    die "Can't init CsvUtils::Output::Distrib: undefined 'file' value";

  my $file_path_templ = $params{file};
  my %tokens;
  my @rules;

  while($file_path_templ =~ m|{(\d+)(\%(\d+))?}|g)
  {
    my $token = $&;
    my $column_index = $1;
    my $chunks = $2;
    if(!exists($tokens{$token}))
    {
      $tokens{$token} = 1;
      my $new_rule = new Rule(token => quotemeta $token, column => $column_index - 1, chunks => $chunks);
      push(@rules, $new_rule);
    }
  }

  my $single_rule = scalar(@rules) <= 1 ? 1 : undef;
  my %files;
  if(!defined($single_rule))
  {
    tie(%files, 'Hash::MultiKey');
  }

  my $fields = {
    csv_ => Text::CSV->new({ binary => 1, eol => $/ }),
    file_ => $params{file},
    files_ => \%files,
    rules_ => \@rules,
    single_rule_ => $single_rule
  };

  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  # eval hash
  my $key;

  if(defined($self->{single_rule_}))
  {
    if(scalar(@{$self->{rules_}}) > 0)
    {
      my $rule = $self->{rules_}->[0];
      if(defined($rule->chunks()))
      {
        my $crc = crc32($row->[$rule->column()]);
        $key = $crc % $rule->chunks();
      }
      else
      {
        $key = $row->[$rule->column()];
      }
    }
    else
    {
      $key = '';
    }
  }
  else
  {
    my @vals;
    foreach my $rule(@{$self->{rules_}})
    {
      if(defined($rule->chunks()))
      {
        my $crc = crc32($row->[$rule->column()]);
        push(@vals, $crc % $rule->chunks());
      }
      else
      {
        push(@vals, $row->[$rule->column()]);
      }
    }

    $key = \@vals;
  }

  if(!exists($self->{files_}->{$key}))
  {
    my $file_path = $self->{file_};
    my $rules = $self->{rules_};
    for my $i (0 .. (scalar @$rules - 1))
    {
      my $token = $rules->[$i]->token();
      my $value;
      if(defined($self->{single_rule_}))
      {
        $value = quotemeta $key;
      }
      else
      {
        $value = quotemeta $key->[$i];
      }
      $file_path =~ s/$token/$value/g;
    }

    open(my $fh, '>>', $file_path) || die "Can't open file '$file_path'";
    $self->{files_}->{$key} = $fh;
  }

  my $res_row = CsvUtils::Utils::prepare_row($row);

  $self->{csv_}->print($self->{files_}->{$key}, $res_row);

  # TODO: remove oldest used file descriptors if limit reached
  return $row;
}

sub flush
{
  my ($self) = @_;
  foreach my $key(keys(%{$self->{files_}}))
  {
    close($self->{files_}->{$key});
  }
}

1;
