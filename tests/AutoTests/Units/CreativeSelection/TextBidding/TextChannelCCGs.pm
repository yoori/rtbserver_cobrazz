
package TextChannelCCGs;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use lib "$FindBin::Dir/../Units/CreativeSelection/TextBidding";
use TextBiddingCommon;

our @ISA = qw(TextBiddingCommon);

sub create_channnel_text_ccg
{
  my ($self, $ns, $index, $size, $publishers, $cpm) = @_;
  my $keyword = make_autotest_name($ns, "Channel" . $index);

  my @site_links = ();
  foreach my $publisher (@$publishers) {
    push(@site_links, 
         {site_id => $publisher->{site_id}});
  }
  my $campaign = 
    $ns->create(ChannelTargetedTACampaign => { 
      name => "Channel" . $index,
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      behavioralchannel_keyword_list => $keyword,
      campaigncreativegroup_cpm => $cpm,
      site_links => \@site_links });

  $ns->create(DB::BehavioralChannel::BehavioralParameter->blank( 
     channel_id => $campaign->{channel_id},  
     trigger_type => "P" ));

  $ns->output("CHANNELKEYWORD" .$index, $keyword);
  $ns->output("CHANNELCC". $index, $campaign->{cc_id});

  return $campaign;
}

sub create_keyword_text_ccg {
  my ($self, $ns, $index, $size, $publisher, $account, $cpc_bid) = @_;

  my $keyword = make_autotest_name($ns, "Keyword" . $index);
  my $campaign = 
    $ns->create(TextAdvertisingCampaign => { 
      name => "Keyword" . $index,
      size_id => $size,
      template_id =>  DB::Defaults::instance()->text_template,
      account_id => $account,
      original_keyword => $keyword,
      ccgkeyword_ctr => 0.1,
      max_cpc_bid => $cpc_bid,
      site_links => [
        {site_id => $publisher->{site_id} }] });
  $ns->output("KWDKEYWORD" .$index, $keyword);
  $ns->output("KWDCC".$index,  $campaign->{cc_id});
}


sub init {
  my ($self, $ns) = @_;
  # Remove this comment and insert your code here

  my $size = $ns->create(CreativeSize => { 
    name => "Size",
    max_text_creatives => 3 });


  my $publisher_simple = $ns->create(Publisher => {
    name => "PublisherSimple",
    pricedtag_size_id => $size,
    pricedtag_cpm => 1 });


  my $publisher_no_margin = $ns->create(Publisher => {
    name => "PublisherNoMargin",
    pubaccount_internal_account_id =>
      DB::Defaults::instance()->no_margin_internal_account,
    pricedtag_cpm => 0 });

  my $index = 0;
  foreach my $cpm (10, 9, 8, 7) {
    $self->create_channnel_text_ccg(
      $ns, ++$index, $size, 
      [$publisher_simple, 
       $publisher_no_margin], 
      $cpm);
  }

  my @campaigns = ();
  foreach my $cpm (11.12, 9) {
    push (
      @campaigns, 
      $self->create_channnel_text_ccg(
        $ns, ++$index, $size, 
        [$publisher_simple], $cpm));
    
  }

  $index = 0;
  foreach my $cpc_bid (0.09, 0.12) {
    $self->create_keyword_text_ccg(
      $ns, ++$index, $size, 
      $publisher_simple, 
      $campaigns[$index-1]->{account_id},
      $cpc_bid);
  }

  $ns->output("TAGSIMPLE", $publisher_simple->{tag_id});
  $ns->output("TAGNOMARGIN", $publisher_no_margin->{tag_id});
  $ns->output("COLONOMARGIN", 
    DB::Defaults::instance()->no_margin_isp->{colo_id});

  # Expected click & impressions revenue
  # Case1. One text creative served.
  # Case2. Channel targeted CCG on tag with CPM=0 and margin=0.
  $self->expected_revenue_output(
    $ns, 1,
    [0],
    [0.01]);

  # Case3. Three text creatives served (four available).
  # Case4. Three text creatives served (three available).
  $self->expected_revenue_output(
    $ns, 3,
    [0, 0, 0, 0],
    [0.01, 0.009, 0.008, 0.007]);

  # Case5. Keyword and Channel Text CCGs from the same account.
    $self->expected_revenue_output(
    $ns, 5,
    [0.12, 0],
    [0, 0.01112]);
}

1;
