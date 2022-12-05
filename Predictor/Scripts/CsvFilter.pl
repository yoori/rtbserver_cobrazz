#!/usr/bin/perl -w

# TODO
# ability to define package as filter (packages in CsvUtils folder, CsvUtils::X)
#
# input hooks
#   --input=Input::Find('MASK') \
#   --input=Input::Std
#
# process, filter, output hooks
#   --process=Process::InFile(field=3,file="<file>")
#   --process=Process::NotInFile(field=3,file="<file>")
#   --process=Output::Std
#   --process='Output::Distrib(chunks=20,file="RBid_##ID##",field=3)'
#
use strict;
use warnings;
use Encode;

sub init_hook
{
  my ($module, $args) = @_;
  my $res_module = "CsvUtils::" . $module;

  eval
  {
    (my $file = $res_module) =~ s|::|/|g;
    require $file . '.pm';
    $res_module->import();
    1;
  } || die "Can't load '$module': " . $@;

  return $res_module->new(%$args);
};

package main;

sub main
{
  my @str_args = @_;

  my @inputs;
  my @processors;

  # parse parameters
  foreach my $arg(@ARGV)
  {
    my $command;
    my $value;

    Encode::_utf8_on($arg);

    if($arg =~ m/^--([^=]*)=(.*)$/)
    {
      $command = $1;
      $value = $2;
    }
    elsif($arg =~ m/^--([^=]*)='(.*)'$/)
    {
      $command = $1;
      $value = $2;
    }
    else
    {
      die "Can't parse argument: $arg";
    }

    if($command eq "input" || $command eq "process")
    {
      # parse hook arguments
      my $module_name;
      my %args;
      if($value =~ m/^([^(]+)[(](.*)[)]$/)
      {
        $module_name = $1;
        my $args_str = "(" . $2 . ")";
        eval qq{ \%args = $args_str; };
        if($@)
        {
          die "Can't parse arguments: $args_str"
        }
      }
      else
      {
        $module_name = $value;
      }

      my $hook = init_hook($module_name, \%args);
      if($command eq "input")
      {
        push(@inputs, $hook);
      }
      elsif($command eq "process")
      {
        push(@processors, $hook);
      }
    }
    else
    {
      die "Unknown command: $command";
    }
  }

  # fetch & process inputs
  foreach my $input(@inputs)
  {
    while(my $row = $input->get())
    {
      my @cur_rows = ($row);

      foreach my $processor(@processors)
      {
        if(scalar(@cur_rows) == 0)
        {
          last;
        }

        my @new_rows;

        foreach my $sub_row(@cur_rows)
        {
          my @sarr = $processor->process($sub_row);

          foreach my $check_row(@sarr)
          {
            if(defined($check_row))
            {
              push(@new_rows, $check_row);
            }
          }
        }

        @cur_rows = @new_rows;
      }
    }
  }

  for(my $i = 0; $i < scalar @processors; ++$i)
  {
    my $processor = $processors[$i];
    my $rows = $processor->flush();

    # process flush result
    if(defined($rows) && $i < scalar @processors - 1)
    {
      my @cur_rows = @$rows;

      for(my $next_i = $i + 1; $next_i < scalar @processors; ++$next_i)
      {
        if(scalar(@cur_rows) == 0)
        {
          last;
        }

        my @new_rows;

        foreach my $row(@cur_rows)
        {
          my @sarr = $processors[$next_i]->process($row);

          foreach my $check_row(@sarr)
          {
            if(defined($check_row))
            {
              push(@new_rows, $check_row);
            }
          }
        }

        @cur_rows = @new_rows;
      }
    }
  }
}

main(@ARGV);
