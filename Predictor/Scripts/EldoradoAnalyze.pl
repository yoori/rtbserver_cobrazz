#!/usr/bin/perl -w

use strict;
use warnings;

use Text::CSV_XS;
use Time::Local;
use Class::Struct;

# in file
# user,geo,[time:market:action(utm_source);]

# rule string:
# action_name(source);<300;action_name(source)

struct(Action => [
  action_prefix => '$',
  action_name => '$',
  time => '$',
  source => '$']);

struct(SubRule => [
  not => '$',
  action_prefix => '$',
  action_name => '$',
  time_less => '$',
  source => '$']);

# Rule
package Rule;

use Class::Struct;

struct('Rule',  {name => '$', sub_rules => '$', });

sub apply
{
  my ($self, $acts) = @_;
  return $self->apply_(
    $acts,
    scalar(@$acts) - 1,
    scalar(@{$self->sub_rules()}) - 1,
    undef);
}

sub apply_
{
  my ($self, $acts, $act_i, $rule_i, $ts_check) = @_;

  my $count_i = 0;

  while(1)
  {
    my $cur_rule = $self->sub_rules()->[$rule_i];
    my $rule_check = undef;

    #print STDERR "rule_i = " . $rule_i .
    #  ", act_i = $act_i" .
    #  ", ts_check = " . (defined($ts_check) ? $ts_check : 'undef') .
    #  ", act time = " . ($act_i >= 0 ? $acts->[$act_i]->time() : 'undef') . "\n";

    if(defined($ts_check) &&
      $act_i >= 0 &&
      $acts->[$act_i]->time() < $ts_check)
    {
      #print STDERR "ts_check = $ts_check, act time = " . $acts->[$act_i]->time() . "\n";
      return undef;
    }

    if(defined($cur_rule->action_name()))
    {
      #print '$cur_rule->source(): ' . (defined($cur_rule->source()) ? $cur_rule->source() : 'undef') . "\n";
      #print '$cur_act->source(): ' . (defined($cur_act->source()) ? $cur_act->source() : 'undef') . "\n";
      #print '$cur_act->source(): ' . (defined($cur_act->source()) ? $cur_act->source() : 'undef') . "\n";

      if($act_i >= 0)
      {
        my $cur_act = $acts->[$act_i];

        if(($cur_rule->action_prefix() eq '*' ||
            $cur_rule->action_prefix() eq $cur_act->action_prefix()) &&
          ($cur_rule->action_name() eq '*' ||
            $cur_rule->action_name() eq $cur_act->action_name()) &&
          ($cur_rule->source() eq "*" || $cur_rule->source() eq $cur_act->source()))
        {
          if(!defined($cur_rule->not()))
          {
            $rule_check = 1;
          }
          else
          {
            # repeat match from this point
            $rule_i = scalar(@{$self->sub_rules()}) - 1;
            ++$act_i;
          }
        }
      }
      else
      {
        if(defined($cur_rule->not()))
        {
          $rule_check = 1;
          ++$act_i; # don't move action pos
        }
      }
    }
    else
    {
      # time check
      if($act_i >= 0 && $act_i < scalar(@$acts) - 1)
      {
        my $cur_act = $acts->[$act_i + 1];

        if(defined($ts_check) && $ts_check > $cur_act->time() - $cur_rule->time_less())
        {}
        else
        {
          $ts_check = $cur_act->time() - $cur_rule->time_less();
        }
      }

      $rule_check = 1;
      ++$act_i; # don't move action pos
    }

    if(defined($rule_check))
    {
      if($rule_i == 0)
      {
        return 1;
      }
      else
      {
        $act_i -= 1;
        $rule_i -= 1;
        #return $self->apply_($acts, $act_i - 1, $rule_i - 1, $ts_check);
      }
    }
    else
    {
      if($act_i <= 0)
      {
        return undef;
      }
      else
      {
        $act_i -= 1;
        #return $self->apply_($acts, $act_i - 1, $rule_i, $ts_check);
      }
    }

    ++$count_i;
    if($count_i > 10000)
    {
      die "much check interations";
    }
  } # while
}

# main
package main;

sub to_ts
{
  my ($ft) = @_;

  if($ft =~ m|^(\d{4})-(\d{2})-(\d{2})_(\d{2}):(\d{2}):(\d{2})$|)
  {
    return timelocal($6, $5, $4, $3, $2-1, $1);

    #return DateTime->new(
    #  year => $1,
    #  month => $2,
    #  day => $3,
    #  hour => $4,
    #  minute => $5,
    #  second => $6)->epoch();
  }

  return undef;
}

sub parse_rule
{
  my ($rule_str) = @_;

  my $rule_name = '';
  if($rule_str =~ m/^([^=]+)=(.*)$/)
  {
    $rule_name = $1;
    $rule_str = $2;
  }

  my @sub_rule_strs = split(';', $rule_str);
  my @sub_rules;
  foreach my $sub_rule_str(@sub_rule_strs)
  {
    if($sub_rule_str =~ m|^\s*(~)?(?:(.*)/)?(.*)[(]\s*(.*)\s*[)]\s*$|)
    {
      push(@sub_rules, new SubRule(
        not => (defined($1) && length($1) > 0 ? 1 : undef),
        action_prefix => $2,
        action_name => $3,
        source => $4));
    }
    elsif($sub_rule_str =~ m/^[<](.*)$/)
    {
      push(@sub_rules, new SubRule(time_less => $1));
    }
    else
    {
      die "can't parse sub rule: " . $sub_rule_str;
    }
  }

  return new Rule(name => $rule_name, sub_rules => \@sub_rules);
}

sub parse_actions
{
  my ($actions_str) = @_;
  my @actions = split(';', $actions_str);
  @actions = grep { $_ ne '' } @actions;

  #print STDERR '@actions(' . scalar(@actions) . '): ' . join(',', @actions) . "\n";

  my @res_actions;

  for(my $act_i = 0; $act_i < scalar(@actions); ++$act_i)
  {
    my $act = $actions[$act_i];
    #print STDERR "act($act_i): $act\n";

    if($act =~ m|^(\d{4}-\d{2}-\d{2}_\d{2}:\d{2}:\d{2}):(?:([^:/]*)/)?([^:]*)(?::([^)]*))?(?:[(]([^)]*)[)])?$|)
    {
      my $time_str = $1;
      my $action_prefix = defined($2) ? $2 : '*';
      my $action = $3;
      my $item = defined($4) ? $4 : '';
      my $source = defined($5) ? $5 : '';
      #print STDERR "parsed full action: action_prefix=$action_prefix,action=$action,item=$item,source=$source\n";

      #if($action =~ m/^(.*)[(](.*)[)]$/)
      #{
      #  $action = $1;
      #  $source = $2;
      #  print STDERR "parsed source: $source\n";
      #}

      push(@res_actions,
        new Action(
          time => to_ts($time_str),
          action_prefix => $action_prefix,
          action_name => $action,
          source => $source,
          item => $item
          ));
    }
    else
    {
      die "can't parse action string: " . $act;
    }
  }

  return \@res_actions;
}

sub main
{
  my %rule_stats;

  my @rules;
  my $command = shift @ARGV;

  foreach my $rule(@ARGV)
  {
    push(@rules, parse_rule($rule));
  }

  my $csv = Text::CSV_XS->new({ binary => 1, eol => undef });

  my $line_i = 0;
  while(my $rows = $csv->getline(*STDIN))
  {
    my $geo = $rows->[1];
    my $actions = parse_actions($rows->[2]);

    # apply rules
    my %true_rules;

    foreach my $rule(@rules)
    {
      if(!exists($true_rules{$rule->name()}) && $rule->apply($actions))
      {
        if($command eq 'count')
        {
          if(!exists($rule_stats{$rule->name()}))
          {
            $rule_stats{$rule->name()} = 0;
          }

          $rule_stats{$rule->name()} += 1;
        }
        elsif($command eq 'print')
        {
          my @p_row = ($rule->name(), @$rows);
          $csv->print(\*STDOUT, \@p_row);
          print "\n";
        }
        else
        {
          die "unknown command: $command";
        }

        $true_rules{$rule->name()} = 1; # don't check rules with equal name
      }    
    }

    ++$line_i;
    if($line_i % 10000 == 0)
    {
      print STDERR "processed $line_i records\n";
    }
  }

  foreach my $rule_name(sort keys %rule_stats)
  {
    print $rule_name . ": " . $rule_stats{$rule_name} . "\n";
  }
}

main(@ARGV);
