#!/usr/bin/perl -w

use strict;
use warnings;
use Text::CSV_XS;

package main;

sub main
{
  my ($old_file_path, $new_file_path) = @_;

  open(my $old_file, '>>', $old_file_path) || die "Can't open file '$old_file_path'";
  open(my $new_file, '>>', $new_file_path) || die "Can't open file '$new_file_path'";

  my $csv = Text::CSV_XS->new({ binary => 1, eol => undef });

  my $old_row = $csv->getline($old_file);
  my $new_row = $csv->getline($new_file);

  while(1)
  {
    # fetch old
    my $rows = $csv->getline(*STDIN);
    my $user_id = defined($rows) ? $rows->[0] : undef;

    if(!defined($user_id) ||
      !defined($cur_user_id) ||
      $cur_user_id ne $user_id)
    {
      # dump prev user
      if(defined($cur_user_id))
      {
        my @res_row = ($cur_user_id, $cur_geo, $cur_data);
        $csv->print(\*STDOUT, \@res_row);
        print "\n";
        #print $cur_user_id . "," . $cur_geo . "," . $cur_data . "\n";
      }

      if(!defined($user_id))
      {
        last;
      }
    }
  }
}

main(@ARGV);
