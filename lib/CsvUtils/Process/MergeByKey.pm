package CsvUtils::Process::MergeByKey;

use strict;
use warnings;
use CsvUtils::Utils;

sub new
{
  my $class = shift;
  my %params = @_;
  exists($params{'file'}) ||
    die "CsvUtils::Process::MergeByKey: not defined 'file' argument";

  my $file = $params{'file'};
  open my $file_descr, $file or die "Can't open $file";
  my $fields = {
    file_ => $file_descr,
    csv_ => Text::CSV->new({ binary => 1, eol => $/ }),
    base_key_field_ => (exists($params{'field'}) ? $params{'field'} - 1 : 0),
    merge_key_field_ => (exists($params{'merge_field'}) ? $params{'merge_field'} - 1 : 0),
    merge_fields_ => 0,
    cur_key_ => undef,
    cur_row_ => undef,
    order_suffix_ => (exists($params{'order_suffix'}) ? $params{'order_suffix'} : '')
    };
  return bless($fields, $class);
}

sub process
{
  my ($self, $row) = @_;

  # read file row
  my $row_key = $row->[$self->{base_key_field_}] . $self->{order_suffix_};

  #print "ROW KEY: '$row_key'\n";

  while(!defined($self->{cur_key_}) || ($self->{cur_key_} lt $row_key))
  {
    my $line = readline($self->{file_});
    #print "READ RES '$line'\n";

    if(!defined($line))
    {
      $self->{cur_row_} = undef;
      $self->{cur_key_} = undef;
      last;
    }
    else
    {
      chomp $line;
      $self->{csv_}->parse($line);

      my @merge_row = $self->{csv_}->fields();
      $self->{cur_row_} = \@merge_row;
      $self->{cur_key_} = $merge_row[$self->{merge_key_field_}] . $self->{order_suffix_};
      $self->{merge_fields_} = scalar(@merge_row) - 1;
    }
  }

  #print "DEB: " . (defined($self->{merge_fields_}) ? $self->{merge_fields_} : "undef") . "\n";

  my @join_row;
  for(my $i = 0; $i < $self->{merge_fields_}; ++$i)
  {
    my @arr;
    push(@join_row, \@arr);
  }

  while(defined($self->{cur_key_}) && ($self->{cur_key_} eq $row_key))
  {
    my $target_i = 0;
    for(my $i = 0; $i < scalar(@{$self->{cur_row_}}); ++$i)
    {
      if($i != $self->{merge_key_field_})
      {
        push(@{$join_row[$target_i]}, $self->{cur_row_}->[$i]);
        ++$target_i;
      }
    }

    my $line = readline($self->{file_});

    #print "READ RES2 '$line'\n";
    if(!defined($line))
    {
      $self->{cur_row_} = undef;
      $self->{cur_key_} = undef;
    }
    else
    {
      chomp $line;
      $self->{csv_}->parse($line);

      my @merge_row = $self->{csv_}->fields();
      $self->{cur_row_} = \@merge_row;
      $self->{cur_key_} = $merge_row[$self->{merge_key_field_}]. $self->{order_suffix_};
    }
  }

  return [@$row, @join_row];
}

sub flush
{}

1;
