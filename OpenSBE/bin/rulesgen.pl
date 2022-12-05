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
# File   : rulesgen.pl
# Author : Pavel Gubin <pgubin@ipmce.ru>

use strict;
use File::Basename;

sub make_dir($);
sub make_includes($);

my $usage = "Usage: rulesgen.pl {-ar | -so | -inl | -fake}";

(@ARGV == 1) or die $usage;

my $ENV_CPP_FLAGS =
    exists($ENV{rg_cpp_flags}) ? $ENV{rg_cpp_flags} : "";
my $ENV_RULES_PATH =
    exists($ENV{rg_rules_path}) ? $ENV{rg_rules_path} : "";
my $ENV_EXPANDED_RULES_PATH =
    exists($ENV{rg_expanded_rules_path}) ? $ENV{rg_expanded_rules_path} : "";
my $ENV_RULES_DEP_FROM =
    exists($ENV{rg_rules_dep_from}) ? $ENV{rg_rules_dep_from} : "";
my $ENV_TARGET =
    exists($ENV{rg_target}) ? $ENV{rg_target} : "";
my $ENV_LIBNAME =
    exists($ENV{rg_libname}) ? $ENV{rg_libname} : $ENV_TARGET;
my $ENV_POST_CONFIG =
    exists($ENV{rg_post_config}) ? $ENV{rg_post_config} : "";
my $ENV_INCLUDE_GUARD =
    exists($ENV{rg_include_guard}) ? $ENV{rg_include_guard} : "";
my $ENV_PRECOMPILED_HEADER_EXPORT =
    exists($ENV{rg_precompiled_header_export}) ? $ENV{rg_precompiled_header_export} : "";
my $ENV_PRECOMPILED_HEADER =
    exists($ENV{rg_precompiled_header}) ? $ENV{rg_precompiled_header} : "";
my $ENV_PRECOMPILED_HEADER_FLAGS =
    exists($ENV{rg_precompiled_header_flags}) ? $ENV{rg_precompiled_header_flags} : "";

($ENV_EXPANDED_RULES_PATH eq "") and die "Variable ENV_EXPANDED_RULES_PATH undefined\n";

my $expanded_rules_dir = dirname($ENV_EXPANDED_RULES_PATH);
make_dir($expanded_rules_dir)
    or die "Cannot create $expanded_rules_dir\n";

my $rules_content;

if ($ARGV[0] eq "-fake")
{
  $rules_content=<<"EOF";
# \@file ${ENV_EXPANDED_RULES_PATH}
#
# !!! DO NOT EDIT !!!
# This file is generated automatically by RulesGenerator (TM).

ifndef OSBE_MAKE_CLEAN_GOAL
  \$(error File does not exist. Build corresponding library first)
endif

EOF

  open(RULES_FILE, ">$ENV_EXPANDED_RULES_PATH")
      or die "Cannot open $ENV_EXPANDED_RULES_PATH for write\n";

  print RULES_FILE $rules_content;
  close RULES_FILE;

  exit 0;
}

($ENV_RULES_PATH eq "") and die "Variable ENV_RULES_PATH undefined\n";
($ARGV[0] ne "-inl" && $ENV_TARGET eq "") and die "Variable ENV_TARGET undefined\n";
($ENV_INCLUDE_GUARD eq "") and die "Variable ENV_INCLUDE_GUARD undefined\n";

my $includes = make_includes($ENV_RULES_DEP_FROM);

if ($ARGV[0] eq "-so")
{
  $ENV_RULES_PATH .= ".so.pre.rules";
  $ENV_EXPANDED_RULES_PATH .= ".so.pre.rules";
  $ENV_INCLUDE_GUARD .= "_SO_PRE_RULES_";

  $rules_content=<<"EOF";
# \@file ${ENV_LIBNAME}.so.pre.rules
#
# !!! DO NOT EDIT !!!
# This file is generated automatically by RulesGenerator (TM).

ifndef ${ENV_INCLUDE_GUARD}
define ${ENV_INCLUDE_GUARD}
1
endef

ifeq (\$(origin rg_first_level), undefined)
  rg_first_level:=yes
endif

rg_ar_only := no \$(rg_ar_only)

ifeq (\$(rg_first_level), yes)

  rg_first_level:=no
  $includes
  rg_first_level:=yes

  rg_rules_dep_from += $ENV_RULES_PATH

else

  $includes

endif

rg_ar_only := \$(wordlist 2, 100000, \$(rg_ar_only))

CPP_FLAGS := $ENV_CPP_FLAGS \$(CPP_FLAGS)
LD_FLAGS  := -L\$(top_builddir)/lib \$(LD_FLAGS)
LIBS      := -l$ENV_TARGET \$(LIBS)

$ENV_POST_CONFIG

endif # ${ENV_INCLUDE_GUARD}

EOF

}
elsif ($ARGV[0] eq "-ar")
{
  $ENV_RULES_PATH .= ".ar.pre.rules";
  $ENV_EXPANDED_RULES_PATH .= ".ar.pre.rules";
  $ENV_INCLUDE_GUARD .= "_AR_PRE_RULES_";

  $rules_content=<<"EOF";
# \@file ${ENV_LIBNAME}.ar.pre.rules
#
# !!! DO NOT EDIT !!!
# This file is generated automatically by RulesGenerator (TM).

ifndef ${ENV_INCLUDE_GUARD}
define ${ENV_INCLUDE_GUARD}
1
endef

ifeq (\$(origin rg_first_level), undefined)
  rg_first_level:=yes
endif

ifeq (\$(rg_first_level), yes)

  rg_first_level:=no
  $includes
  rg_first_level:=yes

  rg_rules_dep_from += $ENV_RULES_PATH

else

  $includes

endif

CPP_FLAGS := $ENV_CPP_FLAGS \$(CPP_FLAGS)

ifeq (,\$(rg_ar_only))

  LD_FLAGS  := -L\$(top_builddir)/lib \$(LD_FLAGS)
  LIBS      := -l$ENV_TARGET \$(LIBS)

endif

$ENV_POST_CONFIG

endif # ${ENV_INCLUDE_GUARD}

EOF

}
elsif ($ARGV[0] eq "-inl")
{
  $ENV_RULES_PATH .= ".inl.pre.rules";
  $ENV_EXPANDED_RULES_PATH .= ".inl.pre.rules";
  $ENV_INCLUDE_GUARD .= "_INT_PRE_RULES_";

  my $precompiled_header = "";
  if ($ENV_PRECOMPILED_HEADER_EXPORT eq "yes")
  {
    $precompiled_header = <<EOF;

PRECOMPILED_HEADER := $ENV_PRECOMPILED_HEADER
PRECOMPILED_HEADER_FLAGS := $ENV_PRECOMPILED_HEADER_FLAGS

EOF
  }

  $rules_content=<<"EOF";
# \@file ${ENV_LIBNAME}.inl.pre.rules
#
# !!! DO NOT EDIT !!!
# This file is generated automatically by RulesGenerator (TM).

ifndef ${ENV_INCLUDE_GUARD}
define ${ENV_INCLUDE_GUARD}
1
endef

rg_ar_only := no \$(rg_ar_only)

ifeq (\$(origin rg_first_level), undefined)
  rg_first_level:=yes
endif

ifeq (\$(rg_first_level), yes)

  rg_first_level:=no
  $includes
  rg_first_level:=yes

  rg_rules_dep_from += $ENV_RULES_PATH

else

  $includes

endif

rg_ar_only := \$(wordlist 2, 100000, \$(rg_ar_only))

CPP_FLAGS := $ENV_CPP_FLAGS \$(CPP_FLAGS)
$precompiled_header
$ENV_POST_CONFIG

endif # ${ENV_INCLUDE_GUARD}

EOF

}
else
{
  die $usage;
}

open(RULES_FILE, ">$ENV_EXPANDED_RULES_PATH")
    or die "Cannot open $ENV_EXPANDED_RULES_PATH for write\n";

print RULES_FILE $rules_content;
close RULES_FILE;

sub make_dir($)
{
  my ($dir_path) = @_;
  my @dirs;

  while(! -d "${dir_path}")
  {
    push(@dirs, $dir_path);
    $dir_path=dirname($dir_path);
  }

  while($dir_path=pop(@dirs))
  {
    mkdir($dir_path, 0777);
  }

  return -d "$_[0]";
}

sub make_includes($)
{
  my ($rules_dep_from) = @_;
  my $includes="";

  for my $rule_file (split(/ /, $rules_dep_from))
  {
    $includes .= "include $rule_file\n  ";
  }

  return $includes;
}
