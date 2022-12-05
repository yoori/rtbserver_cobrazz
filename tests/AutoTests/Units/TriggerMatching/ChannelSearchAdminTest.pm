package ChannelSearchAdminTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_expression_channel_
{
  my ($ns, $acc, $format, $defaults, @triggers) = @_;

  my $i = 0;
  while (my ($trigger_type, $trigger_list) = splice(@triggers, 0, 2))
  {
    my $name = sprintf($format, ++$i);

    my $trigger_list_key =
      $trigger_type eq "U" ? 'url_list' : 'keyword_list';

    my %bp_args = %$defaults;
    $bp_args{trigger_type} = $trigger_type;

    my $behav_channel = $ns->create(DB::BehavioralChannel->blank(
      name => $name,
      account_id => $acc,
      $trigger_list_key => $trigger_list,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(%bp_args) ]));

    my $ch = $ns->create(DB::ExpressionChannel->blank(
      name => "$name-1",
      account_id => $acc,
      expression => $behav_channel->channel_id()));
    $ns->output("$name-1", $ch);

    $ch = $ns->create(DB::ExpressionChannel->blank(
      name => "$name-2",
      account_id => $acc,
      expression => $ch->channel_id));
    $ns->output("$name-2", $ch);
  }
}

sub init
{
  my ($self, $ns) = @_;

  my $test_name = "ChannelSearchAdminTest";

  my $acc = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  # CombinedHardSoftMatchingTest
  create_expression_channel_(
    $ns, $acc, "CHSMT-%02d", {},
    P => "\"". $test_name . "Test3 ". $test_name . 
      "Test4\" ". $test_name . "Test5");

  # HardTriggerMatchingTest
  create_expression_channel_(
    $ns, $acc, "HTMT-%02d", {},
    P => "\"" . $test_name . "Test1 ". $test_name . "Test2\"");

  # MultipleChannelsMatchingTest
  create_expression_channel_(
    $ns, $acc, "MCMT-%02d", {},
    P => $test_name . "Test10",
    P => $test_name . "Test11",
    P => $test_name . "Test12",
    U => $test_name . "Test12",
    P => $test_name . "Test13",

    P => $test_name . "Test14",
    P => $test_name . "Test14");

  # RefererMatchingTest
  create_expression_channel_(
    $ns, $acc, "RMT-%02d", {},
    U => "http://dev.ocslab.com/services/");

  # SoftTriggerMatchingTest
  create_expression_channel_(
    $ns, $acc, "STMT-%02d", {},
    P => $test_name . "Test1 ". $test_name . "Test2",
    P => $test_name . "Test15 ". $test_name . "Test16 ". 
      $test_name . "Test17 ". $test_name . "Test18",
    P => $test_name . "Test17 ". $test_name . "Test19 ". $test_name . 
      "Test20 ". $test_name . "Test16 ". $test_name . "Test21");
}

1;
