#!/usr/bin/perl
use warnings;
use strict;

=head1 NAME

unit.pl - prepare environment for the new AutoTest unit.

=head1 SYNOPSIS

  unit.pl OPTIONS

=head2 OPTIONS

=over

=item C<--category, -c category name>

B<Required.> Specifies category name for the new unit. If there isn't
such category in the category list script creates new one. 

=item C<--testname, -n unit name>

B<Required.> Specifies unit name. In case of creation new unit
you should indicate new unit name as a argument of this option,
and in case of deleting - name of unit which it is necessary to delete.
Test name length must be less than 30 chars.

=item C<--add, -a>

B<Optional.> Enables 'add' script scenario. Script
adds new unit to AutoTests environment. Used by default.
You can't use this key together with C<--delete> key (see below).

=item C<--delete, -d>

B<Optional.> Enables 'delete' script scenario. Script
deletes existing unit from AutoTests environment.
If neither C<--add> nor C<--delete> keys are enabled,
'add' scenario will be performed by default.
You can't use this key together with C<--add> key (see above).

=item C<--group, -g (fast, quiet, slow)>

B<Optional.> Specifies unit's group by its execution time.
Can be one of the following:

=over

=over

=item 1)I<fast>
 - tests with executing time less than 60 sec

=item 2)I<quiet>
 - tests with execution time more than 60 sec

=item 3)I<slow>
 - tests that work with DB and with execution time more than 15 min

=back

=back

If group not defined, 'fast' used by default.

=back

=head2 See also:

https://confluence.ocslab.com/display/ADS/Programmer%27s+Guide

=cut

use FindBin;
use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;
use File::Spec;
use File::Path;

my @test_groups = ("fast", "quiet", "slow"); 

use constant UNITS_PATH               => "$FindBin::Dir/../Units";
use constant MAX_TESTNAME_LENGTH      => 30;

my %option = (group => 'fast');

if (!GetOptions(\%option,
                qw(category|c=s testname|n=s add|a delete|d group|g=s))
    || (grep { not defined } @option{qw(category testname)}))
{
  pod2usage(1);
}

if (not grep {$option{group} eq $_} @test_groups)
{
  print "\033[31munit.pl: Error: argument of '--group' key must be one of the following: fast, quiet, slow!\033[0m\n";
  pod2usage(1);
}

my $new_unit_name    = $option{testname};
my $new_cat_name     = $option{category};
my $sub_path         = $new_cat_name;
my $def_name         = "AutoTests" . $new_cat_name . "Units";

$def_name =~ s/\.//;
my $make_file_def = "\@" . lc($def_name) . "_deps\@";
$sub_path =~ s/\./\//;
my $test_path =  File::Spec->join(UNITS_PATH,$sub_path);

my $makefile_path =  File::Spec->join($test_path,"Makefile.in");
my $makefile_tmp_path = File::Spec->join($test_path,"~Makefile.in");
my $deffile_path = File::Spec->join($test_path,"$def_name.def");
my $dirac_path = File::Spec->join($test_path,"dir.ac");

if (! -d $test_path) 
{
  File::Path::mkpath($test_path) || die "Cann't create directory $test_path";
}

my $makefile_exists = 1;
unless (-e $makefile_path) 
{
  $makefile_exists =  0;
  # add Makefile.in
  open(TARGET, ">$makefile_tmp_path") || die "Cann't open file $makefile_path\n";
  print TARGET << "EOF;";
$make_file_def

sources := \\
  $new_unit_name.cpp \\

includes := .

$make_file_def
EOF;

  close(TARGET);

  # Add def file
  open(TARGET, ">$deffile_path") || die "Cann't open file $deffile_path\n";
  print TARGET << "EOF;";
name="$def_name"
so_files=$def_name

osbe_cxx_dep ExceptionHandling
osbe_cxx_dep Generics
osbe_cxx_dep Commons
osbe_cxx_dep AutoTestsCommons
osbe_cxx_dep AdServerCommons
EOF;
  close(TARGET);

  # Add dir.ac file
  open(TARGET, ">$dirac_path") || die "Cann't open file $deffile_path\n";
  print TARGET << "EOF;";
OSBE_CONFIG_FILE([Makefile])

OSBE_CXX_DEF([$def_name])
EOF;
  close(TARGET);

  
}

if (length($new_unit_name) > MAX_TESTNAME_LENGTH and $option{add})
{
  print "\033[31munit.pl: Error: Test name length must be less than ".MAX_TESTNAME_LENGTH."\033[0m\n";
  pod2usage(1);
}

my $cpp_file_path    = File::Spec->join($test_path, "$new_unit_name.cpp");
my $hpp_file_path    = File::Spec->join($test_path, "$new_unit_name.hpp");
my $perl_module_path = File::Spec->join($test_path, "$new_unit_name.pm");

if ( not $option{delete})
{
  my $uc_unitname = uc($new_unit_name);
  my $BASE_UNIT_INCLUDE_STRING = "<tests/AutoTests/Commons/Common.hpp>";
  # add C++ header stub for test
  open(TARGET, ">$hpp_file_path") || die "Cann't open file $hpp_file_path\n";
  print TARGET << "EOF;";
#ifndef _AUTOTEST__$uc_unitname\_
#define _AUTOTEST__$uc_unitname\_
  
#include $BASE_UNIT_INCLUDE_STRING
  

class $new_unit_name : public BaseUnit
{
public:
  $new_unit_name(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual \~$new_unit_name() throw()
  {};

private:

  virtual bool run_test();
};

#endif // _AUTOTEST__$uc_unitname\_
EOF;
 
  close(TARGET);

  my $cpp_qroup="AUTO_TEST_FAST";
  if ($option{group} eq "slow")
  {
    $cpp_qroup="AUTO_TEST_SLOW";
  }
  elsif ($option{group} eq "quiet")
  {
    $cpp_qroup="AUTO_TEST_QUIET";
  }

  # add C++ file stub for test
  open(TARGET, ">$cpp_file_path") || die "Cann't open file $cpp_file_path.\n";
  print TARGET << "EOF;";
#include "$new_unit_name.hpp"

REFLECT_UNIT($new_unit_name) (
  "$new_cat_name",
  $cpp_qroup);

bool
$new_unit_name\:\:run_test()
{
  return true;
}

EOF;

  close(TARGET);

  # add perl module stub for test
  open(TARGET, ">$perl_module_path") || die "Can't open file $perl_module_path.\n";
  print TARGET << "EOF;";
#v2

package $new_unit_name;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my (\$self, \$ns) = \@_;
  # Remove this comment and insert your code here
}

1;
EOF;


if ($makefile_exists) 
{
  # Modify Makefile.in
  open(SOURCE, $makefile_path) || die "Cann't open file $makefile_path\n";
  my @make=<SOURCE>;
  my $make_size=@make;
  close(SOURCE);

  open(TARGET, ">" . $makefile_tmp_path) || die "Cann't open file $makefile_tmp_path\n";

  my $found_source=0;
  my $first=0;
  for (my $ind=0; $ind < $make_size; ++$ind)
  {
    my $string=$make[$ind];

    if ($found_source == 0)
    {
      if ($string =~ /sources/o)
      {
        $found_source=1;
      }
      print TARGET $string;
    }
    else 
    {
      if (not $string =~ /cpp/o)
      {
        my $last_str = $make[$ind - 1];
        my $new_str = $last_str;
        if ($last_str =~ /\\/o)
        {}
        else
        {
          $last_str =~ s/\n/ \\\n/;
        }
        print TARGET $last_str;
        $new_str =~ s/\w+\.cpp/$new_unit_name\.cpp/;
        print TARGET $new_str;
	print TARGET $string;

        $found_source = 0;
        $first=0;
      }
      elsif ($first != 0)
      {
        print TARGET $make[$ind - 1];
      }
      else
      {
        $first=1;
      }

    }
  }
  close(TARGET);
 }

}
elsif ($option{add})
{
  print "\033[31munit.pl: Error: both '--add' and '--delete' keys appear in command line!\033[0m\n";
  pod2usage(1);
}
else
{
  open(SOURCE, $makefile_path) || die "Cann't open file $makefile_path\n";
  my @make=<SOURCE>;
  my $make_size=@make;
  close(SOURCE);

  open(TARGET, ">" . $makefile_tmp_path) || die "Cann't open file $makefile_tmp_path\n";

  for (my $ind=0; $ind < $make_size; ++$ind)
  {
    my $string=$make[$ind];

    if ($string =~ /(\w*?)$new_unit_name(\w*?)/ &&
        $1 eq "" && $2 eq "")
    {

    }
    else
    {
      print TARGET $string;
    }
  }
  
  close(TARGET);

  unlink $hpp_file_path                              || die "Error: can't remove file $hpp_file_path\n";
  unlink $cpp_file_path                              || die "Error: can't remove file $cpp_file_path\n";
  unlink $perl_module_path                           || die "Error: can't remove file $perl_module_path\n";

}

unlink $makefile_path     || die "Error: can't remove file $makefile_path\n";

rename($makefile_tmp_path, $makefile_path)         || die "Error: can't rename file $makefile_tmp_path\n";

print "\033[32mDon't forget to run osbe!\033[0m\n";

exit(0);
