#!/usr/bin/perl -w

my $line_i = 0;
#my $skipped_lines = 0;

while(<STDIN>)
{
  my $line = $_;
  chomp $line;
  $line =~ s/\s+/ /g;

  my @arr = split(' ', $line);
  my $label = shift(@arr);
  if($label !~ m/^-?\d+(.\d+)?$/)
  {
    die "invalid label at line #" . $line_i . ": $line";   
  }

  my %features;
  foreach my $el(@arr)
  {
    if($el !~ m/^([0-9]+):([-.0-9]+)$/)
    {
      last;
      #die "can't parse '$el' from line #" . $line_i . ": $line";
    }

    my $key = $1;
    my $val = $2;
    
    if($key <= 2000000000) # if($key <= 4294967295)
    {
      $features{$key} = $val;
    }
    else
    {
      print STDERR "skipped value: $key\n";
    }
  }

  my $res_line = $label;
  foreach my $key(sort {$a <=> $b} keys %features)
  {
    $res_line = $res_line . ' ' . $key . ':' . $features{$key};
  }

  print $res_line . "\n";
  ++$line_i;
}
