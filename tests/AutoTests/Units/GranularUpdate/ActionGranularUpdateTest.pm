package ActionGranularUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

# Case1. Add action & link to CCG 
sub add_action
{
  my ($self, $namespace) = @_;
  
  my $ns = $namespace->sub_namespace("ADDACTION");
 
  my $campaign = $ns->create(DisplayCampaign => { 
    name => 'Display' });

  $ns->output("CCG", $campaign->{ccg_id});
  $ns->output("ACTIONNAME", make_autotest_name($ns, "act"));
  $ns->output("ACCOUNT", $campaign->{account_id});
  $ns->output("URL", "http:\\www.addaction.com");
}

# Case2. Unlink action from CCG 
sub unlink_action
{
  my ($self, $namespace) = @_;
  
  my $ns = $namespace->sub_namespace("UNLINKACTION");

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $action1 = $ns->create(Action => {
    name => 'Action1',
    url => "http:\\www.unlinkaction1.com",
    account_id => $advertiser});

  my $action2 = $ns->create(Action => {
    name => 'Action2',
    url => "http:\\www.unlinkaction2.com",
    account_id => $advertiser});

  my $campaign1 = $ns->create(DisplayCampaign => { 
    name => 'Display-1',
    account_id => $advertiser,
    action_id => [$action1, $action2]
 });

  my $campaign2 = $ns->create(DisplayCampaign => { 
    name => 'Display-2',
    account_id => $advertiser,
    action_id => [$action1, $action2]
 });

  $ns->output("CCG1", $campaign1->{ccg_id});
  $ns->output("CCG2", $campaign2->{ccg_id});
  $ns->output("ACTION1", $action1);
  $ns->output("ACTION2", $action2);
}

# Case3. Action linked to inactive CCG 
sub inactive_ccg
{
  my ($self, $namespace) = @_;
  
  my $ns = $namespace->sub_namespace("INACTIVECCG");

  my $advertiser = $ns->create(Account => {
    name => 'Advertiser',
    role_id => DB::Defaults::instance()->advertiser_role });

  my $action = $ns->create(Action => {
    name => 'Action',
    url => "http:\\www.inactive.com",
    account_id => $advertiser});

  my $keyword = make_autotest_name($ns, "kw");

  my $campaign1 = $ns->create(DisplayCampaign => {
    name => 'Display-1',
    account_id => $advertiser,
    behavioralchannel_keyword_list => $keyword,
    campaign_flags => 0,
    campaigncreativegroup_cpa => 10,
    campaigncreativegroup_action_id => $action,
    campaigncreativegroup_status => 'I',
    campaigncreativegroup_display_status_id => DISPLAY_STATUS_INACTIVE,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{name => 'Site'}] });

  my $campaign2 = $ns->create(DisplayCampaign => {
    name => 'Display-2',
    account_id => $advertiser,
    behavioralchannel_keyword_list => $keyword,
    campaign_flags => 0,
    campaigncreativegroup_cpa => 10,
    campaigncreativegroup_action_id => $action,
    campaigncreativegroup_flags => DB::Campaign::INCLUDE_SPECIFIC_SITES,
    site_links => [{site_id => $campaign1->{Site}[0]->{site_id}}] });

  my $tag = $ns->create(PricedTag => {
    name => 'Tag',
    site_id => $campaign1->{Site}[0]->{site_id} });

  $ns->output("CCG1", $campaign1->{ccg_id});
  $ns->output("CCG2", $campaign2->{ccg_id});
  $ns->output("ACTION", $action);

}

sub init
{
  my ($self, $ns) = @_;

  $self->add_action($ns);
  $self->unlink_action($ns);
  $self->inactive_ccg($ns);
}

1;
