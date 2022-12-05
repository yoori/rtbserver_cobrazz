package CombinedChannelsMatchingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init
{
  my ($self, $ns) = @_;
  my $kwdlist1 = "\"exchange rate\" stocks\n" .
      "stocks price\n" .
      "stocks OpTion\n" .
      "cdml2search";
  my $kwdlist2 = "\"nice bird\" PHOTO\n" .
      "\"Secretary bird\"\n";
  my $kwdlist3 = "\"credit limit\"";
  my $kwdlist4 = "plate\n" .
      "dish\n" .
      "glaze\n" .
      "crockery";
  my $kwdlist5 = "audi\n" .
      "renault\n" .
      "citroen";
  my $kwdlist6 = "reinsurance\n" .
      "medical insurance";
  my $kwdlist7 = "clothes\n" .
      "shops\n" .
      "fashion";
  my $kwdlist8 = "ahmad\n" .
      "maitre\n" .
      "lipton\n" .
      "tea\n" .
      "teacup";
  my $kwdlist9 = "generating machinery\n" .
      "electric goods\n" .
      "accumulator\n" .
      "electric-battery car\n" .
      "electric fan";

  my $urllist1 = "cnews.ru\n" .
      "cnews.fr";

  my $urllist2 = "electricshop.com";

  my $account = $ns->create(Account =>
                              { name => 1,
                                role_id => DB::Defaults::instance()->advertiser_role });

  # Simple channels
  my $channel1 = $ns->create(DB::BehavioralChannel->blank(
    name => 1,
    account_id => $account,
    keyword_list => $kwdlist1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 60, trigger_type => "S") ]));

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    name => 2,
    account_id => $account,
    keyword_list => $kwdlist2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 60, trigger_type => "P") ]));

  my $channel3 = $ns->create(DB::BehavioralChannel->blank(
    name => 3,
    account_id => $account,
    keyword_list => $kwdlist3,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P") ]));

  my $channel4 = $ns->create(DB::BehavioralChannel->blank(
    name => 4,
    account_id => $account,
    keyword_list => $kwdlist4,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P"),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "S") ]));

  my $channel5 = $ns->create(DB::BehavioralChannel->blank(
    name => 5,
    account_id => $account,
    keyword_list => $kwdlist5,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 60, trigger_type => "P") ]));

  my $channel6 = $ns->create(DB::BehavioralChannel->blank(
    name => 6,
    account_id => $account,
    url_list => $urllist1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U") ]));

  my $channel7 = $ns->create(DB::BehavioralChannel->blank(
    name => 7,
    account_id => $account,
    keyword_list => $kwdlist6,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 60, trigger_type => "P") ]));

  my $channel8 = $ns->create(DB::BehavioralChannel->blank(
    name => 8,
    account_id => $account,
    keyword_list => $kwdlist7,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P") ]));

  my $channel9 = $ns->create(DB::BehavioralChannel->blank(
    name => 9,
    account_id => $account,
    keyword_list => $kwdlist8,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'S', time_to =>  60),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P', time_to =>  60) ]));

  my $channel10 = $ns->create(DB::BehavioralChannel->blank(
    name => 10,
    account_id => $account,
    keyword_list => $kwdlist9,
    url_list => $urllist2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P"),
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U") ]));

  # Expression channels
  my $expr_channel1 = $ns->create(DB::ExpressionChannel->blank(
    name => "Expr1",
    account_id => $account,
    expression => $channel1->channel_id . "&" . $channel2->channel_id));

  my $expr_channel2 = $ns->create(DB::ExpressionChannel->blank(
    name => "Expr2",
    account_id => $account,
    expression => $channel3->channel_id . "|" . $channel4->channel_id));

  my $expr_channel3 = $ns->create(DB::ExpressionChannel->blank(
    name => "Expr3",
    account_id => $account,
    expression => $channel5->channel_id . "^" . $channel6->channel_id));

  my $expr_channel4 = $ns->create(DB::ExpressionChannel->blank(
    name => "Expr4",
    account_id => $account,
    expression => $channel7->channel_id . "|" . $channel8->channel_id . "^" . 
      $channel9->channel_id));

  my $expr_channel5 = $ns->create(DB::ExpressionChannel->blank(
    name => "Expr5",
    account_id => $account,
    expression => "(" . $channel7->channel_id . "|" . 
      $channel8->channel_id . ")^" . 
      $channel9->channel_id));

  my $expr_channel6 = $ns->create(DB::ExpressionChannel->blank(
    name => "Expr6",
    account_id => $account,
    expression => "(" . $channel3->channel_id . "|" . 
      $channel4->channel_id . ")^(" . 
      $channel9->channel_id  . "|" .
      $channel10->channel_id . ")&(" . 
      $channel4->channel_id . "&" . 
      $channel3->channel_id  . "|" .
      $channel9->channel_id . ")"));

  # Campaign
  my $expr_ccg = $ns->create(DB::ExpressionChannel->blank(
    name => "ExprCCG",
    account_id => $account,
    expression => $expr_channel1->channel_id . "|" . 
      $expr_channel2->channel_id . "|" . 
      $expr_channel3->channel_id  . "|" .
      $expr_channel4->channel_id . "|" . 
      $expr_channel5->channel_id . "|" . 
      $expr_channel6->channel_id));

  my $publisher = $ns->create(Publisher => { name => "Publisher" });
  
  my $campaign  = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    channel_id =>  $expr_ccg->{channel_id},
    targeting_channel_id => undef,
    campaigncreativegroup_cpm => 1000,
    site_links => [{ site_id =>  $publisher->{site_id} }] });

  $ns->output("REF1", "search.live.fr/search?hl=en&q=Stocks+option&btnG=Search");
  $ns->output("REF2", "http://search.live.de/results.aspx?q=stocks+option");
  $ns->output("KWD2", "secretary bird");
  $ns->output("KWD3", "glaze");
  $ns->output("KWD4", "credit limit");
  $ns->output("REF5", "cnews.ru");
  $ns->output("KWD5", "citroen");
  $ns->output("KWD6", "citroen");
  $ns->output("KWD7", "medical insurance,shops,tea");
  $ns->output("KWD8", "shops");
  $ns->output("KWD9", "medical insurance,shops,tea");
  $ns->output("KWD10", "shops");
  $ns->output("KWD11", "credit limit,glaze");
  $ns->output("KWD12", "credit limit,glaze");
  $ns->output("REF12", "www.electricshop.com");

  $ns->output("Channel1", $expr_channel1->channel_id);
  $ns->output("Channel2", $expr_channel2->channel_id);
  $ns->output("Channel3", $expr_channel3->channel_id);
  $ns->output("Channel4", $expr_channel4->channel_id);
  $ns->output("Channel5", $expr_channel5->channel_id);
  $ns->output("Channel6", $expr_channel6->channel_id);
  $ns->output("Tag", $publisher->{tag_id});
  $ns->output("CC", $campaign->{cc_id});
} 

1;
