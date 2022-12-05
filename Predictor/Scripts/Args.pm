package Args;

use strict;

sub parse
{
  my @args = @_;
  my %res;
  foreach my $arg(@args)
  {
    while($arg =~ m/--([^=]*)='([^']*)'/g)
    {
      my $key = $1;
      if(!exists($res{$key}))
      {
        $res{$key} = $2;
      }
      else
      {
        $res{$key} = $res{$key} . "," . $2;
      }
    }

    while($arg =~ m/--([^=]*)=([^ ']*)/g)
    {
      my $key = $1;
      if(!exists($res{$1}))
      {
        $res{$key} = $2;
      }
      else
      {
        $res{$key} = $res{$key} . "," . $2;
      }
    }

    while($arg =~ m/--([^=]*)/g)
    {
      if(!exists($res{$1}))
      {
        $res{$1} = undef;
      }
    }
  }

  return \%res;
}

1;
