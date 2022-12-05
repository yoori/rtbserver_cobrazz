package Args;

use strict;

sub parse
{
  my @args = @_; # CHANGE
  my %res;

  my $expect_spaced_val = undef;
  foreach my $arg(@args)
  {
    if($arg =~ m/^-[-]?([^-][^=]*)='([^']*)'$/)
    {
      if(!exists($res{$1}))
      {
        $res{$1} = $2;
      }
      $expect_spaced_val = undef;
    }
    elsif($arg =~ m/^-[-]?([^-][^=]*)=([^ ']*)$/)
    {
      if(!exists $res{$1})
      {
        $res{$1} = $2;
      }
      $expect_spaced_val = undef;
    }
    elsif($arg =~ m/^-[-]?([^-][^=]*)$/)
    {
      if(!exists $res{$1})
      {
        $res{$1} = undef;
      }

      $expect_spaced_val = $1;
    }
    elsif(defined $expect_spaced_val)
    {
      $res{$expect_spaced_val} = $arg;
      $expect_spaced_val = undef;
    }
  }

  return \%res;
}

1;
