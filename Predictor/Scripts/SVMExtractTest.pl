#!/usr/bin/perl -w

srand();

my $train_file = $ARGV[0];
my $test_file = $ARGV[1];

open(my $res_train, '>', $train_file) or die "Can't open '$train_file': $!";
open(my $res_test, '>', $test_file) or die "Can't open '$test_file': $!";

while(<STDIN>)
{
  my $line = $_;
  chomp $line;
  my $r = rand(100);
  if($r < 10)
  {
    print $res_test ($line . "\n");
  }
  else
  {
    print $res_train ($line . "\n");
  }
}

close $res_test;
close $res_train;
