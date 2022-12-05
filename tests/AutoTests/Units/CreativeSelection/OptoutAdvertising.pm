
package OptoutAdvertising;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_display_campaign
{
  my ($self, $ns, $name, $args) = @_;
  my %args_copy = %$args;
  $args_copy{site_links} = [{name=> $name}];
  $args_copy{name} = $name;

  my $campaign = 
   $ns->create(DisplayCampaign => \%args_copy);

  my $tag = $ns->create(PricedTag => 
    { name => $name,
      site_id => $campaign->{Site}[0]->{site_id},
      cpm => 0
    });  


  $ns->output($name . "CC", $campaign->{cc_id});
  $ns->output($name . "TAG", $tag);
}

sub create_optin_status_targeting_campaigns
{
  my ($self, $ns) = @_;

  my $publisher1 = 
    $ns->create(Publisher => { 
      name => "Publisher-OST-1" });

  my $publisher2 = 
    $ns->create(Publisher => { 
      name => "Publisher-OST-2" });

  my @optin_statusses = (
    ['YNN', $publisher1 ],
    ['NYN', $publisher1 ],
    ['NNY', $publisher2 ],
    ['NYY', $publisher2 ],
    ['YNY', $publisher2 ],
    ['YYN', $publisher2 ],
    ['YYY', $publisher2 ]);
  
  foreach my $s (@optin_statusses)
  {
    my ($status, $publisher)  = @$s;
    my $keyword = make_autotest_name($ns, "KWD-" . $status);

    my $advertiser = $ns->create(Account => {
      name => 'Advertiser-OST-' . $status,
      role_id => DB::Defaults::instance()->advertiser_role,
      account_type_id => DB::Defaults::instance()->advertiser_type });

    my $campaign = $ns->create(DisplayCampaign => {
      name => 'Campaign-OST-' . $status,
      account_id => $advertiser,
      optin_status_targeting => $status,
      campaigncreativegroup_cpm => 100,
      channel_id => 
        DB::BehavioralChannel->blank(
          account_id => $advertiser,
          name => 'Channel-OST-' . $status,
          keyword_list => $keyword,
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ]),
      site_links => [{site_id => $publisher->{site_id}}] });

    $ns->output("OSTCC/" . $status, $campaign->{cc_id});
    $ns->output("OSTKWD/" . $status, $keyword);
  }

  my $ron = $ns->create(DisplayCampaign => {
    name => 'Campaign-OST-RON',
    channel_id => undef,
    campaigncreativegroup_cpm => 1,
    campaigncreativegroup_flags => 
      DB::Campaign::INCLUDE_SPECIFIC_SITES |
      DB::Campaign::RON,
    site_links => [{site_id => $publisher1->{site_id}}]    
  });

  $ns->output("OSTTAG1", $publisher1->{tag_id});
  $ns->output("OSTTAG2", $publisher2->{tag_id});
  $ns->output("OSTCC/RON", $ron->{cc_id});
   
}

sub init {
  my ($self, $ns) = @_;

  # Publisher
  my $publisher = $ns->create(Publisher => { 
    name => "Publisher" });

  my $publisherTA = $ns->create(Publisher => { 
    name => "PublisherTA" });

  # Advertisier
  my $advertiser = $ns->create(Account => {
    name => "Advertiser",
    role_id => DB::Defaults::instance()->advertiser_role });

  # Campaigns
  my $keyword = make_autotest_name($ns, "KWD");
  my $url = "http://www." . make_autotest_name($ns, "test.com/url");
  my $channel = $ns->create(DB::BehavioralChannel->blank(
    name => "URLChannel",
    url_list => $url,
    account_id => $advertiser,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        time_to => 60, trigger_type => 'U')]));


  # Test 6.1. RON ads matching
  $self->create_display_campaign(
    $ns, "DisplayRONCPM",
    { channel_id => undef,
      campaigncreativegroup_cpm => 2,
      campaigncreativegroup_flags => 
        DB::Campaign::INCLUDE_SPECIFIC_SITES |
        DB::Campaign::RON
      });

  # Test 6.2. Targeted ads matching on urls
  $self->create_display_campaign(
    $ns, "DisplayCPM",
    { channel_id => $channel,
      campaigncreativegroup_cpm => 20,
      campaigncreativegroup_flags => 
        DB::Campaign::INCLUDE_SPECIFIC_SITES
      });

  # Test 6.6.1. Campaigns with action tracking (RON)
  $self->create_display_campaign(
    $ns, "DisplayRONCPA",
    { channel_id => undef,
      campaigncreativegroup_cpm => 0,
      campaigncreativegroup_cpa => 400,
      campaigncreativegroup_ar => 0.01,
      campaigncreativegroup_flags => 
        DB::Campaign::INCLUDE_SPECIFIC_SITES |
        DB::Campaign::RON
      });

  # Test 6.6.2 Campaigns with action tracking
  $self->create_display_campaign(
    $ns, "DisplayCPA",
    { channel_id => $channel,
      campaigncreativegroup_cpm => 0,
      campaigncreativegroup_cpa => 500,
      campaigncreativegroup_ar => 0.01,
      campaigncreativegroup_flags => 
        DB::Campaign::INCLUDE_SPECIFIC_SITES,
      });

  # Test 6.7. Campaigns with impression/click tracking
  $self->create_display_campaign(
    $ns, "DisplayCPC",
    { channel_id => $channel,
      campaigncreativegroup_cpm => 0,
      campaigncreativegroup_cpc => 400,
      campaigncreativegroup_flags => 
        DB::Campaign::INCLUDE_SPECIFIC_SITES,
      });

  # Test 6.5. Campaigns with frequency caps
  $self->create_display_campaign(
    $ns, "DisplayFC",
    { channel_id => $channel,
      campaigncreativegroup_cpm => 60,
      campaigncreativegroup_freq_cap_id => 
        DB::FreqCap->blank(period => 60),
      campaigncreativegroup_flags => 
        DB::Campaign::INCLUDE_SPECIFIC_SITES,
      });

  my $size_id = $publisher->{Tags}->{size_id};

  # Test 6.10. Text(K) targeted campaigns
  my $ta_campaign =  $ns->create(TextAdvertisingCampaign => {
    name => "TA",
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $keyword,
    max_cpc_bid => 200,
    site_links => [{site_id => $publisherTA->{site_id}}] });

  # Test 6.8. Text RON campaigns
  my $text_ron = $ns->create(ChannelTargetedTACampaign => {
    name => "TextTargetedRON",
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    channel_id => undef,
     campaigncreativegroup_flags => 
        DB::Campaign::INCLUDE_SPECIFIC_SITES |
        DB::Campaign::RON,
    campaigncreativegroup_cpm => 1,
    site_links => [{site_id => $publisher->{site_id}}] });

  # Test 6.9. Text(C) targeted campaigns
  my $text_simple = $ns->create(ChannelTargetedTACampaign => {
    name => "TextTargetedSimple",
    size_id => $size_id,
    template_id => DB::Defaults::instance()->text_template,
    channel_id => $channel,
    campaigncreativegroup_cpm => 7,
    site_links => [{site_id => $publisher->{site_id}}] });

  $ns->output("OPTINCOLO", 
    DB::Defaults::instance()->optin_only_isp->{colo_id});
  $ns->output("ALLCOLO", 
    DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("NONOPTOUTCOLO", 
    DB::Defaults::instance()->non_optout_isp->{colo_id});
  $ns->output("NOADSCOLO", 
    DB::Defaults::instance()->no_ads_isp->{colo_id});
  $ns->output("DELETEDCOLO", 
    DB::Defaults::instance()->deleted_colo_isp->{colo_id});

  $ns->output("URL", $url);
  $ns->output("KEYWORD", $keyword);
  $ns->output("TEXTTAG", $publisher->{tag_id});
  $ns->output("TATAG", $publisherTA->{tag_id});
  $ns->output("TACC", $ta_campaign->{cc_id});
  $ns->output("TEXTRONCC", $text_ron->{cc_id});
  $ns->output("TEXTCC", $text_simple->{cc_id});

  $self->create_optin_status_targeting_campaigns($ns);

  print_NoAdvNoTrack($ns);
}

1;
