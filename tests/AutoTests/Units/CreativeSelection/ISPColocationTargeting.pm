
package ISPColocationTargeting;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $isp1 = DB::Defaults::instance()->ads_isp;

  my $isp2 = $ns->create(Isp => {
    name => "Colo-2",
    colocation_optout_serving => 'ALL',
    account_internal_account_id =>
      DB::Defaults::instance()->no_margin_internal_account->{account_id} });

  my $isp3 = $ns->create(Isp => {
    name => "Colo-3",
    colocation_optout_serving => 'ALL',
    account_internal_account_id =>
      DB::Defaults::instance()->no_margin_internal_account->{account_id} });
  
  my $isp_colo_deleted =  $ns->create(Isp => {
    name => "Deleted",
    colocation_optout_serving => 'ALL',
    colocation_status => 'D',
    account_internal_account_id =>
      DB::Defaults::instance()->no_margin_internal_account->{account_id} });

  my $size = $ns->create(CreativeSize => {
    name => 'Size',
    max_text_creatives => 2 });

  my $publisher = 
    $ns->create(Publisher => { 
      name => "Publisher",
      size_id => $size });

  # Display campaigns
  my %display_campaign = (
    'DISPLAY' => { cpm => 10, isps => [$isp1] },
    'SEVERALCOLOS1' => { cpm => 20, isps => [$isp1, $isp2], channel => 'SEVERALCOLOS' },
    'SEVERALCOLOS2' => { cpm => 10, isps => [], channel => 'SEVERALCOLOS' },
    'DELETEDCOLO' => { cpm => 10, isps => [$isp_colo_deleted] } );
  
  while (my ($k, $v) = each %display_campaign )
  {
    my $channel_name = $v->{channel}? $v->{channel}: $k;
    my $keyword = make_autotest_name($ns, "KWD-" . $channel_name);

    my $advertiser = $ns->create(Account => {
      name => 'Advertiser-' . $k,
      role_id => DB::Defaults::instance()->advertiser_role,
      account_type_id => DB::Defaults::instance()->advertiser_type });
    
    my @colos = map {$_->{colo_id}} (@{$v->{isps}});

    my $campaign = $ns->create(DisplayCampaign => {
      name => 'Campaign-' . $k,
      account_id => $advertiser,
      size_id => $size,
      campaigncreativegroup_cpm => $v->{cpm},
      colocations => \@colos,
      channel_id => 
        DB::BehavioralChannel->blank(
          account_id => $advertiser,
          name => 'Channel-' . $channel_name,
          keyword_list => $keyword,
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(
              trigger_type => 'P') ]),
      site_links => [{site_id => $publisher->{site_id}}] });

    $ns->output("CHANNEL/" . $channel_name, $campaign->{channel_id});
    $ns->output("CC/" . $k, $campaign->{cc_id});
    $ns->output("KWD/" . $channel_name, $keyword);
  }

  # TA campaigns

  my $ch_advertiser = $ns->create(Account => {
      name => 'Advertiser-TEXT-1',
      role_id => DB::Defaults::instance()->advertiser_role,
      account_type_id => DB::Defaults::instance()->advertiser_type });

  my $ch_keyword = make_autotest_name($ns, "KWD-TEXT-1");

  my $ch_campaign = $ns->create(
    ChannelTargetedTACampaign => {
      name => "TEXT-1",
      account_id => $ch_advertiser,
      size_id => $size,
      template_id => DB::Defaults::instance()->text_template,
      campaigncreativegroup_cpm => 200,
      campaigncreativegroup_ctr => 0.01,
      colocations => $isp1->{colo_id},
      channel_id =>  
         DB::BehavioralChannel->blank(
           name => 'Channel-TEXT-1',
           account_id => $ch_advertiser,
           keyword_list => $ch_keyword,
           behavioral_parameters => [
             DB::BehavioralChannel::BehavioralParameter->blank(
               trigger_type => 'P')]),
      site_links => [{site_id => $publisher->{site_id} }] });
                                 
  my $ta_advertiser = $ns->create(Account => {
      name => 'Advertiser-TEXT-2',
      role_id => DB::Defaults::instance()->advertiser_role,
      account_type_id => DB::Defaults::instance()->advertiser_type });

  my $ta_keyword = make_autotest_name($ns, "KWD-TEXT-2");

  my $ta_channel = $ns->create(
    DB::BehavioralChannel->blank(
      name => 'Channel-TEXT-2',
      account_id => $ta_advertiser,
      keyword_list => $ta_keyword,
      channel_type => 'K',
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type => 'P')]));

  my $ta_campaign = $ns->create(
   TextAdvertisingCampaign => {
      name => "TEXT-2",
      account_id => $ta_advertiser,
      size_id => $size,
      template_id => DB::Defaults::instance()->text_template,
      max_cpc_bid => 1,
      colocations => $isp1->{colo_id},
      original_keyword => $ta_keyword,
      campaigncreativegroup_ctr => 0.01,
      ccgkeyword_channel_id => $ta_channel->{channel_id},
      site_links => [{site_id => $publisher->{site_id} }] });


  foreach my $template (DB::Defaults::instance()->display_template(), 
                        DB::Defaults::instance()->text_template)
  {
    $ns->create(TemplateFile => {
      template_id => $template,
      size_id => $size,
      template_file => 'UnitTests/textad.xsl',
      template_type => 'X',
      flags => DB::TemplateFile::PIXEL_TRACKING,
      app_format_id => DB::Defaults::instance()->app_format_track});

    $ns->create(TemplateFile => {
      template_id => $template,
      size_id => $size,
      template_file => 'UnitTests/textad.xsl',
      template_type => 'X',
      app_format_id => DB::Defaults::instance()->app_format_no_track});
  }

  $ns->output("CHANNEL/TEXT/1", $ch_campaign->{channel_id});
  $ns->output("CC/TEXT/1", $ch_campaign->{cc_id});
  $ns->output("KWD/TEXT/1", $ch_keyword);

  $ns->output("CHANNEL/TEXT/2", $ta_channel);
  $ns->output("CC/TEXT/2", $ta_campaign->{cc_id});
  $ns->output("KWD/TEXT/2", $ta_keyword);

  $ns->output("TAG", $publisher->{tag_id});
  $ns->output("COLO/1", $isp1->{colo_id});
  $ns->output("COLO/2", $isp2->{colo_id});
  $ns->output("COLO/3", $isp3->{colo_id});
  $ns->output("COLO/DELETED", $isp_colo_deleted->{colo_id});

}

1;
