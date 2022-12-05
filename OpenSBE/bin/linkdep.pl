#!/usr/bin/perl -w
# Open Software Building Environment (OpenSBE, OSBE)
# Copyright (C) 2001-2002 Boris Kolpackov
# Copyright (C) 2001-2004 Pavel Gubin, Karen Arutyunov
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
#
# File   : linkdep.pl
# Author : Konstatntin Sadov <Konstantin.Sadov@gmail.com>

use strict;
use Fcntl;

my $usage = "usage: linkdep.pl <destination> <template> <source> " .
            "[-L<path>]* [-l<lib>]* [<library>]*\n";

die $usage if $#ARGV < 2;

my $destination = $ARGV[0];
my $template = $ARGV[1];
my $source = $ARGV[2];

my @paths;
my @libraries;
my @final;

foreach my $arg (@ARGV[3 .. $#ARGV])
{
  if (substr($arg, 0, 1) eq '-')
  {
    my $prefix = substr($arg, 1, 1);
    my $name = substr($arg, 2);
    if ($prefix eq 'L')
    {
      push @paths, $name;
    }
    elsif ($prefix eq 'l')
    {
      push @libraries, $name;
    }
  }
  else
  {
    push (@final, $arg);
  }
}

LIB: foreach my $lib (@libraries)
{
  my $libs = $template;
  $libs =~ s/%/$lib/g;
  my @libs = split(/\s+/, $libs);

  foreach my $dir (@paths)
  {
    foreach my $file (@libs)
    {
      my $final = $dir . "/" . $file;
      if (-f $final)
      {
        push @final, $final;
        next LIB;
      }
    }
  }
}

sysopen(DST, $destination, O_CREAT | O_TRUNC | O_WRONLY, 0666) ||
  die "unable to create $destination: $!\n";

if ($#final >= 0)
{
  print DST "$source : \\\n";
  foreach my $final (@final)
  {
    print DST "\t$final \\\n";
  }
  print DST "\n\n";

  foreach my $final (@final)
  {
    print DST "$final :\n\n";
  }
}
else
{
  print DST "$source :\n";
}
close(DST) || die "unable to close $destination: $!\n";
