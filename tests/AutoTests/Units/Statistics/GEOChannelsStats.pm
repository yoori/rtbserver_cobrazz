
package GEOChannelsStats;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_display_campaign
{
  my ($self, $ns, $suffix, $params) = @_;
  
  my $publisher = 
     $ns->create(Publisher => { 
       name => 'DisplayPublisher-' . $suffix,
       pricedtag_adjustment => 1.0,
       pubaccount_country_code => 'GB' });

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'DisplayCampaign-' . $suffix,
    account_id => $params->{account},
    channel_id => $params->{channel},
    country_code => 'GB',
    campaigncreativegroup_cpm => $params->{cpm},
    campaigncreativegroup_cpc => $params->{cpc},
    campaigncreativegroup_cpa => $params->{cpa},
    campaigncreativegroup_ctr => 0.01,
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES,
    campaigncreativegroup_geo_channels =>
      $params->{geo_channels},
    site_links => 
      [{ site_id => $publisher->{site_id} }] });

  $ns->output("DISPLAYTID-" . $suffix, $publisher->{tag_id});
  $ns->output("DISPLAYCC-"  . $suffix, $campaign->{cc_id});
}

sub init {
  my ($self, $ns) = @_;
  
  my $account = $ns->create(Account => {
    name => 'Advertiser',
    country_code => 'GB',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $state1 = $ns->create(DB::GEOChannel->blank(
    name => 'London, City of',
    parent_channel_id => 
       DB::Defaults::instance()->geo_gb_country->{channel_id},
    country_code => 'GB',
    geo_type => 'STATE'));

  my $state2 = $ns->create(DB::GEOChannel->blank(
    name => 'Middlesborough, City of',
    parent_channel_id => 
       DB::Defaults::instance()->geo_gb_country->{channel_id},
    country_code => 'GB',
    geo_type => 'STATE'));

  my $state3 = $ns->create(DB::GEOChannel->blank(
    name => 'Stirling',
    parent_channel_id => 
       DB::Defaults::instance()->geo_gb_country->{channel_id},
    country_code => 'GB',
    geo_type => 'STATE'));

  my $city1 = $ns->create(DB::GEOChannel->blank(
    name => 'London',
    country_code => 'GB',
    geo_type => 'CITY',
    parent_channel_id =>  $state1->{channel_id},
    city_list => $ns->namespace . "-"  . 'London',
    latitude => 51.5002,
    longitude => -0.1262));

  my $city2 = $ns->create(DB::GEOChannel->blank(
    name => 'Middlesborough',
    country_code => 'GB',
    geo_type => 'CITY',
    parent_channel_id =>  $state2->{channel_id},
    city_list =>  $ns->namespace . "-"  . 'Middlesborough',
    latitude => 54.5728,
    longitude => -1.1628));

  my $city3 = $ns->create(DB::GEOChannel->blank(
    name => 'Aberfoyle',
    country_code => 'GB',
    geo_type => 'CITY',
    parent_channel_id =>  $state3->{channel_id},
    city_list => $ns->namespace . "-"  . 'Aberfoyle',
    latitude => 56.1833,
    longitude => -4.3833));

  # Display
  my $display_keyword =  make_autotest_name($ns, "Display");

  my $display_channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'DisplayChannel',
    account_id => $account,
    keyword_list => $display_keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60 )] ));

  $self->create_display_campaign(
    $ns, 'CPM',
    { account => $account,
      channel => $display_channel,
      cpm => 20,
      geo_channels => $city1 });

  $self->create_display_campaign(
    $ns, 'CPC',
    { account => $account,
      channel => $display_channel,
      cpc => 3,
      geo_channels => [] });

  $self->create_display_campaign(
    $ns, 'CPA',
    { account => $account,
      channel => $display_channel,
      cpa => 4,
      geo_channels => $state1 });

  # Text campaigns
  my $size = $ns->create(CreativeSize => {
    name => "Text",
    max_text_creatives => 2 });

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'PublisherText',
       pubaccount_country_code => 'GB',
       pricedtag_adjustment => 1.0,
       size_id => $size });

  my $text_keyword = make_autotest_name($ns, "Text");
  my $channel_keyword = make_autotest_name($ns, "Channel");

  my $text_channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'TextChannel',
    account_id => $account,
    keyword_list => $text_keyword,
    channel_type => 'K',
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60 )] ));

  my $channel_channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'ChannelTAChannel',
    account_id => $account,
    keyword_list => $channel_keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 3*60*60 )] ));

  my $text_campaign = 
      $ns->create(TextAdvertisingCampaign => { 
        name => "TextCampaign",
        size_id => $size,
        template_id =>  DB::Defaults::instance()->text_template,
        country_code => 'GB',
        original_keyword => $text_keyword,
        ccgkeyword_channel_id => $text_channel,
        campaigncreativegroup_cpc => 5,
        campaigncreativegroup_ctr => 0.1,
        ccgkeyword_ctr => 0.1,
        max_cpc_bid => undef,
        campaigncreativegroup_geo_channels =>  $city1,
        site_links => [{site_id => $publisher->{site_id} }] });

  my $channel_campaign = 
    $ns->create(ChannelTargetedTACampaign => { 
      name => "ChannelCampaign",
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      country_code => 'GB',
      channel_id => $channel_channel,
      campaigncreativegroup_cpc => 6,
      campaigncreativegroup_ctr => 0.1,
      campaigncreativegroup_geo_channels =>  $city1,
      site_links => [{site_id => $publisher->{site_id} }] });

  $ns->output("CITY1CH", $city1);
  $ns->output("STATE1CH", $state1);
  $ns->output("CITY2CH", $city2);
  $ns->output("STATE2CH", $state2);
  $ns->output("CITY3CH", $city3);
  $ns->output("STATE3CH", $state3);
  $ns->output("CITY1", $city1->{name});
  $ns->output("STATE1", $state1->{name});
  $ns->output("CITY2", $city2->{name});
  $ns->output("STATE2", $state2->{name});
  $ns->output("CITY3", $city3->{name});
  $ns->output("STATE3", $state3->{name});


  $ns->output("DISPLAYCHANNEL", $display_channel);
  $ns->output("DISPLAYCH", $display_channel->page_key());
  $ns->output("DISPLAYKWD", $display_keyword);
  $ns->output("TEXTCH", $text_channel->page_key());
  $ns->output("TEXTKWD", $text_keyword);
  $ns->output("CHANNELCH", $channel_channel->page_key());
  $ns->output("CHANNELKWD", $channel_keyword);
  $ns->output("TEXTTID", $publisher->{tag_id});
  $ns->output("TEXTCC", $text_campaign->{cc_id});
  $ns->output("CHANNELCC", $channel_campaign->{cc_id});

  $ns->output("DISPLAYCPM", 20);
  $ns->output("DISPLAYCPC", 3);
  $ns->output("DISPLAYCPA", 4);
  $ns->output("TEXTCPC1", 5);
  $ns->output("TEXTCPC2", 6);

  $ns->output("DISPLAYSIZE", DB::Defaults::instance()->size());
  $ns->output("TEXTSIZE", $size);
}

1;
