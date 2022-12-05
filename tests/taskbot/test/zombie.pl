#! /usr/bin/perl
# -*- perl -*-
#

$pid = fork;
$timeout = @ARGV[0];
if ($pid) {
}
elsif (defined $pid) {
  $SIG{__DIE__} = 'DEFAULT';
  $SIG{__WARN__} = 'DEFAULT';
  $SIG{USR1} = 'DEFAULT';
  my $start = time; 
  print "" while time < $start + 2*$timeout;
}
sleep(2);
exit(-1);


