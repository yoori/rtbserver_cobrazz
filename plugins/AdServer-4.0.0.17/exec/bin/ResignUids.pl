#!/usr/bin/perl -w
# This script extract UID's from SignedUids.pm, resign its with custom private key
# and renew the SignedUids.pm
# Usage: ResignUids.pl <Path to store result> <path to file with old uids, that should be resigned>
use strict;

use MIME::Base64;
use IPC::Open3;

sub generate_sign
{
  my $uid = shift;
  $uid = str_to_asn1(decode($uid));

  my ($fd_in, $fd_out, $fd_err) = (\*CMD_IN, \*CMD_OUT, \*CMD_ERR);
  my $stdout_buf = undef;
  my $stderr_buf = undef;

  local $| = 1;
  my $cmd_pid = eval
  {
    open3($fd_in, $fd_out, $fd_err,
      "openssl rsautl -sign -pkcs -inkey $ARGV[0]/private.der -keyform der -in /dev/stdin" );
  };
  print $fd_in $uid;
  close CMD_IN;
  waitpid($cmd_pid, 0);
  $stdout_buf .= do { local $/; <CMD_OUT> };
  $stderr_buf .= do { local $/; <CMD_ERR> };
  close CMD_OUT;
  close CMD_ERR;

  if ($stderr_buf ne '')
  {
    print STDERR "ERROR: $stderr_buf\n";
    return;
  }

  return encode($stdout_buf);
}

sub str_to_asn1
{
  my $str = shift;
  return chr(4) . chr(16) . $str;
}

sub encode
{
  my $sequence = join("", @_);
  my $encoded = encode_base64($sequence, "");

  $encoded =~ s/\+/-/g;
  $encoded =~ s/\//_/g;
  $encoded =~ s/=/./g;

  return $encoded;
}

sub decode
{
  my $sequence = join("", @_);
  $sequence =~ s/-/+/g;
  $sequence =~ s/_/\//g;
  $sequence =~ s/\./=/g;

  return decode_base64($sequence);
}

  my @uids;
  if (open FILE, "<$ARGV[1]")
  {
    while (<FILE>)
    {
      if (-/\s'(.+)',/)
      {
        my $uid = substr $1, 0, 22;
        push @uids, $uid . "..";
      }
    }
    close FILE;
  }
  else
  {
    print STDERR "Failed to open file: $!\n";
  }
  my @signed_uids;
  foreach my $got_uid (@uids)
  {
    my $sign = generate_sign($got_uid);
    if (!defined($sign))
    {
      exit;
    }
    $got_uid =~ s/\.//g;
    $sign =~ s/\.//g;
    push @signed_uids, "  '$got_uid$sign',\n";
  }
  if (open OUT_FILE, ">$ARGV[0]/SignedUids.pm")
  {
    print OUT_FILE "package signed_uids;\n";
    print OUT_FILE "use strict;\n";
    print OUT_FILE "use warnings;\n\n";
    print OUT_FILE 'our @monitoring_uids = (' . "\n";
    foreach my $signed_uid (@signed_uids)
    {
      print OUT_FILE $signed_uid;
    }
    print OUT_FILE "  );\n\n1;\n";
    close OUT_FILE;
  }
  else
  {
    print STDERR "Failed to open file: $!\n";
  }

1;
