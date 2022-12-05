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
# File   : cxxdep.pl
# Author : Boris Kolpackov <boris@kolpackov.net>

use strict;

use File::Basename;
use Fcntl;

my $debug_mode = 0;

my $usage = "usage: cxxdep.pl [--fstype <fs-type>] [--strict] " .
            "[-I<path>]* [-D<define>]* <destination> <source>\n";

my %search_path_seen = ();
my @search_paths = ();
my $source_file = "";
my $destination_file = "";


my $need_include_path_translation = 0;
my $strict_mode = 0;

# STEP 1: parse command line

my $key_stanza = "";

for my $arg (@ARGV)
{
  print (STDERR "<handling argument> : $arg\n") if $debug_mode;

  if ($key_stanza ne "")
  {
    SWITCH1:
    {
      if ($key_stanza =~ /^--fstype$/)
      {
        &fileparse_set_fstype ($arg);
        print (STDERR "<using fstype> : $arg\n") if $debug_mode;

        if ($arg =~ /mswin32/i && $^O  =~ /cygwin/i)
        {
          # We are running cygwin and was asked to
          # use win32 path format. It's very likely
          # that path translation is needed as well
          # for cases like this:
          #
          #  #include "include/Hello.h"
          #
          $need_include_path_translation = 1;
        }

        last SWITCH1;
      }

      die $usage;
    }
  }

  if ($key_stanza ne "")
  {
    $key_stanza = "";
    next;
  }

  SWITCH:
  {
    if ($arg =~ /^-I(.+)/)
    {
      push (@search_paths, $1) unless $search_path_seen{$1}++;
      last SWITCH;
    }
    if ($arg =~ /^-D(.+)/)
    {
      last SWITCH;
    }
    if ($arg =~ /^--fstype$/)
    {
      $key_stanza = $arg;
      last SWITCH;
    }
    if ($arg =~ /^--strict$/)
    {
      $strict_mode = 1;
      last SWITCH;
    }
    if ($arg =~ /^-.+/) # skip anything that starts from -
    {
      last SWITCH;
    }

    # default

    if ($destination_file eq "")
    {
      $destination_file = $arg;
    }
    else
    {
      die $usage unless $source_file eq "";
      $source_file = $arg;
    }
  }
}

die $usage unless $destination_file ne "";


# STEP 2: detecting path separator.
my $dir_sep = &detect_directory_separator();
print (STDERR "<dir sep>      : $dir_sep\n") if $debug_mode;

# STEP 3: normilize search paths
for my $path (@search_paths)
{
  if ($path !~ /$dir_sep$/)
  {
    $path .= $dir_sep
  }
}

# STEP 4: parse our source file path
my ($source_name, $source_path, $source_ext) =
  &fileparse($source_file, '\..*');

$source_path = &inc_dirname($source_file);

print (STDERR "<search paths> : \n") if $debug_mode;

for my $path (@search_paths)
{
  print (STDERR "\t$path\n") if $debug_mode;
}

print (STDERR "<source>       : $source_file\n") if $debug_mode;
print (STDERR "<source path>  : $source_path\n") if $debug_mode;
print (STDERR "<source name>  : $source_name\n") if $debug_mode;
print (STDERR "<source ext>   : $source_ext\n") if $debug_mode;


sysopen (SRC, $source_file, O_RDONLY) ||
  die "Unable to open $source_file: $!\n";

my %prereq_hash = ();
$prereq_hash{$source_file}++;

my @prereq_list = ($source_file);

print (STDERR "Parsing source file '$source_file'\n") if $debug_mode;
&gen_dep(*SRC, $source_path);
print (STDERR "Finished parsing source file '$source_file'\n") if $debug_mode;

close (SRC) || die "unable to close $source_file: $!\n";

sysopen (DST, $destination_file, O_CREAT | O_TRUNC | O_WRONLY, 0666) ||
  die "unable to create $destination_file: $!\n";

print (DST "$destination_file :");

for my $file (@prereq_list)
{
  print (DST " \\\n $file");
}

print (DST "\n");

for my $file (@prereq_list)
{
  print (DST "$file:\n\n");
}

close (DST) || die "unable to close $destination_file: $!\n";

sub gen_dep
{
  my ($fh, $path) = @_;

  # parsing file

  my %inc_signatures = ();
  my @inc_paths = ();

  my $line;
  OUTER: while ($line = <$fh>)
  {
    chomp $line;
    if ($line =~ s/\\$//)
    {
      my $add = <$fh>;
      last unless defined $add;
      $line .= $add;
      redo;
    }

    my $include_quoted = $line =~ /^\s*\#\s*include\s*\"(.+)\"\s*/;

    if ($include_quoted || $line =~ /^\s*\#\s*include\s*<(.+)>\s*/)
    {
      print (STDERR "<match> : $1\n") if $debug_mode;

      my $match = $1;

      if ($need_include_path_translation)
      {
        $match =~ s/\//$dir_sep/g;
      }

      unless ($inc_signatures{$match}++)
      {
        my $inc_path = $path . $match;
        my $succ_flag = 0;

        INNER: while (1)
        {
          # try to form the path

          if ($include_quoted)
          {
            print (STDERR "<try1> : $path,  $match ($inc_path)\n") if $debug_mode;

            if (stat ($inc_path))
            {
              print (STDERR "<found1> : $path,  $match\n") if $debug_mode;
              last INNER;
            }
          }

          for my $p (@search_paths)
          {
            $inc_path = $p . $match;

            print (STDERR "<try2> : $p,  $match ($inc_path)\n") if $debug_mode;

            if (stat ($inc_path))
            {
              print (STDERR "<found2> : $p,  $match\n") if $debug_mode;
              last INNER;
            }
          }

          if ($strict_mode)
          {
            die "No include path in which to find $match\n";
          }
          else
          {
            next OUTER;
          }
        }

        print (STDERR "<path> : $inc_path\n") if $debug_mode;

        if (!$prereq_hash{$inc_path})
        {
          push (@inc_paths, $inc_path);
        }
      }
    }
  }

  # Now process recursively what we've got
  for my $key (@inc_paths)
  {
    $prereq_hash{$key} = 1;
    push (@prereq_list, $key);
    my $next_fh;
    print (STDERR "Parsing file '$key'\n") if $debug_mode;
    sysopen (TEMP, $key, O_RDONLY) ||
      die "Unable to open $key: $!\n";

    $next_fh = *TEMP;

    my ($key_name, $key_path) = &fileparse ($key);

    &gen_dep($next_fh, $key_path);
    close ($next_fh);
    print (STDERR "Finished parsing file '$key'\n") if $debug_mode;
  }
}

sub detect_directory_separator($$)
{
  my ($name, $path) = &fileparse("dummy");
  my $path_without_sep = &dirname("dummy");
  die "Unable to detect directory separator.\n"
    unless $path =~ /^$path_without_sep(.+)/;
  my $path_sep = $1;
  return $path_sep;
}

sub inc_dirname($)
{
  my ($path) = @_;
  my $dir = dirname ($path);
  my $result = "";

  if ($dir ne ".")
  {
    $dir .= $dir_sep;
  }
}
