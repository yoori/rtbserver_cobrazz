package ActionStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use encoding 'utf8';


sub basic_case
{
  my ($self, $prefix, $namespace) = @_;

  my $ns = $namespace->sub_namespace($prefix);

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $keyword = make_autotest_name($ns, "keyword");
  my $url = "http://" . make_autotest_name($ns, "url") . ".com";

  my $action1 = $ns->create(Action => {
    name => 1,
    url => "http:\\www.ebay.com",
    account_id => $advertiser});

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 1,
    account_id => $advertiser,
    campaigncreativegroup_country_code => 'US',
    campaigncreativegroup_action_id => $action1,
    campaigncreativegroup_cpa => 1,
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 1 }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel1',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P')])
    });

  my $action2 = $ns->create(Action => {
    name => 2,
    url => "http:\\www.ebay.com",
    account_id => $advertiser});

  my $action3 = $ns->create(Action => {
    name => 3,
    account_id => $advertiser});

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 2,
    account_id => $advertiser,
    campaigncreativegroup_country_code => 'RU',
    campaigncreativegroup_action_id => 
      [$action2, $action3],
    campaigncreativegroup_cpa => 1,
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    creative_id => $campaign1->{creative_id},
    site_links => [{name => 2 }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel2',
      account_id => $advertiser,
      url_list => $url,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'U')])
    });

  my $action4 = $ns->create(Action => {
    name => 4,
    url => "http:\\www.ebay.com",
    account_id => $advertiser});

  my $action5 = $ns->create(Action => {
    name => 5,
    url => "http:\\www.ebay.com",
    account_id => $advertiser});

  my $campaign3 = $ns->create(DisplayCampaign => {
    name => 3,
    account_id => $advertiser,
    campaigncreativegroup_country_code => 'US',
    campaigncreativegroup_action_id => $action5,
    campaigncreativegroup_cpa => 1,
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 3 }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel3',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P')])
    });

  my $action6 = $ns->create(Action => {
    name => 6,
    url => "http:\\www.ebay.com",
    account_id => $advertiser});

  my $campaign4 = $ns->create(DisplayCampaign => {
    name => 4,
    account_id => $advertiser,
    campaigncreativegroup_country_code => 'US',
    campaigncreativegroup_action_id => $action6,
    campaigncreativegroup_cpa => 1,
    campaigncreativegroup_ar => 0.01,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 4 }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel4',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P')])
    });

  my $tag1 = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign1->{Site}[0]->{site_id} });

  my $tag2 = $ns->create(PricedTag => {
    name => 2,
    site_id => $campaign2->{Site}[0]->{site_id} });

  my $tag3 = $ns->create(PricedTag => {
    name => 3,
    site_id => $campaign3->{Site}[0]->{site_id} });

  my $tag4 = $ns->create(PricedTag => {
    name => 4,
    site_id => $campaign4->{Site}[0]->{site_id} });


  $ns->output("CCID/1", $campaign1->{cc_id});
  $ns->output("CCID/2", $campaign2->{cc_id});
  $ns->output("CCID/3", $campaign3->{cc_id});
  $ns->output("CCID/4", $campaign4->{cc_id});
  $ns->output("TID/1", $tag1);
  $ns->output("TID/2", $tag2);
  $ns->output("TID/3", $tag3);
  $ns->output("TID/4", $tag4);
  $ns->output("KEYWORD", $keyword);
  $ns->output("URL", $url);
  $ns->output("ACTION/1", $action1);
  $ns->output("ACTION/2", $action2);
  $ns->output("ACTION/3", $action3);
  $ns->output("ACTION/4", $action4);
  $ns->output("ACTION/5", $action5);
  $ns->output("ACTION/6", $action6);
}

sub cross_action
{
  my ($self, $prefix, $namespace) = @_;

  my $ns = $namespace->sub_namespace($prefix);

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $template = $ns->create(Template =>
    { name => "Template" } );

  my $keyword1 = make_autotest_name($ns, "keyword1");
  my $keyword2 = make_autotest_name($ns, "keyword2");

  my $action1 = $ns->create(Action => {
    name => 'Action1',
    account_id => $advertiser});

  my $action2 = $ns->create(Action => {
    name => 'Action2',
    account_id => $advertiser});

  my $size1 = $ns->create(CreativeSize => {
    name => "Size1" });

  my $size2 = $ns->create(CreativeSize => {
    name => "Size2" });

  my $size3 = $ns->create(CreativeSize => {
    name => "Size3" });

  for my $size ($size1, $size2, $size3)
  {
    $ns->create(TemplateFile =>
      { template_id => $template,
        size_id => $size,
        template_file =>
          'UnitTests/banner_img_clk.html',
        flags => 0 });
  }

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'Publisher',
       size_id => $size1 });

  my $tag2 = $ns->create(PricedTag => {
    name => 'Tag2',
    size_id => $size2,
    site_id => $publisher->{site_id} });

  my $tag3 = $ns->create(PricedTag => {
    name => 'Tag3',
    size_id => $size3,
    site_id => $publisher->{site_id} });

 my $campaign1 = $ns->create(DisplayCampaign => {
    name => "Campaign1",
    account_id => $advertiser,
    campaigncreativegroup_action_id => 
      [$action1, $action2],
    size_id => $size2,
    template_id => $template,
    campaigncreativegroup_cpm => 10,
    site_links => [{ site_id => $publisher->{site_id},
                     size_id => $size2 }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel1',
      account_id => $advertiser,
      keyword_list => $keyword1,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60 )])
    });

  my $creative = $ns->create(Creative => 
    {  name => 'Creative1_2',
       account_id => $advertiser,
       size_id => $size3,                   
       template_id => $template });

  my $cc1_2 =  $ns->create(CampaignCreative =>
   { ccg_id => $campaign1->{ccg_id},
     creative_id => $creative });


  my $campaign2 = $ns->create(DisplayCampaign => {
    name => "Campaign2",
    account_id => $advertiser,
    campaigncreativegroup_action_id => $action2,
    size_id => $size1,
    template_id => $template,
    campaigncreativegroup_cpm => 10,
    site_links => [{ site_id => $publisher->{site_id},
                     size_id => $size1 }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel2',
      account_id => $advertiser,
      keyword_list => $keyword2,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60)])
    });
  
  $ns->output("CCID/1_1", $campaign1->{cc_id});
  $ns->output("CCID/1_2", $cc1_2->{cc_id});
  $ns->output("CCID/2", $campaign2->{cc_id});
  $ns->output("TID/1", $publisher->{tag_id});
  $ns->output("TID/2", $tag2);
  $ns->output("TID/3", $tag3);
  $ns->output("KEYWORD/1", $keyword1);
  $ns->output("KEYWORD/2", $keyword2);
  $ns->output("ACTION/1", $action1);
  $ns->output("ACTION/2", $action2);
}

sub imp_update
{
  my ($self, $prefix, $namespace) = @_;

  my $ns = $namespace->sub_namespace($prefix);

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $keyword = make_autotest_name($ns, "keyword");

  my $publisher1 = 
     $ns->create(Publisher => { 
       name => 'Publisher1' });

  my $publisher2 = 
     $ns->create(Publisher => { 
       name => 'Publisher2' });

  my $action = $ns->create(Action => {
    name => 'Action',
    account_id => $advertiser});

  my $campaign = $ns->create(DisplayCampaign => {
    name => "Campaign",
    account_id => $advertiser,
    campaigncreativegroup_action_id => $action,
    campaigncreativegroup_cpm => 10,
    site_links => 
      [ 
        { site_id => $publisher1->{site_id} },  
        { site_id => $publisher2->{site_id} },  
      ],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60 )])
    });

  $ns->output("CCID", $campaign->{cc_id});
  $ns->output("COLO", DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("TID/1", $publisher1->{tag_id});
  $ns->output("TID/2", $publisher2->{tag_id});
  $ns->output("KEYWORD", $keyword);
  $ns->output("ACTION", $action);
}

sub text_ads
{
  my ($self, $prefix, $namespace) = @_;

  my $ns = $namespace->sub_namespace($prefix);

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $size = $ns->create(CreativeSize => {
    name => 'Size',
    max_text_creatives => 2});

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'Publisher',
       size_id => $size });

  my $action = $ns->create(Action => {
    name => 'Action',
    account_id => $advertiser});

  my $keyword = make_autotest_name($ns, "keyword");

  my $campaign1 = $ns->create(TextAdvertisingCampaign => {
    name => 'Campaign1',
    template_id => DB::Defaults::instance()->text_template,
    size_id => $size,
    original_keyword => $keyword,
    max_cpc_bid => 20,
    campaigncreativegroup_action_id => $action,
    site_links => [{ site_id => $publisher->{site_id} }]  
  });

  my $campaign2 = $ns->create(TextAdvertisingCampaign => {
    name => 'Campaign2',
    template_id => DB::Defaults::instance()->text_template,
    size_id => $size,
    original_keyword => $keyword,
    max_cpc_bid => 10,
    campaigncreativegroup_action_id => $action,
    site_links => [{ site_id => $publisher->{site_id} }]  
  });

  $ns->output("CCID/1", $campaign1->{cc_id});
  $ns->output("CCID/2", $campaign2->{cc_id});
  $ns->output("TID", $publisher->{tag_id});
  $ns->output("KEYWORD", $keyword);
  $ns->output("ACTION", $action);  
}

sub deleted_action
{
  my ($self, $prefix, $namespace) = @_;

  my $ns = $namespace->sub_namespace($prefix);

  my $keyword = make_autotest_name($ns, "keyword");

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $action = $ns->create(Action => {
    name => 'Action',
    account_id => $advertiser,
    status => 'D'});

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'Publisher' });

  my $campaign = $ns->create(DisplayCampaign => {
    name => "Campaign",
    account_id => $advertiser,
    campaigncreativegroup_action_id => [$action],
    campaigncreativegroup_cpm => 10,
    site_links => [{site_id => $publisher->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60 )]) }); 

  $ns->output("CCID", $campaign->{cc_id});
  $ns->output("COLO", DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("TID", $publisher->{tag_id});
  $ns->output("KEYWORD", $keyword);
  $ns->output("ACTION", $action);
}

sub expired_profile
{
  my ($self, $prefix, $namespace) = @_;

  my $ns = $namespace->sub_namespace($prefix);

  my $keyword1 = make_autotest_name($ns, "keyword1");
  my $keyword2 = make_autotest_name($ns, "keyword2");

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $action = $ns->create(Action => {
    name => 'Action',
    account_id => $advertiser});

  my $publisher1 = 
     $ns->create(Publisher => { 
       name => 'Publisher1' });

  my $publisher2 = 
     $ns->create(Publisher => { 
       name => 'Publisher2' });

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => "Campaign1",
    account_id => $advertiser,
    campaigncreativegroup_action_id => [$action],
    campaigncreativegroup_cpm => 10,
    site_links => [{site_id => $publisher1->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel1',
      account_id => $advertiser,
      keyword_list => $keyword1,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60 )]) }); 

  my $campaign2= $ns->create(DisplayCampaign => {
    name => "Campaign2",
    account_id => $advertiser,
    campaigncreativegroup_action_id => [$action],
    campaigncreativegroup_cpm => 10,
    site_links => [{site_id => $publisher2->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel2',
      account_id => $advertiser,
      keyword_list => $keyword2,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60 )]) }); 

  $ns->output("CCID/1", $campaign1->{cc_id});
  $ns->output("CCID/2", $campaign2->{cc_id});
  $ns->output("TID/1", $publisher1->{tag_id});
  $ns->output("TID/2", $publisher2->{tag_id});
  $ns->output("KEYWORD/1", $keyword1);
  $ns->output("KEYWORD/2", $keyword2);
  $ns->output("COLO", DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("ACTION", $action);
}

sub conversion_service
{
  my ($self, $prefix, $namespace, @values) = @_;

  my $ns = $namespace->sub_namespace($prefix);

  my $keyword = make_autotest_name($ns, "keyword");

  my $advertiser = $ns->create(Advertiser => {
    name => 'Advertiser' });

  my $publisher = 
     $ns->create(Publisher => { 
       name => 'Publisher' });

  my @actions = ();

  my $index = 0;

  for my $val (@values)
  {
    my $action = $ns->create(Action => {
      name => "Action" . ++$index,
      account_id => $advertiser,
      cur_value => $val});
    
    $ns->output("ACTION/" . $index, $action);
    push @actions, $action;
  }

  my $campaign= $ns->create(DisplayCampaign => {
    name => "Campaign",
    account_id => $advertiser,
    campaigncreativegroup_action_id => \@actions,
    campaigncreativegroup_cpm => 10,
    site_links => [{site_id => $publisher->{site_id} }],
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P',
          time_to => 3*60*60 )]) }); 

  $ns->output("CCID", $campaign->{cc_id});
  $ns->output("TID", $publisher->{tag_id});
  $ns->output("KEYWORD", $keyword);

  $ns->output("ORDER/1", qq[\xe8\x9e\x8d\xe8\xb5\x84\xe4\xb8\x9a\xe5\x8a\xa1]);
  $ns->output("ORDER/2", qq[\xd0\xb9\xd1\x86\xd1\x83\xd0\xba\xd0\xb5\xd0\xbd]);
}

sub init
{
  my ($self, $ns) = @_;
  $self->basic_case('Base', $ns);
  $self->cross_action('CrossAction', $ns);
  $self->imp_update('ImpUpdate', $ns);
  $self->text_ads('TextAds', $ns);
  $self->deleted_action('Deleted', $ns);
  $self->expired_profile('Expired', $ns);
  $self->conversion_service('Conversation', $ns, (0.0, 0.0, 3.7, 2.3, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 4.0));
  $self->conversion_service('Referrer',     $ns, (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
}

1;

