#!/usr/bin/perl -w

use strict;
use warnings;
use Text::CSV_XS;

package main;

sub main
{
  my $csv = Text::CSV_XS->new({ binary => 1, eol => undef });
  my $cur_user_id;
  my $cur_geo;
  my $cur_data;

  while(1)
  {
    # "External User ID",Timestamp,Geo,Action_with_utm_source
    my $rows = $csv->getline(*STDIN);
    my $user_id = defined($rows) ? $rows->[0] : undef;
    #print STDERR "<" . (defined($user_id) ? $user_id : '') . "," .
    #  (defined($cur_user_id) ? $cur_user_id : '') . ">\n";

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

      $cur_user_id = $user_id;
      $cur_geo = $rows->[2];
      $cur_data = '';

      if(!defined($user_id))
      {
        last;
      }
    }

    $cur_geo = $rows->[2];
    my $ts = $rows->[1];
    $ts =~ s/ /_/;
    $cur_data .= $ts . ":" . $rows->[3] . ";"
  }
}

main(@ARGV);
