package ChannelsGrannularUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;

  my $acc = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $cmp = $ns->create(Account => {
    name => 2,
    role_id => DB::Defaults::instance()->cmp_role });

  my $keyword1 = make_autotest_name($ns, "Kwd1");
  my $keyword2 = make_autotest_name($ns, "Kwd2");
  my $keyword3 = make_autotest_name($ns, "Kwd3");
  my $chan_name = make_autotest_name($ns, "1");

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    name => "Channel2",
    account_id => $acc,
    keyword_list => $keyword2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P')]));

  my $channel3 = $ns->create(DB::BehavioralChannel->blank(
    name => "Channel3",
    account_id => $acc,
    keyword_list => $keyword2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S')]));

  my $channel4 =
    $ns->create(
      DB::BehavioralChannel->blank(
        name => "Channel4",
        account_id => $cmp,
        keyword_list => $keyword2,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ],
        cpm => 10 ));

  my $expression1 = $ns->create(ExpressionChannel =>
     { name => "Expr1",
       account_id => $acc,
       expression => join("|", $channel2->channel_id,
         $channel3->channel_id) });

  $ns->output("Account", $acc);
  $ns->output("CMP", $cmp);
  $ns->output("Keyword1", $keyword1);
  $ns->output("Keyword2", $keyword2);
  $ns->output("Keyword3", $keyword3);
  $ns->output("ChannelName", $chan_name);
  $ns->output("Channel2", $channel2);
  $ns->output("Channel3", $channel3);
  $ns->output("Channel4", $channel4);
  $ns->output("Channel4Rate", $channel4->{channel_rate_id});
  $ns->output("Expr", $expression1);

}

1;
