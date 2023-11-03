#!/usr/bin/perl

use strict;
use warnings;

use Text::Template;
use File::Find::Rule ();
use File::Copy;
use Cwd 'abs_path';

our $source = undef;
our $destination = undef;
our @modules = ();
our %CONFIG;

my $usage =
  "Usage: $0 [ -y ] [ -v | -vv | -vvv ] " .
  "-i <input file or directory> " .
  "-o <output file or directory>" .
  "[-p (<processing params module>)+ ]" .
  "\nExample: " .
  "$0 -v -i test_in/ -o test_out/ -p vars\n" .
  "Options: " .
  "'-v' | '-vv' | '-vvv' : verbose level\n";

# process input args

our $verbose_level = 0; # | 1 | 2 | 3

sub run
{
  my $opt;
  my $read_source = 0;
  my $read_dest = 0;
  my $read_modules = 0;

  foreach $opt(@ARGV)
  {
    if (substr($opt, 0, 1) eq '-')
    {
      $read_modules = 0;

      if ($opt =~ m/\-(v{1,3})/)
      {
        $verbose_level = length($1);
      }
      elsif ($opt eq '-i')
      {
        $read_source = 1;
        $read_dest = 0;
      }
      elsif ($opt eq '-o')
      {
        $read_source = 0;
        $read_dest = 1;
      }
      elsif ($opt eq '-p')
      {
        $read_modules = 1;
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
      elsif ($read_modules)
      {
        @modules = (@modules, $opt);
      }
    }
  }

  if(!defined($source) || !defined($destination))
  {
    die "Not defined source or destination folder.\n$usage";
  }

  my $vars = load_vars(@modules);
  process_template_files($source, $destination, $vars);
}

sub parse_module_ref
{
  my ($module_file) = @_;
  $module_file =~ m/(.*)([:](.*))?/;
  return ($1, $2);
}

sub load_vars
{
  my (@modules) = @_;
  my %vars = ();

  foreach my $mod (@modules)
  {
    trace("Loading module $mod.\n", 2);
    my ($module_file, $module_name) = parse_module_ref($mod);
    eval('require "' . $mod . '"') or
      die "Error: can not load $mod file.\n";

    %vars = (%vars, defined($module_name) && eval("\%$module_name") || %CONFIG );
  }

  return \%vars;
}

sub process_template_files
{
  my ($source, $destination, $vars) = @_;

  my $template_config =
  {
    INCLUDE_PATH => "$source",
    INTERPOLATE => 1, # expand "$var" in plain text
    POST_CHOMP => 1, # cleanup whitespace
    EVAL_PERL => 1, # evaluate Perl code blocks
    ABSOLUTE => 1,
#   OUTPUT_PATH => "$destination"
  };

  #my $template = Text::Template->new(TYPE => 'STRING', SOURCE => $template_config);
  my $template = Text::Template->new(TYPE => 'FILE', SOURCE => abs_path($source));

  if(! -d $source)
  {
    my $tfile = $destination;

    if(-d $destination)
    {
      $tfile = $source;
      $tfile =~ s|(.*)\.t|$1|;
      $tfile = $destination . '/' . $tfile;
    }

    my $res = $template->fill_in(HASH => $vars)
      or warning(
        "Warning: while processing the '$source into '$destination': ".
        $template->error(). "\n");

    open(my $fh, '>', abs_path($tfile)) or die "Can't open target file '" + abs_path($tfile) + "' $!";
    print $fh $res;
    close $fh;
  }
  else
  {
    if($source !~ m|^.*[/]$|)
    {
      $source = $source . "/";
    }

    if($destination !~ m|^.*[/]$|)
    {
      $destination = $destination . "/";
    }


    my $directory_rule = File::Find::Rule->new;
    my @dirs = $directory_rule->relative->directory->name(qr/[^.].*/)->in("$source");

    my $process_file_rule = File::Find::Rule->new;
    my @tfiles = $process_file_rule->relative->file->name(qr/^[^.].*\.t$/)->in("$source");

    my $copy_file_rule = File::Find::Rule->new;
    my @cfiles = $copy_file_rule->relative->file->name(qr/^[^.].*(?<!\.t)$/)->in("$source");

    # process directories
    foreach my $dir (@dirs)
    {
      my $dst_dir = $destination . $dir;
      trace("Create directory '$dst_dir'\n", 2);
      mkdir($dst_dir);
    }

    # process template files
    foreach my $tfile (@tfiles)
    {
      my $src_file = $source . $tfile;
      $tfile =~ s|(.*)\.t|$1|;
      my $dst_file = $destination . $tfile;

      trace("Process '$src_file' into '$dst_file'\n", 2);

      $template->process(
        abs_path($src_file),
        $vars,
        abs_path($dst_file))
        or warning(
          "Warning: while processing the '$src_file' into '$dst_file': ".
          $template->error(). "\n");
    }

    # copy other files
    foreach my $cfile (@cfiles)
    {
      my $src_file = $source . $cfile;
      my $dst_file = $destination . $cfile;

      trace("Copying '$src_file' into '$dst_file'\n", 2);

      copy($src_file, $dst_file)
        or warning(
          "Warning: can't copy '$src_file' to '$dst_file'.\n");
    }
  }
}

sub trace
{
  my ($message, $log_level) = @_;
  if($log_level <= $verbose_level)
  {
    print $message;
  }
}

sub warning
{
  print STDERR @_;
}

run();

0;
