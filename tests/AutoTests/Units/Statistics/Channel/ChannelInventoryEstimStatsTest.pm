package ChannelInventoryEstimStatsTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use FindBin;
use lib "$FindBin::Dir/../Units/Statistics/Channel";
use ChannelInventoryEstimCommon;

our @ISA = qw(ChannelInventoryEstimCommon);

sub init
{
  my ($self, $ns) = @_;
  
  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  # Session channels
  for (my $i = 0; $i < 2; ++$i)
  {
    $self->create_channel($ns, $i, $account, 
      [  DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type =>  "P",
          time_from => 60,
          time_to => 120,
          minimum_visits => 4),
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type =>  "S",
          time_from => 120,
          time_to => 240,
          minimum_visits => 2),        
        DB::BehavioralChannel::BehavioralParameter->blank(
          trigger_type =>  "U",
          time_to => 180,
          minimum_visits => 1)]);
  }    

  # ADSC-7276
  my $colo1 = $ns->create(Isp => {
    name => "colo1" });

  $ns->output("COLO1", $colo1->{colo_id});

  my $colo2 = $ns->create(Isp => {
    name => "colo2" });

  $ns->output("COLO2", $colo2->{colo_id});

  $self->create_channel($ns, "COLO", $account,
    [ DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "P",
         time_to => 1800,
         minimum_visits => 4) ]);

  # History+today channels
  $self->create_channel($ns, 'HT', $account, 
    [  DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "P",
         time_to => 259200,
         minimum_visits => 2)]);

  # History channels
  $self->create_channel($ns, 'H1', $account, 
    [  DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "U",
         time_from => 86400,
         time_to => 259200,
         minimum_visits => 1)]);

  $self->create_channel($ns, 'H2', $account, 
    [  DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "P",
         time_from => 172800,
         time_to => 259200,
         minimum_visits => 1)]);

}

1;
