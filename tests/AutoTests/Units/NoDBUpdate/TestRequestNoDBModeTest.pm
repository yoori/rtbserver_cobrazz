
package TestRequestNoDBModeTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_testcase
{
  my ($self, $ns, $name, $cpm, $args) = @_;

  my $keyword = make_autotest_name($ns, $name);

  my %args_copy = %$args;
  $args_copy{name} = $name unless defined $args_copy{name};
  $args_copy{behavioralchannel_keyword_list} = $keyword
    unless defined $args_copy{behavioralchannel_keyword_list} or
           defined $args_copy{keyword_list};
  $args_copy{campaigncreativegroup_cpm} = $cpm;
  $args_copy{site_links} = [{ site_id => $self->{publisher}->{site_id} }]
    unless defined $args_copy{site_links};

  my $campaign = $ns->create(DisplayCampaign => \%args_copy);

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      channel_id => $campaign->{channel_id} ));

  $args_copy{name} = "$name-test";
  $args_copy{site_links} = [{ site_id => $self->{test_publisher}->{site_id} }];

  my $test_campaign = $ns->create(DisplayCampaign => \%args_copy);

  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      trigger_type => "P",
      channel_id => $test_campaign->{channel_id} ));

  $ns->output("$name/KEYWORD", $keyword);
  $ns->output("$name/CCGID", $campaign->{ccg_id});
  $ns->output("$name/CCID", $campaign->{cc_id});
  $ns->output("$name/TEST_CCGID", $test_campaign->{ccg_id});
  $ns->output("$name/TEST_CCID", $test_campaign->{cc_id});
} 

sub init
{
  my ($self, $ns) = @_;

  my $daily_budget = 1;
  my $cpm = 1001;

  # Common data

  my $keyword1 = make_autotest_name($ns, "Keyword1");
  my $keyword2 = make_autotest_name($ns, "Keyword2");
  my $keyword3 = make_autotest_name($ns, "Keyword3");
  
  $self->{publisher} =  
      $ns->create(Publisher => { name => 'PUB'});

  $self->{test_publisher} =  
      $ns->create(Publisher => { name => 'PUB-TEST'});

  $self->create_testcase($ns, "CCGBudget", $cpm,
    { campaigncreativegroup_daily_budget => $daily_budget,
      campaigncreativegroup_delivery_pacing => 'F' });

  $self->create_testcase($ns, "CampaignBudget", $cpm,
    { campaign_daily_budget => $daily_budget,
      campaign_delivery_pacing => 'F' });

  $self->create_testcase($ns, "AccountBudget", $cpm,
    { account_prepaid_amount => $daily_budget,
      campaign_daily_budget => $daily_budget,
      campaign_delivery_pacing => 'F' });

  $ns->output("TID", $self->{publisher}->{tag_id});
  $ns->output("TEST_TID", $self->{test_publisher}->{tag_id});
}

1;
