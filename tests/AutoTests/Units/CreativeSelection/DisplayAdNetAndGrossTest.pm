
package DisplayAdNetAndGrossTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init_case
{
  my ($self, $ns, $name_prefix, $tag_cur, $cpm1, $cpm2, $perc1, $perc2) = @_;

  my $keyword = make_autotest_name($ns, $name_prefix);

  $ns->output("Keyword/".$name_prefix, $keyword);

  # GROSS campaign
  my $campaign1 = $ns->create(DisplayCampaign => {
    name => $name_prefix."_1",
    advertiser_agency_account_id => 
       DB::Defaults::instance()->agency_gross(),
    currency_id => $tag_cur,
    campaign_commission => $perc1,
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_cpm => $cpm1,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => $name_prefix}],
    });

  $ns->output("CC Id/$name_prefix/1", $campaign1->{cc_id}, "cc_id 1");

  # Net concurent campaign
  my $campaign2 = $ns->create(DisplayCampaign => {
    name => $name_prefix."_2",
    currency_id => $tag_cur,
    campaign_commission => $perc2,
    channel_id => $campaign1->{channel_id},
    campaigncreativegroup_cpm => $cpm2,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{site_id => $campaign1->{Site}[0]->{site_id}}],
    });
  
  $ns->output("CC Id/$name_prefix/2", $campaign2->{cc_id}, "cc_id 2");

  my $bp = $ns->create(
     DB::BehavioralChannel::BehavioralParameter->blank(
       channel_id => $campaign1->{channel_id},
       trigger_type => "P" ));

  my $tag_id = $ns->create(PricedTag => {
    name => $name_prefix,
    site_id => $campaign1->{Site}[0]->{site_id},
    cpm => 3 });

  $ns->output("Tag Id/$name_prefix", $tag_id, "tag_id");
  $ns->output("PubAcc Id/$name_prefix", $campaign1->{Site}[0]->{account_id}, "acc_id");
}

sub init_site_case
{
  my ($self, $ns, $name_prefix, $tag_cur, $cpmx, $cpm1, $cpm2, $percx, $percs) = @_;

  my $keyword = make_autotest_name($ns, $name_prefix);

  $ns->output("Keyword/".$name_prefix, $keyword);

  my $site_acc = $ns->create(Account => {
    name => "SiteAcc".$name_prefix,
    commission => $percs,
    currency_id => $tag_cur,
    role_id => DB::Defaults::instance()->publisher_role});

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => $name_prefix,
    advertiser_agency_account_id => 
       DB::Defaults::instance()->agency_gross(),
    currency_id => $tag_cur,
    campaign_commission => $percx,
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_cpm => $cpmx,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => $name_prefix, account_id => $site_acc}],
    });

  $ns->output("CC Id/$name_prefix", $campaign1->{cc_id}, "cc_id 1");

  my $bp = $ns->create(
     DB::BehavioralChannel::BehavioralParameter->blank(
       channel_id => $campaign1->{channel_id},
       trigger_type => "P" ));

  my $tag_id1 = $ns->create(PricedTag => {
    name => $name_prefix."-1",
    site_id => $campaign1->{Site}[0]->{site_id},
    cpm => $cpm1 });

  $ns->output("Tag Id/$name_prefix/1", $tag_id1, "tag_id");
  my $tag_id2 = $ns->create(PricedTag => {
    name => $name_prefix."-2",
    site_id => $campaign1->{Site}[0]->{site_id},
    cpm => $cpm2 });
  $ns->output("Tag Id/$name_prefix/2", $tag_id2, "tag_id");
  $ns->output("PubAcc Id/$name_prefix", $campaign1->{Site}[0]->{account_id}, "acc_id");
}

sub init
{
  my ($self, $ns) = @_;

  my $tag_cur = $ns->create(Currency => { rate => 2 });

  $self->init_case($ns, "1", $tag_cur, 10, 8, 0.3, 0);
  $self->init_case($ns, "2", $tag_cur, 12, 8, 0.3, 0);
  $self->init_case($ns, "3", $tag_cur, 10, 8, 0.3, 0.3);
  $self->init_site_case($ns, "4", $tag_cur, 2.8, 3, 3.4, 0.3, 0.4);
}

1;
