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
# File   : msvc_fe.pl
# Author : Pavel Gubin <pgubin@ipmce.ru>
#
# Frontend for MSVC 6.0 build tools

sub translate_path ($);

$usage = "Usage: msvc_fe.pl [frontend options] <program> [program_options]\n" .
    "  where 'wrapper_options' are:\n" .
    "    --help\n" .
    "    --silent\n" .
    "    --dry-run\n";

my ($dry_run, $silent, $exit_val) = (0, 0, 0);

# Determine frontend arguments
die $usage unless (@ARGV > 0);
while ($ARGV[0] =~ /^-/) {
  if ($ARGV[0] eq "--silent") { $silent = 1; }
  elsif ($ARGV[0] eq "--dry-run") { $dry_run = 1; }
  elsif ($ARGV[0] eq "--help") { die $usage; }
  else {
    print "Unrecognized option: $ARGV[0]\n";
    die $usage;
  }
  shift(@ARGV);
}

# Determine program name
die $usage unless (@ARGV > 0);
if ($ARGV[0] =~ /^cl(\.exe)?$/) {

  #
  # cl
  #

  shift(@ARGV);
  my $compile_only = 0;
  foreach (@ARGV) {
    if ($_ eq "-c") {
      $compile_only = 1;
      last;
    }
  }

  my $common_args = "";
  my $link_args = "";
  my $pending_option = "common";
  for (my $i = 0; $i < @ARGV; $i++) {
    if ($pending_option eq "common") {
      if ($ARGV[$i] eq "-link") {
	$pending_option = "link";
      }
      elsif ($ARGV[$i] =~ /^-l/) {
	if (length($ARGV[$i]) < 3) {
	  die "Invalid use of -l option\n";
	}
	$link_args .= " " . substr($ARGV[$i], 2) . ".lib";
      }
      elsif ($ARGV[$i] =~ /^-L/) {
	$link_args .= " -LIBPATH:" . translate_path(substr($ARGV[$i], 2));
      }
      elsif ($ARGV[$i] =~ /^-I/) {
	$common_args .= " -I" . translate_path(substr($ARGV[$i], 2));
      }
      elsif ($ARGV[$i] eq "-o") {
	(@ARGV > $i) or die "Option -o must precede output filename\n";
	if (compile_only) {
	  $common_args .= " -Fo" . translate_path($ARGV[++$i]);
	}
	else {
	  $common_args .= " -Fe" . translate_path($ARGV[++$i]);
	}
      }
      elsif ($ARGV[$i] =~ /^-/) {
	# leave unknown option intact
	$common_args .= " " . $ARGV[$i];
      }
      elsif ($ARGV[$i] =~ m|.*/.*|) {
	# convert path
	$common_args .= " " . translate_path($ARGV[$i]);
      }
      else {
	$common_args .= " " . $ARGV[$i];
      }
    }
    elsif ($pending_option eq "link") {
      if ($ARGV[$i] =~ /^-/) {
	# leave unknown option intact
	$link_args .= " " . $ARGV[$i];
      }
      elsif ($ARGV[$i] =~ m|.*/.*|) {
	# convert path
	$link_args .= " " . translate_path($ARGV[$i]);
      }
      else {
	$link_args .= " " . $ARGV[$i];
      }
    }
  }

  if ($link_args) {
    $link_args = "-link " . $link_args;
  }

  system("echo cl $common_args $link_args") unless $silent;
  if (!$dry_run) {
    system("cl $common_args $link_args");
    $exit_val = $? >> 8;
  }
}
elsif ($ARGV[0] =~ /^link(\.exe)?$/) {

  #
  # link
  #

  shift(@ARGV);
  my $build_dll = 0;
  foreach (@ARGV) {
    if ($_ eq "-dll") {
      $build_dll = 1;
      last;
    }
  }

  my $out_file_name = "";
  my $i;
  for ($i = 0; $i < @ARGV; $i++) {
    if ($ARGV[$i] eq "-o") {
      ($i < @ARGV) or die "Option -o must precede output filename\n";
      $out_file_name = translate_path($ARGV[++$i]);
      last;
    }
    elsif ($ARGV[$i] =~ /\.obj$/)
    {
      $out_file_name = translate_path($ARGV[$i]);
      $out_file_name =~ s/\.obj$/\.dll/;
      last;
    }
  }
  ($i < @ARGV) or die "Cannot determine output file name\n";

  my $def_file_name = "";
  if ($build_dll)
  {
    $def_file_name = $out_file_name;
    $def_file_name =~ s/dll$/def/;
    system("echo EXPORTS > $def_file_name") unless $dry_run;
  }

  my $common_args = "";
  for ($i = 0; $i < @ARGV; $i++) {
    if ($ARGV[$i] eq "-o") {
      $i++;
      $common_args .= " -OUT:$out_file_name";
      if ($build_dll) {
	$common_args .= " -DEF:$def_file_name";
      }
    }
    elsif ($ARGV[$i] =~ /^-l/) {
      if (length($ARGV[$i]) < 3) { die "Invalid use of -l option\n"; }
      $common_args .= " " . substr($ARGV[$i], 2) . ".lib";
    }
    elsif ($ARGV[$i] =~ /^-L/) {
      $common_args .= " -LIBPATH:" . translate_path(substr($ARGV[$i], 2));
    }
    else {
      my $arg = $ARGV[$i];
      if ($arg =~ m|.*/.*|) {
	# convert path
	$arg = translate_path($arg);
      }

      if (($arg =~ /\.obj$/) && $build_dll) {
	system("dump_obj $arg >> $def_file_name") unless $dry_run;
      }

      $common_args .= " " . $arg;
    }
  }

  system("echo link $common_args") unless $silent;
  if (!$dry_run) {
    system("link $common_args");
    $exit_val = $? >> 8;
  }

  if ($build_dll) {
    system("rm $def_file_name") unless $dry_run;
  }
}
elsif ($ARGV[0] =~ /^lib(\.exe)?$/) {

  #
  # lib
  #

  shift(@ARGV);

  my $common_args = "";
  for ($i = 0; $i < @ARGV; $i++) {
    if ($ARGV[$i] eq "c") {
      ($i < @ARGV) or die "Option c must precede output filename\n";
      $common_args .= "-OUT:" . translate_path($ARGV[++$i]);
    }
    elsif ($ARGV[$i] =~ /^-/) {
      # leave unknown option intact
      $common_args .= " " . $ARGV[$i];
    }
    elsif ($ARGV[$i] =~ m|.*/.*|) {
      # convert path
      $common_args .= " " . translate_path($ARGV[$i]);
    }
    else {
      $common_args .= " " . $ARGV[$i];
    }
  }

  system("echo lib $common_args") unless $silent;
  if (!$dry_run) {
    system("lib $common_args");
    $exit_val = $? >> 8;
  }
}
else {
  print "Cannot determine execution mode\n";
  die $usage;
}

exit $exit_val;

#
# Subroutines
#

sub translate_path ($)
{
  my ($path_to_translate) = @_;
  my $translated = `cygpath -w $path_to_translate`;
  $translated =~ s/\n$//;
  $translated =~ s/\\/\\\\/g;
  return $translated;
}
