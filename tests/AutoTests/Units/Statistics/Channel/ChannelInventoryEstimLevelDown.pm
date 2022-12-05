
package ChannelInventoryEstimLevelDown;

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
         time_to => 3600,
         minimum_visits => 4)]);

  $self->create_channel($ns, "S2", $account, 
    [ DB::BehavioralChannel::BehavioralParameter->blank(
         trigger_type =>  "P",
         time_from => 0,
         time_to => 82800,
         minimum_visits => 1)]);

}

1;
