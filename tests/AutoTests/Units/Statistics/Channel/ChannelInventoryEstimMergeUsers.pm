
package ChannelInventoryEstimMergeUsers;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;
use FindBin;
use lib "$FindBin::Dir/../Units/Statistics/Channel";
use ChannelInventoryEstimCommon;

our @ISA = qw(ChannelInventoryEstimCommon);

sub init {
  my ($self, $ns) = @_;

  my $account = $ns->create(Account => {
    name => 1,
    role_id => DB::Defaults::instance()->advertiser_role });

  # Session channels
  $self->create_channel($ns, "S1", $account, 
    [  DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "P",
         time_from => 0,
         time_to => 14400,
         minimum_visits => 4),
       DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "U",
         time_from => 0,
         time_to => 7200,
         minimum_visits => 1)]);

  $self->create_channel($ns, "S2", $account, 
    [ DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "U",
         time_from => 0,
         time_to => 7200,
         minimum_visits => 1)]);

  # History channels
  $self->create_channel($ns, "H1", $account, 
    [  DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "P",
         time_from => 86400,
         time_to => 259200,
         minimum_visits => 10)]);

  # History+today channels
  $self->create_channel($ns, "HT1", $account, 
    [  DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "P",
         time_from => 0,
         time_to => 259200,
         minimum_visits => 10)]);

  $self->create_channel($ns, "HT2", $account, 
    [  DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "P",
         time_from => 0,
         time_to => 259200,
         minimum_visits => 1)]);

}

1;
