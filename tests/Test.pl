my $timeout = shift(@ARGV);
my $pid = fork();
if ($pid)
{
  $SIG{ALRM} =
    sub
    {
      kill(9, $pid);
      print STDERR "\nKilled by timeout\n";
    };
  alarm($timeout * 60);
  wait;
  exit($?);
}
else
{
  sleep(1);
  exec(@ARGV);
  exit(-1);
}
