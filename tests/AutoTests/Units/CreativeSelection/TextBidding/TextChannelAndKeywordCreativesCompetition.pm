

package TextChannelAndKeywordCreativesCompetition;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use lib "$FindBin::Dir/../Units/CreativeSelection/TextBidding";
use TextBiddingCommon;

our @ISA = qw(TextBiddingCommon);


sub init {
  my ($self, $ns) = @_;

  my $size = $ns->create(CreativeSize => { 
    name => "Size",
    max_text_creatives => 3 });

  my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => $size,
    pricedtag_cpm => 1 });

  my $textkeyword1 = make_autotest_name($ns, "text1");
  my $textkeyword1a = make_autotest_name($ns, "text1a");
  my $textkeyword1b = make_autotest_name($ns, "text1b");
  my $textkeyword1c = make_autotest_name($ns, "text1c");
  my $textkeyword2 = make_autotest_name($ns, "text2");
  my $textkeyword2a = make_autotest_name($ns, "text2a");
  my $textkeyword2b = make_autotest_name($ns, "text2b");
  my $channelkeyword1 = make_autotest_name($ns, "channel1");
  my $channelkeyword2 = make_autotest_name($ns, "channel2");
  my $channelkeyword3 = make_autotest_name($ns, "channel3");
  my $channelkeyword4 = make_autotest_name($ns, "channel4");
  my $channelkeyword5 = make_autotest_name($ns, "channel5");

  # Text CCGs
  my$text1 = $ns->create(TextAdvertisingCampaign => { 
    name => "Text1",
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $textkeyword1,
    max_cpc_bid => 0.2,
    ccgkeyword_ctr => 0.1,
    site_links => [{site_id => $publisher->{site_id} }] });

  $ns->create(CCGKeyword => {
    name => "Text1a",
    ccg_id => $text1->{ccg_id},
    original_keyword => $textkeyword1a,
    max_cpc_bid => 0.2,
    ctr => 0.1 });

  $ns->create(CCGKeyword => {
    name => "Text1b",
    ccg_id => $text1->{ccg_id},
    original_keyword => $textkeyword1b,
    max_cpc_bid => 0.2,
    ctr => 0.1 });

  $ns->create(CCGKeyword => {
    name => "Text1c",
    ccg_id => $text1->{ccg_id},
    original_keyword => $textkeyword1c,
    max_cpc_bid => 0.2,
    ctr => 0.1 });

  my$text2 = $ns->create(TextAdvertisingCampaign => { 
    name => "Text2",
    size_id => $size,
    template_id => DB::Defaults::instance()->text_template,
    original_keyword => $textkeyword2,
    max_cpc_bid => 0.12,
    ccgkeyword_ctr => 0.1,
    site_links => [{site_id => $publisher->{site_id} }] });

  $ns->create(CCGKeyword => {
    name => "Text2a",
    ccg_id => $text2->{ccg_id},
    original_keyword => $textkeyword2a,
    max_cpc_bid => 0.12,
    ctr => 0.1 });

  $ns->create(CCGKeyword => {
    name => "Text2b",
    ccg_id => $text2->{ccg_id},
    original_keyword => $textkeyword2b,
    max_cpc_bid => 0.12,
    ctr => 0.1 });

  # Channel targeted text CCGs
  my $channel1 = 
    $ns->create(ChannelTargetedTACampaign => { 
      name => "Channel1",
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      behavioralchannel_keyword_list => $channelkeyword1,
      campaigncreativegroup_cpm => 13,
      site_links => [
        {site_id => $publisher->{site_id}}]});

   $ns->create(DB::BehavioralChannel::BehavioralParameter->blank( 
     channel_id => $channel1->{channel_id},  
     trigger_type => "P" ));

  my $channel2 = 
    $ns->create(ChannelTargetedTACampaign => { 
      name => "Channel2",
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      behavioralchannel_keyword_list => $channelkeyword2,
      campaigncreativegroup_cpm => 8,
      site_links => [
        {site_id => $publisher->{site_id}}]});  

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank( 
    channel_id => $channel2->{channel_id},  
    trigger_type => "P" ));

  my $channel3 = 
    $ns->create(ChannelTargetedTACampaign => { 
      name => "Channel3",
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      behavioralchannel_keyword_list => $channelkeyword3,
      campaigncreativegroup_cpm => 20,
      site_links => [
        {site_id => $publisher->{site_id} }]});    

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank( 
    channel_id => $channel3->{channel_id},  
    trigger_type => "P" ));


  my $channel4 = 
    $ns->create(ChannelTargetedTACampaign => { 
      name => "Channel4",
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      behavioralchannel_keyword_list => $channelkeyword4,
      campaigncreativegroup_cpc => 2,
      campaigncreativegroup_ctr => 0.008,
      site_links => [
        {site_id => $publisher->{site_id} }]});    

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank( 
    channel_id => $channel4->{channel_id},  
    trigger_type => "P" ));


  my $channel5 = 
    $ns->create(ChannelTargetedTACampaign => { 
      name => "Channel5",
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      account_id => $channel4->{account_id},
      behavioralchannel_keyword_list => $channelkeyword5,
      campaigncreativegroup_cpc => 2,
      campaigncreativegroup_ctr => 0.1,
      site_links => [
        {site_id => $publisher->{site_id} }]});    

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank( 
    channel_id => $channel5->{channel_id},  
    trigger_type => "P" ));


  $ns->output("TEXTKEYWORD1", $textkeyword1);
  $ns->output("TEXTKEYWORD1a", $textkeyword1a);
  $ns->output("TEXTKEYWORD1b", $textkeyword1b);
  $ns->output("TEXTKEYWORD1c", $textkeyword1c);
  $ns->output("TEXTKEYWORD2", $textkeyword2);
  $ns->output("TEXTKEYWORD2a", $textkeyword2a);
  $ns->output("TEXTKEYWORD2b", $textkeyword2b);
  $ns->output("CHANNELKEYWORD1",  $channelkeyword1);
  $ns->output("CHANNELKEYWORD2",  $channelkeyword2);
  $ns->output("CHANNELKEYWORD3",  $channelkeyword3);
  $ns->output("CHANNELKEYWORD4",  $channelkeyword4);
  $ns->output("CHANNELKEYWORD5",  $channelkeyword5);
  $ns->output("TEXTCC1", $text1->{cc_id});
  $ns->output("TEXTCC2", $text2->{cc_id});
  $ns->output("CHANNELCC1", $channel1->{cc_id});
  $ns->output("CHANNELCC2", $channel2->{cc_id});
  $ns->output("CHANNELCC3", $channel3->{cc_id});
  $ns->output("CHANNELCC4", $channel4->{cc_id});
  $ns->output("CHANNELCC5", $channel5->{cc_id});
  $ns->output("TAG", $publisher->{tag_id});


  # https://confluence.ocslab.com/display/TDOCDRAFT/REQ-2849+Drop+to+the+Floor+Fix+2
  # 0.01 - top eCPM correction

  # Expected click & impressions revenue
  # Case1. 2 Text and one channel creatives
  $self->expected_revenue_output(
    $ns, 1,
    [0.13 + 0.01, 0, 0.12],
    [0, 0.012, 0]);
  # Case2. 2 Text on top and one channel.
  $self->expected_revenue_output(
    $ns, 2,
    [0.12 + 0.01, 0.12, 0],
    [0, 0, 0.008]);
  # Case3. Text(C) CCG ecpm > ccg keyword ecpm.
  $self->expected_revenue_output(
    $ns, 3,
    [0, 0.12],
    [0.013, 0]);
  # Case4. Text(C) CCG ecpm = ccg keyword ecpm.
  $self->expected_revenue_output(
    $ns, 4,
    [0.2, 0],
    [0, 0.02]);
  # Case5. Actual CPC persistence for Text(C) CCGs 
  # (Text(C) CCG ecpm < ccg keyword ecpm).
  $self->expected_revenue_output(
    $ns, 5,
    [2       # channel ccg#4 cpc
     * 0.008 # channel ccg#4 ctr 
     / 0.1 + 0.01, 2],
    [0, 0]);
  # Case6. Actual CPC persistence for Text(C) CCGs 
  # (Text(C) CCG ecpm > ccg keyword ecpm).
  $self->expected_revenue_output(
    $ns, 6,
    [2, 0.2],
    [0, 0]);
}

1;
