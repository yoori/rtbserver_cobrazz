package HistorySessionTargettingTest;

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

  # Session channels
  $self->make_channel(
    $ns,
    "Session1",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_to => 8*60 });

  $self->make_channel(
    $ns,
    "Session2",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 30,
      time_to => 120 });

  $self->make_channel(
    $ns,
    "Session3",
    $account,
    { trigger_type => "P",
      minimum_visits => 1,
      time_to => 10*60 });

  $self->make_channel(
    $ns,
    "Session4",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 2*60,
      time_to => 10*60 });

  $self->make_channel(
    $ns,
    "SessionBoundary1",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 11*60*60,
      time_to => 23*60*60 + 59*60 });

  $self->make_channel(
    $ns,
    "SessionBoundary2",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 12*60*60,
      time_to => 13*60*60 });


  # H+T channels
  $self->make_channel(
    $ns,
    "HT1",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_to => 86400 });

  $self->make_channel(
    $ns,
    "HT2",
    $account,
    { trigger_type => "P",
      minimum_visits => 3,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "HT3",
    $account,
    { trigger_type => "P",
      minimum_visits => 1,
      time_to => 86400 });

  $self->make_channel(
    $ns,
    "HT4",
    $account,
    { trigger_type => "P",
      minimum_visits => 1,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "HT5",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "HT6",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "HT7",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_to => 259200 });

  # History channels
  $self->make_channel(
    $ns,
    "History1",
    $account,
    { trigger_type => "P",
      minimum_visits => 3,
      time_from => 86400,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "History2",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 86400,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "History3",
    $account,
    { trigger_type => "P",
      minimum_visits => 3,
      time_from => 86400,
      time_to => 172800 });

  $self->make_channel(
    $ns,
    "History4",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 172800,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "History5",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 86400,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "History6",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 86400,
      time_to => 259200 });

  $self->make_channel(
    $ns,
    "HistoryBoundary",
    $account,
    { trigger_type => "P",
      minimum_visits => 2,
      time_from => 24*60*60,
      time_to => 2*24*60*60 });
}

1;
