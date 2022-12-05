package Common::ModifierExec;

use warnings;
use strict;

sub path_wrapper;

use constant PATH_WRAPPER_CLASS => 'PathWrapper';

sub path_wrapper
{
  my @args = @_;
  my @result;

  foreach my $arg(@args)
  {
    my $blessed_arg = { path => $arg };
    bless($blessed_arg, PATH_WRAPPER_CLASS);
    push(@result, $blessed_arg);
  }
  return @result;
}

1;
