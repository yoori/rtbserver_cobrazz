
package ZerodowntimeTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  # Common data
  my $publisher1 = $ns->create(
    Publisher => { name => 'Publisher1'});

  my $publisher2 = $ns->create(
    Publisher => { name => 'Publisher2'});

  my $advertiser = $ns->create(
    Advertiser => { name => 'Advertiser'});

  my $keyword_ctx = make_autotest_name($ns, "Context");
  my $keyword_ctx_new = make_autotest_name($ns, "ContextNew");
  my $keyword_non_ctx = make_autotest_name($ns, "Noncontext");

  my $channel_ctx = $ns->create(
    DB::BehavioralChannel->blank(
      name => "Context",
      account_id => $advertiser,
      keyword_list => $keyword_ctx,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P") ]));

  my $trigger_ctx_new = $ns->create(
    DB::BehavioralChannel::ChannelTrigger->blank(
      channel_id => $channel_ctx->channel_id(),
      trigger_id =>
         DB::BehavioralChannel::Trigger->blank(
           normalized_trigger => lc $keyword_ctx_new,
           country_code => $channel_ctx->{country_code}),
      qa_status => 'D',
      original_trigger => $keyword_ctx_new));

  my $channel_non_ctx = $ns->create(
    DB::BehavioralChannel->blank(
      name => "Non-Context",
      account_id => $advertiser,
      keyword_list => $keyword_non_ctx,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => "P",
          minimum_visits => 2,
          time_to => 3*60*60 ) ]));

  my $expression = $ns->create(DB::ExpressionChannel->blank(
      name => "Expression",
      account_id => $advertiser,
      expression => $channel_ctx->channel_id() . "|" .
        $channel_non_ctx->channel_id()));

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    account_id => $advertiser,
    channel_id => $expression,
    campaigncreativegroup_cpm => 1,
    site_links => [{site_id => $publisher1->{site_id} }] });

  my $campaign_d = $ns->create(DisplayCampaign => {
    name => 'Campaign-D',
    account_id => $advertiser,
    channel_id => $expression,
    campaigncreativegroup_cpm => 5,
    campaign_status => 'D',
    site_links => [{site_id => $publisher1->{site_id} }] });

  my $publisher_cpa = $ns->create(
    Publisher => { name => 'PublisherCPA'});

  my $isp_cpa = $ns->create(Isp => {
      name => 'Isp',
      colocation_revenue_share => 0.5,
      account_currency_id => 
        DB::Currency->blank( rate => 0.51 ),
      account_internal_account_id =>
        DB::Defaults::instance()->no_margin_internal_account->{account_id} });

  # Click, action & discover request data
  for (my $i = 1; $i <= 8; ++$i)
  {
    my $url = "http://www." . make_autotest_name($ns, "test.com/url" . $i);
    my $keyword = make_autotest_name($ns, "kwd" . $i);

    my $action = $ns->create(Action => {
      name => "Action" . $i,
      account_id => $advertiser});

    my $channel = $ns->create(
      DB::BehavioralChannel->blank(
        name => "ChannelUrl" . $i,
        account_id => $advertiser,
        url_list => $url,
        keyword_list => $keyword,
        behavioral_parameters => [
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => "U"),
          DB::BehavioralChannel::BehavioralParameter->blank(
            trigger_type => "P") ]));

    my $c = $ns->create(
      DisplayCampaign => {
      name => 'Campaign-CPA' . $i,
      account_id => $advertiser,
      campaigncreativegroup_action_id => $action,
      channel_id => $channel,
      campaigncreativegroup_cpa => 1,
      campaigncreativegroup_ar => 0.01,
      site_links => [{site_id => $publisher_cpa->{site_id} }] });

    my $wdtag = $ns->create(WDTag => {
      name => 'WDTag' . $i,
      site_id => $publisher1->{site_id} });

    $ns->output("URL/" . $i, $url);
    $ns->output("KEYWORD/" . $i, $keyword);
    $ns->output("TRIGGER/" . $i, 
      $channel->keyword_channel_triggers_->[0]->channel_trigger_id());
    $ns->output("CHANNEL/" . $i, $channel);
    $ns->output("CPA/CC/" . $i, $c->{cc_id});
    $ns->output("CPA/CCG/" . $i, $c->{ccg_id});
    $ns->output("ACTION/" . $i, $action);
    $ns->output("WDTAG/" . $i, $wdtag);
  }

  # Precondition phase data
  my $publisher_pre = $ns->create(
    Publisher => { name => 'PublisherPre'});

  for (my $i = 1; $i <= 2; ++$i)
  {
    my $url = "http://www." . make_autotest_name($ns, "test-pre.com/url" . $i);

    my $channel = $ns->create(
      DB::BehavioralChannel->blank(
         name => "ChannelPre" . $i,
         account_id => $advertiser,
         url_list => $url,
         behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(
              trigger_type => "U") ]));

    my $c = $ns->create(
      DisplayCampaign => {
      name => 'CampaignPre' . $i,
      account_id => $advertiser,
      channel_id => $channel,
      campaigncreativegroup_cpm => 1,
      site_links => [{site_id => $publisher_pre->{site_id} }] });

    $ns->output("URL/PRE/" . $i, $url);
    $ns->output("CC/PRE/" . $i, $c->{cc_id});
    $ns->output("BP/PRE/" . $i, $channel->url_key());
    $ns->output("CHANNEL/PRE/" . $i, $channel);
  }

  $ns->output("KWD/CTX", $keyword_ctx);
  $ns->output("KWD/CTX_NEW", $keyword_ctx_new);
  $ns->output("KWD/CTX/NORM", lc $keyword_ctx);
  $ns->output("KWD/CTX_NEW/NORM", lc $keyword_ctx_new);
  $ns->output("KWD/NON_CTX", $keyword_non_ctx);
  $ns->output("CHANNEL/CTX", $channel_ctx);
  $ns->output("BP/CTX", $channel_ctx->page_key());
  $ns->output("CHANNELTRIGGER/CTX_NEW", $trigger_ctx_new);
  $ns->output("CHANNEL/NON_CTX", $channel_non_ctx);
  $ns->output("CHANNEL/EXPRESSION", $expression);
  $ns->output("TAG/1", $publisher1->{tag_id});
  $ns->output("TAG/2", $publisher2->{tag_id});
  $ns->output("TAG/PRE", $publisher_pre->{tag_id});
  $ns->output("TAG/CPA", $publisher_cpa->{tag_id});
  $ns->output("PUB_ACCOUNT/CPA", $publisher_cpa->{account_id});
  $ns->output("COLO/CPA", $isp_cpa->{colo_id});
  $ns->output("ISP_ACCOUNT/CPA", $isp_cpa->{account_id});
  $ns->output("SITE/1", $publisher1->{site_id});
  $ns->output("SITE/2", $publisher2->{site_id});
  $ns->output("CC", $campaign->{cc_id});
  $ns->output("CC/D", $campaign_d->{cc_id});
  $ns->output("CCG", $campaign->{ccg_id});
  $ns->output("CCG/D", $campaign_d->{ccg_id});
  $ns->output("CAMPAIGN/D", $campaign_d->{campaign_id});
  $ns->output("COLO/ALL",
    DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("MAPPING",
    DB::Defaults::instance()->wdtag_mapping_optout->{name});
  $ns->output("WDREQUESTMAPPING/NAME",
    make_autotest_name($ns, "Context"));
}

1;
