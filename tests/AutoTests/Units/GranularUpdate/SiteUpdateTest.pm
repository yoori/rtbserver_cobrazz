# $Id$
# Creates DB entites required for SiteUpdateTest.
package SiteUpdateTest::TestCase;

use strict;
use warnings;

sub create_publishers
{
  my ($self, $args) = @_;

  foreach my $arg (@$args)
  {
    die "$self->{case_name}: Publisher '$arg->{name}' id redefined!"
      if exists $self->{publishers}->{$arg->{name}};

    $self->{publishers}->{$arg->{name}} = $self->{ns}->create(Publisher => $arg);
    $self->{ns}->output("$arg->{name}/SITE_ID", $self->{publishers}->{$arg->{name}}->{site_id});
    $self->{ns}->output("$arg->{name}/TAG_ID", $self->{publishers}->{$arg->{name}}->{tag_id});
    my $site = $self->{publishers}->{$arg->{name}}->{Site};
    if (defined $site->freq_cap_id)
    {
      $self->{ns}->output("$arg->{name}/FC", $site->freq_cap_id);
    }
  }
}

sub new
{
  my $self = shift;
  my ($ns, $case_name) = @_;

  unless (ref $self)
  { $self = bless{}, $self; }

  $self->{ns} = $ns->sub_namespace($case_name);
  $self->{case_name} = $case_name;

  return $self;
} 

1;

package SiteUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_site_
{
  my ($self, $ns) = @_;

  my $sub_ns = $ns->sub_namespace("InsertSite");
  $sub_ns->output("Site/NAME", make_autotest_name($sub_ns, "inserted"));

  my $fc = $sub_ns->create(
    FreqCap => { 
      window_length => 11, 
      window_count => 1 });

  $sub_ns->output("FC", $fc);
  $sub_ns->output("FC/WindowLength", 11);

}

sub update_site_campaign_approval_
{
  my ($self, $ns) = @_;

  my $test_case = new SiteUpdateTest::TestCase($ns, "UpdateSiteCreativeApproval");

  $test_case->create_publishers([
    { name => "Publisher",
      pubaccount_account_type_id => $self->{publisher_type},
      cpm => 10,
      exclusions => [{ creative_category_id => $self->{visual_category},
                            approval => 'P' }],
      creative_links => [{ creative_id => $self->{campaign}->{creative_id} }] } ]);
}

sub update_noads_timeout_
{
  my ($self, $ns) = @_;

  my $test_case = new SiteUpdateTest::TestCase($ns, "UpdateNoAdsTimeout");

  $test_case->create_publishers([
    { name => "Publisher",
      cpm => 10,
      no_ads_timeout => 40 } ]);
}

sub update_creative_exclusion_
{
  my ($self, $ns) = @_;

  my $test_case = new SiteUpdateTest::TestCase($ns, "UpdateCreativeExclusion");

  $test_case->create_publishers([
    { name => "Publisher",
      pubaccount_account_type_id => $self->{publisher_type},
      cpm => 10,
      exclusions => [{ creative_category_id => $self->{visual_category},
                            approval => 'P' }],
      creative_links => [{ creative_id => $self->{campaign}->{creative_id} }] } ]);
}

sub delete_site_
{
  my ($self, $ns) = @_;

  my $test_case = new SiteUpdateTest::TestCase($ns, "DeleteSite");

  $test_case->create_publishers([
    { name => "Publisher",
      site_freq_cap_id => 
        DB::FreqCap->blank( 
          window_length => 22, 
          window_count => 1) }] );

  $test_case->{ns}->output("FC/WindowLength", 22);
}

sub update_freq_caps_
{
  my ($self, $ns) = @_;

  my $test_case = new SiteUpdateTest::TestCase($ns, "UpdateFC");

  $test_case->create_publishers([
    { name => "Publisher",
      site_freq_cap_id =>
        DB::FreqCap->blank( 
          window_length => 30, 
          window_count => 1) } ] );

  $test_case->{ns}->output("FC/WindowLength", 30);
}

sub init
{
  my ($self, $ns) = @_;

  $self->{publisher_type} = $ns->create(AccountType => {
    name => "Exclusion",
    account_role_id => DB::Defaults::instance()->publisher_role,
    adv_exclusions => 'T',
    flags => ( DB::AccountType::FREQ_CAPS )
    });

  my $advertiser = $ns->create(Account => {
    name => 'Adv',
    role_id => DB::Defaults::instance()->advertiser_role });

  $self->{content_category} = $ns->create(CreativeCategory => {
    name => "C",
    cct_id => 1 });

  $self->{visual_category} = $ns->create(CreativeCategory => {
    name => "V" });

  my $keyword = make_autotest_name($ns, "kwd");

  $self->{campaign} = $ns->create(DisplayCampaign => {
    name => 'Campaign',
    account_id => $advertiser,
    campaign_flags => 0,
    campaigncreativegroup_flags => 0,
    creative_creative_category_id => [ $self->{visual_category}, $self->{content_category} ],
    # all sites
    channel_id => DB::BehavioralChannel->blank(
      name => 'Channel',
      account_id => $advertiser,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => "P") ])});

  $ns->output('Global/VCAT', $self->{visual_category});
  $ns->output('Global/CCAT', $self->{content_category});

  # ad frontend request traits
  $ns->output("Global/KEYWORD", $keyword, "keyword for matching channel");
  $ns->output('Global/CCID', $self->{campaign}->{cc_id}, "campaign creative");
  $ns->output('Global/CCGID', $self->{campaign}->{ccg_id}, "campaign creative group");
  $ns->output('Global/CAMPAIGNID', $self->{campaign}->{campaign_id}, "campaign for approve");
  $ns->output('Global/CREATIVEID', $self->{campaign}->{creative_id}, "creative");
  $ns->output('Global/ACCOUNT', DB::Defaults::instance()->publisher_account);

  $self->create_site_($ns);
  $self->update_site_campaign_approval_($ns);
  $self->update_noads_timeout_($ns);
  $self->update_creative_exclusion_($ns);
  $self->delete_site_($ns);
  $self->update_freq_caps_($ns);
}

1;
