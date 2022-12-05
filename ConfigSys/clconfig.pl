#!/usr/bin/perl  

use DACSUtils;

sub run
{
  my $opt;
  my $read_source = 0;
  my $read_dest = 0;
  my $read_sn = 0;
  
  foreach $opt(@ARGV)
  {
    if (substr($opt, 0, 1) eq '-')
    {
      if ($opt eq '-i')
      {
        $read_source = 1;
        $read_dest = 0;
        $read_sn = 0;
      }
      elsif ($opt eq '-o')
      {
        $read_source = 0;
        $read_dest = 1;
        $read_sn = 0;
      }
      elsif ($opt eq '-s')
      {
        $read_source = 0;
        $read_dest = 0;
        $read_sn = 1;
      }
      else
      {
        die "Incorrect option '$opt'.\n$usage";
      }
    }
    else
    {
      if ($read_source)
      {
        $source = $opt;
        $read_source = 0;
      }
      elsif ($read_dest)
      {
        $destination = $opt;
        $read_dest = 0;
      }
      elsif ($read_sn)
      {
        $sn = $opt;
        $read_sn = 0;
      }
      else
      {
        die "Unknown '$opt'.\n$usage";
      }
    }
  } 

  if(!defined($source) || !defined($destination) || !defined($sn))
  {
    die "Not defined source, dest file or service name.\n$usage";
  }

  my $locations = DACSUtils::load($source);
  
  my $conf_locations = 
    DACSUtils::generate_conf_locations(
      DACSUtils::get_hosts($locations),
      $sn);
  
  DACSUtils::save($conf_locations, $destination);
}

run();
