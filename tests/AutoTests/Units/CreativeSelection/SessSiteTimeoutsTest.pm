package SessSiteTimeoutsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_data
{
  my ($self, $ns, $suffix, $timeout, $ron) = @_;

  my $advertiser = $ns->create(Account => {
    name => "Advertiser" . $suffix,
    role_id => DB::Defaults::instance()->advertiser_role });

  my $ron_flag = defined $ron? $ron: 0;

  my $channel;
  if (!$ron_flag)
  {
    my $keyword = make_autotest_name($ns, $suffix);
    
    $channel = $ns->create(DB::BehavioralChannel->blank(
    name => 'Channel' . $suffix,
    keyword_list => $keyword,
    account_id => $advertiser,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => 'P')]));

    $ns->output("Keyword/" . $suffix, $keyword);
  }

  my $publisher = $ns->create(Publisher => {
    name => 'Publisher' . $suffix,
    site_no_ads_timeout => $timeout });

  my $campaign = $ns->create(DisplayCampaign => {
    name => 'Campaign' . $suffix,
    account_id => $advertiser,
    channel_id => $channel,
    campaigncreativegroup_flags => 
      $ron_flag? DB::Campaign::INCLUDE_SPECIFIC_SITES:
        DB::Campaign::INCLUDE_SPECIFIC_SITES | DB::Campaign::RON,
    site_links => [
      { site_id => $publisher->{site_id} }] });

  $ns->output("CC/" . $suffix, $campaign->{cc_id});
  $ns->output("Tag/" . $suffix, $publisher->{tag_id} );
}

sub create_text_data
{
  my ($self, $ns) = @_;

  my $keyword = make_autotest_name($ns, 'Text');
  
  my $size = $ns->create(CreativeSize =>
                            { name => 'SizeText',
                              max_text_creatives => 1 });

  my $publisher = $ns->create(Publisher => {
    name => 'PublisherText',
    pricedtag_size_id => $size,
    site_no_ads_timeout => 40 });

  my $campaign = $ns->create(TextAdvertisingCampaign => {
    name => 'CampaignText',
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $keyword,
    max_cpc_bid => 100,
    site_links => [
      { site_id => $publisher->{site_id} }] });

  $ns->output("Keyword/Text", $keyword);
  $ns->output("CC/Text", $campaign->{cc_id});
  $ns->output("Tag/Text", $publisher->{tag_id} );
}

sub init
{
  my ($self, $ns) = @_;

  $self->create_data($ns, 'NOADS', 30);
  $self->create_data($ns, 'ZEROTIMEOUT', 0);
  $self->create_data($ns, 'RON', 40, 1);
  $self->create_data($ns, 'MERGE', 10);
  $self->create_text_data($ns);

  print_NoAdvNoTrack($ns);
}

1;
