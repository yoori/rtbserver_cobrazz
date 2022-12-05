package UserProfileMergingTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub make_channel
{
  my ($self, $ns, $prefix, $account, $params) = @_;

  my $keyword = make_autotest_name($ns, $prefix);
  
  my $channel = $ns->create(DB::BehavioralChannel->blank(
      account_id => $account,
      name => $prefix,
      keyword_list => $keyword,
      behavioral_parameters => 
        [ DB::BehavioralChannel::BehavioralParameter->blank(%$params) ] ));

  $ns->output($prefix . "Keyword", $keyword);
  $ns->output($prefix . "Channel", $channel->{channel_id});
  $ns->output($prefix . "BP", $channel->page_key());
}

sub init
{
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  # Context channel
  $self->make_channel(
    $ns,
    "Context",
    $account,
    { trigger_type =>  "P" ,
      time_from => 0,
      time_to => 0,
      minimum_visits => 1 });

  # Session channel#1
  $self->make_channel(
    $ns,
    "Session1",
    $account,
    { trigger_type => "P",
      minimum_visits => 3,
      time_to => 7200  });

  # Session channel#2
  $self->make_channel(
    $ns,
    "Session2",
    $account,
    { trigger_type => "P",
      minimum_visits => 1,
      time_to => 3600 });

  # History+Today channel
  $self->make_channel(
    $ns,
    "HT",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_to => 259200 });

  # History channel
  $self->make_channel(
    $ns,
    "History",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 86400,
      time_to => 259200});
}

1;
