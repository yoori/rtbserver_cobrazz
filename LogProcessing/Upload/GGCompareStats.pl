use strict;
use warnings;
use Text::CSV_XS;
use open qw( :std :encoding(UTF-8) );

my $old_file = $ARGV[0];
my $new_file = $ARGV[1];

use constant KEY_FIELDS => 7;

# load old file
my $csv = Text::CSV_XS->new({
  binary => 1,
  eol => undef,
  sep_char => "," });

open(my $old_fh, '<', $old_file) or die "Could not open '$old_file' $!\n";

my %old = ();

while (my $line = <$old_fh>)
{
  chomp $line;
  if ($csv->parse($line))
  {
    my @fields = $csv->fields();
    my $key = $fields[0];
    for(my $i = 0; $i < KEY_FIELDS; ++$i)
    {
      $key = $key . "," . $fields[$i];
    }
    $old{$key} = \@fields;
  }
}

# load new file
open(my $new_fh, '<', $new_file) or die "Could not open '$new_file' $!\n";

my %new = ();

while (my $line = <$new_fh>)
{
  chomp $line;
  if ($csv->parse($line))
  {
    my @fields = $csv->fields();
    my $key = $fields[0];
    for(my $i = 0; $i < KEY_FIELDS; ++$i)
    {
      $key = $key . "," . $fields[$i];
    }
    $new{$key} = \@fields;
  }
}

binmode(STDOUT, "encoding(UTF-8)");

my $out_csv = Text::CSV_XS->new({
  binary => 1,
  eol => $/,
  sep_char => ",",
  quote_binary => 1,
  quote_space => 0,
  });

for my $new_key(keys %new)
{
  if(!exists $old{$new_key})
  {
    my $res_row = $new{$new_key};
    $out_csv->print(\*STDOUT, $res_row);
  }
  else
  {
    my $res_row = $new{$new_key};
    my $old_row = $old{$new_key};
    my $not_zero = undef;
    for(my $i = KEY_FIELDS; $i < scalar(@$old_row); ++$i)
    {
      $res_row->[$i] -= $old_row->[$i];
      if($res_row->[$i] != 0)
      {
        $not_zero = 1;
      }
    }

    if($not_zero)
    {
      $out_csv->print(\*STDOUT, $res_row);
    }
  }
}
