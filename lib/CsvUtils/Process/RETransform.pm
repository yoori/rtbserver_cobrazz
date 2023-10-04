package CsvUtils::Process::RETransform;

use Encode;
use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::RETransform: not defined 'field' argument";

  my $fields = {};

  my @re_arr;
  my @sub_arr;

  foreach my $key(sort keys %params)
  {
    if($key =~ m/^re(.*)$/)
    {
      my $ind = $1;
      my $re = $params{$key};
      Encode::_utf8_on($re);
      binmode(STDOUT, ":utf8");
      #print "RE: <$re>\n";
      push(@re_arr, qr/$re/);

      if(exists($params{'s' . $ind}))
      {
        my $subs = $params{'s' . $ind};
        push(@sub_arr, $subs);
      }
      else
      {
        push(@sub_arr, '\1');
      }
    }
  }

  $fields->{re_} = \@re_arr;
  $fields->{sub_} = \@sub_arr;

  if($params{'field'} eq '*')
  {
    $fields->{field_} = undef;
  }
  else
  {
    my @indexes = split(',', $params{'field'});
    my @res_indexes;
    foreach my $index(@indexes)
    {
      if(looks_like_number($index))
      {
        push(@res_indexes, $index - 1);
      }
      else
      {
        die "CsvUtils::Process::Columns: incorrect column index: $index";
      }
    }

    $fields->{field_} = \@res_indexes;
  }

  #print scalar(@{$fields->{re_}}) . "\n";
  #print scalar(@{$fields->{sub_}}) . "\n";
  #print "\n";
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  my @res_row = @$row;

  if(defined($self->{field_}))
  {
    foreach my $field_index(@{$self->{field_}})
    {
      my $value = $res_row[$field_index];
      my $res_value = $value;

      for(my $index = 0; $index < scalar(@{$self->{re_}}); ++$index)
      {
        my $re = $self->{re_}->[$index];
        my $subs = 'do{"' . $self->{sub_}->[$index] . '"}';

        #print "SUBS: $subs\n";

        if($value =~ s/$re/$subs/gee)
        {
          $res_value = $value;
          last;
        }
      }

      $res_row[$field_index] = $res_value;
    }
  }
  else
  {
    for(my $index = 0; $index < scalar(@res_row); ++$index)
    {
      my $value = $res_row[$index];
      my $res_value = $value;

      for(my $re_index = 0; $re_index < scalar(@{$self->{re_}}); ++$re_index)
      {
        my $re = $self->{re_}->[$re_index];
        my $subs = 'do{"' . $self->{sub_}->[$re_index] . '"}';

        if($value =~ s/$re/$subs/gee)
        {
          $res_value = $value;
          last;
        }
      }

      $res_row[$index] = $res_value;
    }
  }


  return \@res_row;
}

sub flush
{}

1;
