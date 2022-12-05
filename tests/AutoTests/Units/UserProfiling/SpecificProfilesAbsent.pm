
package SpecificProfilesAbsent;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $advertiser = $ns->create(Account => {
      name => "advertiser",
      role_id => DB::Defaults::instance()->advertiser_role});

  my $keyword1 = "http://www." . make_autotest_name($ns, "test.com/url1");
  my $keyword2 = "http://www." . make_autotest_name($ns, "test.com/url2");

  $ns->output("Keyword1", $keyword1);
  $ns->output("Keyword2", $keyword2);

  my $action = $ns->create(Action => {
    name => "NonOptedInUsers",
    url => "http:\\www.yahoo.com",
    account_id => $advertiser});
  $ns->output("Action", $action);

  my $channel1 = $ns->create(DB::BehavioralChannel->blank(
    name => "NonOptedInUsers1",
    account_id => $advertiser,
    url_list => $keyword1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U",
        time_from => 0,
        minimum_visits => 1,
        time_to => 0) ]));

  $ns->output("Channel1", $channel1->channel_id);

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    name => "NonOptedInUsers2",
    account_id => $advertiser,
    url_list => $keyword2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "U",
        time_from => 0,
        minimum_visits => 1,
        time_to => 0) ]));

  $ns->output("Channel2", $channel2->channel_id);

  my $size = $ns->create(CreativeSize => {
      name => "NonOptedInUsers",
      max_text_creatives => 2});

  my $campaign1 = $ns->create(ChannelTargetedTACampaign => {
      name => "NonOptedInUsers1",
      size_id => $size,
      template_id => DB::Defaults::instance()->text_template,
      channel_id => $channel1,
      campaigncreativegroup_cpa => 0.01,
      campaigncreativegroup_action_id => $action,
      site_links => [{name => 1}]
    });

  $ns->output("CCId1", $campaign1->{cc_id});
  $ns->output("CCGId1", $campaign1->{ccg_id});

  my $campaign2 = $ns->create(ChannelTargetedTACampaign => {
      name => "NonOptedInUsers2",
      size_id => $size,
      template_id => DB::Defaults::instance()->text_template,
      channel_id => $channel2,
      campaigncreativegroup_cpm => 20,
      campaigncreativegroup_action_id => $action,
      site_links => [{site_id => $campaign1->{Site}[0]->{site_id}}]
    });

  $ns->output("CCId2", $campaign2->{cc_id});
  $ns->output("CCGId2", $campaign2->{ccg_id});

  my $tag_id = $ns->create(PricedTag => {
      name => "NonOptedInUsers",
      size_id => $size,
      site_id => $campaign1->{Site}[0]->{site_id},
      cpm => 10 });

  $ns->output("TagId", $tag_id, "");
  
  $ns->output("DefaultColo", DB::Defaults::instance()->isp->{colo_id});
}

1;
