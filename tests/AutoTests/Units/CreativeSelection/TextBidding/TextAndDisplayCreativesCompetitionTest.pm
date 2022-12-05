package TextAndDisplayCreativesCompetitionTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use lib "$FindBin::Dir/../Units/CreativeSelection/TextBidding";
use TextBiddingCommon;

our @ISA = qw(TextBiddingCommon);

sub create_text_tgt_test_tuple
{
   my ($self, $ns, $index, $params) = @_;
   my $keyword = make_autotest_name($ns, "keyword_". $index);
   $ns->output("Keyword_". $index, $keyword);

   my $display_campaign = $ns->create(DisplayCampaign => { 
     name => "Display-" . $index,
     campaigncreativegroup_cpm => $params->{cpm},
     campaigncreativegroup_cpc => $params->{cpc},
     campaigncreativegroup_cpa => $params->{cpa},
     behavioralchannel_keyword_list => $keyword,
     campaigncreativegroup_flags => 
         DB::Campaign::INCLUDE_SPECIFIC_SITES,
     creative_size_id => $params->{size_id},
     creative_template_id =>  DB::Defaults::instance()->text_template,
     site_links => [ { site_id => $params->{site_id} }] });

   $ns->create(
     DB::BehavioralChannel::BehavioralParameter->blank(
       channel_id => $display_campaign->{channel_id},  
       trigger_type => "P" ));

   my $text_idx = 0;
   foreach my $cpc_bid (@{$params->{cpc_bids}}) {
     my $text_campaign = 
         $ns->create(TextAdvertisingCampaign => { 
           name => "TA-".$index . "-" . ++$text_idx,
        size_id => $params->{size_id},
        template_id =>  DB::Defaults::instance()->text_template,
        original_keyword => $keyword,
        max_cpc_bid => $cpc_bid,
        site_links => [
          {site_id => $params->{site_id} }] });
     $ns->output(
       "TEXTCC".$index . "_" . $text_idx, 
       $text_campaign->{cc_id});
   }
   
   $ns->output("KEYWORD_" . $index,  $keyword);
   $ns->output("DISPLAYCC_" . $index, $display_campaign->{cc_id});
}

sub create_channel_tgt_test_tuple
{
   my ($self, $ns, $index, $params) = @_;
   my $keyword = make_autotest_name($ns, "keyword_". $index);
   $ns->output("Keyword_". $index, $keyword);

   my $display_campaign = $ns->create(DisplayCampaign => { 
     name => "Display-" . $index,
     campaigncreativegroup_cpm => $params->{cpm},
     campaigncreativegroup_cpc => $params->{cpc},
     campaigncreativegroup_cpa => $params->{cpa},
     behavioralchannel_keyword_list => $keyword,
     campaigncreativegroup_flags => 
         DB::Campaign::INCLUDE_SPECIFIC_SITES,
     creative_size_id => $params->{size_id},
     creative_template_id =>  DB::Defaults::instance()->text_template,
     site_links => [ {site_id => $params->{site_id}}] });

   $ns->create(
     DB::BehavioralChannel::BehavioralParameter->blank(
       channel_id => $display_campaign->{channel_id},  
       trigger_type => "P" ));

   my $text_idx = 0;
   foreach my $cpm (@{$params->{cpms}}) {
     my $text_campaign = 
         $ns->create(ChannelTargetedTACampaign => { 
           name => "ChannelTargeted-".$index . "-" . ++$text_idx,
        size_id => $params->{size_id},
        template_id =>  DB::Defaults::instance()->text_template,
        channel_id => $display_campaign->{channel_id},
        campaigncreativegroup_cpm => $cpm,
        site_links => [
          {site_id => $params->{site_id} }] });
     $ns->output(
       "CHANNELTGTCC".$index . "_" . $text_idx, 
       $text_campaign->{cc_id});
   }
    
   $ns->output("KEYWORD_" . $index,  $keyword);
   $ns->output("DISPLAYCC_" . $index, $display_campaign->{cc_id});
}


sub init
{
    my ($self, $ns) = @_;

    my $size = $ns->create(CreativeSize => { 
      name => "Size",
      max_text_creatives => 3 });


    my $foros_min_margin = get_config($ns->pq_dbh, 'OIX_MIN_MARGIN') / 100.00;
    my $foros_min_fixed_margin = get_config($ns->pq_dbh, 'OIX_MIN_FIXED_MARGIN');

    my $tag_cpm = 4.0;
    my $display_cpm = money_upscale(3.0 * $tag_cpm);

   my $publisher = $ns->create(Publisher => {
    name => "Publisher",
    pricedtag_size_id => $size,
    pricedtag_cpm => $tag_cpm });

    $self->create_text_tgt_test_tuple(
      $ns, 1, 
      { cpm => $display_cpm,
        size_id =>  $size,
        site_id => $publisher->{site_id},
        cpc_bids => 
          [money_upscale(0.001 * $tag_cpm, 3),
           money_upscale(0.0014 * $tag_cpm, 3)] });

    $self->create_text_tgt_test_tuple(
      $ns, 2, 
      { cpm => $display_cpm,
        size_id =>  $size,
        site_id => $publisher->{site_id},
        cpc_bids => 
          [money_upscale(16 * $tag_cpm, 3),
           money_upscale(0.0017 * $tag_cpm, 3)] });

    $self->create_channel_tgt_test_tuple(
      $ns, 3, 
      { cpm => 10,
        size_id =>  $size,
        site_id => $publisher->{site_id},
        cpms => [10] });

    $self->create_channel_tgt_test_tuple(
      $ns, 4, 
      { cpm => 24,
        size_id =>  $size,
        site_id => $publisher->{site_id},
        cpms => [10, 9, 8, 7] });

    $self->create_channel_tgt_test_tuple(
      $ns, 5, 
      { cpm => 30,
        size_id =>  $size,
        site_id => $publisher->{site_id},
        cpms => [10, 9, 8, 7] });

  # Expected click & impressions revenue   
  # Case3. Text(C) CCG ecpm = Display CCG ecpm.
  $self->expected_revenue_output(
    $ns, 3,
    [0],
    [0.01]);

  # Case4. Text(C) CCG ecpm > Display CCG ecpm.
  $self->expected_revenue_output(
    $ns, 4,
    [0, 0, 0],
    [0.01, 0.009, 0.008]);

  # Case5. Text(C) CCG ecpm < Display CCG ecpm .
  $self->expected_revenue_output(
    $ns, 5,
    [0],
    [0.03]);


   $ns->output("TAG", $publisher->{tag_id});  
}

1;
