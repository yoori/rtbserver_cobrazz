package ChannelTriggerStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init_case_
{
  my ($self, $namespace, $case_name, $with_ad, $colo_id) = @_;

  my $ns = $namespace->sub_namespace($case_name);

  my $page_keyword_trigger = make_autotest_name($ns, "PageKeyword");
  my $search_keyword_trigger = make_autotest_name($ns, "SearchKeyword");
  my $url_trigger = "www." . make_autotest_name($ns, "url") . ".org";

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel',
    account_id => $self->{advertiser},
    keyword_list => $page_keyword_trigger . "\n" . $search_keyword_trigger,
    url_list => $url_trigger,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P'),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'S'),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U') ]
    ));

  $ns->output("PageTrigger", $page_keyword_trigger);
  $ns->output("SearchTrigger", $search_keyword_trigger);
  $ns->output("UrlTrigger", $url_trigger);

  $ns->output(
    "PageChannelTriggerId",
    $channel->keyword_channel_triggers()->[0]->channel_trigger_id());

  $ns->output(
    "SearchChannelTriggerId",
    $channel->search_channel_triggers()->[1]->channel_trigger_id());

  $ns->output(
    "UrlChannelTriggerId",
    $channel->url_channel_triggers()->[0]->channel_trigger_id());

  $ns->output("ChannelId", $channel->channel_id());

  if(defined($with_ad))
  {

    my $publisher = $ns->create(Publisher => {
      name => 'Publisher'});

    my $campaign = $ns->create(DisplayCampaign => {
      name => "Campaign",
      channel_id => $channel,
      site_links => [ { site_id => $publisher->{site_id} } ]  });

    $ns->output("Tid", $publisher->{tag_id});
    $ns->output("CC", $campaign->{cc_id});
  }

  if(defined($colo_id))
  {
    $ns->output("Colo", $colo_id);
  }
}

sub url_keyword_
{
  my ($self, $namespace) = @_;
  my $ns = $namespace->sub_namespace("URLKWD");
  my $page = make_autotest_name($ns, 'page');
  my $search = make_autotest_name($ns, 'search');
  my $url = make_autotest_name($ns, 'url');

  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => "Channel",
    account_id => $self->{advertiser},
    keyword_list => $page,
    search_list => $search,
    url_kwd_list => $url,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P', time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'S', time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'R', time_to => 60),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U', time_to => 0) ] ));

  $ns->output("PageTrigger", $page);
  $ns->output("SearchTrigger", $search);
  $ns->output("UrlKwdTrigger", $url);

  $ns->output("Channel/P", $channel->page_key());
  $ns->output("Channel/S", $channel->search_key());
  $ns->output("Channel/R", $channel->url_kwd_key());

  $ns->output(
    "PageChannelTriggerId",
    $channel->keyword_channel_triggers()->[0]->channel_trigger_id());

  $ns->output(
    "SearchChannelTriggerId",
    $channel->search_channel_triggers()->[0]->channel_trigger_id());

  $ns->output(
    "UrlKwdChannelTriggerId",
    $channel->url_kwd_channel_triggers()->[0]->channel_trigger_id());


}

sub adsc_6348_
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace("ADSC-6348");

  my $keyword1 = make_autotest_name($ns, 'keywords2');
  my $keyword2 = make_autotest_name($ns, 'softing');
  my $channel1 = $ns->create(DB::BehavioralChannel->blank(
    name => "Channel-1",
    account_id => $self->{advertiser},
    keyword_list => $keyword1,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]
    ));

  my $channel2 = $ns->create(DB::BehavioralChannel->blank(
    name => "Channel-2",
    account_id => $self->{advertiser},
    keyword_list => $keyword1 . " " . $keyword2,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]
    ));


  $ns->output("PageTrigger", "\\\\" . $keyword1 . " \\\\" . $keyword2);

  $ns->output(
    "PageChannelTriggerId/1",
    $channel1->keyword_channel_triggers()->[0]->channel_trigger_id());

  $ns->output(
    "PageChannelTriggerId/2",
    $channel2->keyword_channel_triggers()->[0]->channel_trigger_id());
}

sub adsc_7962_
{
  my ($self, $namespace) = @_;

  my $ns = $namespace->sub_namespace("ADSC-7962");

  # ADSC-7962
  # triggers
  my $keyword = make_autotest_name($ns, "keyword");
  my $text1 = make_autotest_name($ns, "text1");
  my $text2 = make_autotest_name($ns, "text2");
  my $page_keyword = make_autotest_name($ns, "page");
  my $search_keyword = make_autotest_name($ns, "search");

  $ns->output("KEYWORD", $keyword);
  $ns->output("TEXT1", $text1);
  $ns->output("TEXT2", $text2);
  $ns->output("PAGE_KEYWORD", $page_keyword);
  $ns->output("SEARCH_KEYWORD", $search_keyword);

  # channel
  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => "Channel",
    account_id => $self->{advertiser},
    keyword_list => "$keyword\n$text1\n$text2\n$page_keyword",
    search_list => "$keyword\n$text1\n$text2\n$search_keyword",
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P'),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'S'),
      DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U'), ]
    ));

  $ns->output("KEYWORD/CHANNEL_TRIGGER_ID/P",
    $channel->keyword_channel_triggers()->[0]->channel_trigger_id());

  $ns->output("KEYWORD/CHANNEL_TRIGGER_ID/S",
    $channel->search_channel_triggers()->[0]->channel_trigger_id());

  $ns->output("TEXT1/CHANNEL_TRIGGER_ID/P",
    $channel->keyword_channel_triggers()->[1]->channel_trigger_id());

  $ns->output("TEXT2/CHANNEL_TRIGGER_ID/P",
    $channel->keyword_channel_triggers()->[2]->channel_trigger_id());

  $ns->output("PAGE_KEYWORD/CHANNEL_TRIGGER_ID/P",
    $channel->keyword_channel_triggers()->[3]->channel_trigger_id());

  $ns->output("SEARCH_KEYWORD/CHANNEL_TRIGGER_ID/S",
    $channel->search_channel_triggers()->[3]->channel_trigger_id());
}



sub init
{
  my ($self, $ns) = @_;

  $ns->output("DefaultColo", DB::Defaults::instance()->isp->{colo_id});

  $self->{advertiser} = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role });

  $self->init_case_($ns, 'NoTid', undef, undef);

  $self->init_case_($ns, 'WithAd', 1, 
    DB::Defaults::instance()->ads_isp->{colo_id});

  $self->url_keyword_($ns);
  $self->adsc_6348_($ns);
  $self->adsc_7962_($ns);

}

1;
