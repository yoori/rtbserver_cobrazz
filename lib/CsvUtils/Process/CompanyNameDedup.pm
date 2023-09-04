package CsvUtils::Process::CompanyNameDedup;

use Encode;
use List::MoreUtils qw/uniq/;
use Scalar::Util qw(looks_like_number);
use CsvUtils::Utils;
use utf8;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'field'}) ||
    die "CsvUtils::Process::CompanyNameDedup: not defined 'field' argument";

  my $fields = {};

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
        die "CsvUtils::Process::CompanyNameDedup: incorrect column index: $index";
      }
    }

    $fields->{field_} = \@res_indexes;
  }

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
      $res_row[$field_index] = norm_array_($value);
    }
  }
  else
  {
    for(my $index = 0; $index < scalar(@res_row); ++$index)
    {
      my $value = $res_row[$index];
      $res_row[$index] = norm_array_($value);
    }
  }


  return \@res_row;
}

sub norm_array_
{
  my ($arr) = @_;

  if(ref($arr) eq 'ARRAY')
  {
    my @res_arr;

    for(my $index = 0; $index < scalar(@$arr); ++$index)
    {
      my $skip = 0;

      for(my $sub_index = 0; $sub_index < scalar(@$arr); ++$sub_index)
      {
        if($index != $sub_index)
        {
          my @left_words = split(/\s+/, $arr->[$index]);
          my @right_words = split(/\s+/, $arr->[$sub_index]);

          my $contains = array_contains_(\@right_words, \@left_words);
          #print "CHECK " . $arr->[$index] . " IN " . $arr->[$sub_index] . " : " . $contains . "\n";
          if($contains != 0)
          {
            my $oth_contains = array_contains_(\@left_words, \@right_words);
            if($oth_contains == 0 || $index < $sub_index)
            {
              $skip = 1;
              last;
            }
          }
        }
      }

      if($skip == 0)
      {
        push(@res_arr, $arr->[$index]);
      }
    }

    return \@res_arr;
  }

  return $arr;
}

sub array_contains_
{
  my ($left_arr, $right_arr) = @_;

  my %right_hash = map { $_ => 1 } @$right_arr;

  foreach my $el(@$left_arr)
  {
    if(!exists($right_hash{$el}))
    {
      return 0;
    }
  }

  return 1;
}

sub flush
{}

1;
