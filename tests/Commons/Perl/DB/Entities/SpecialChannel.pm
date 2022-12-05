
package DB::SpecialChannelBase;

use warnings;
use strict;
use DB::Entity::PQ;
use DB::Entities::BehavioralChannel;

our @ISA = qw(DB::BehavioralChannelBase);

sub clear_triggers_
{
  my ($self, $ns) = @_;

  $ns->pq_dbh->prepare_cached(qq[
    DELETE FROM ChannelTrigger WHERE channel_id = ?])->
      execute($self->channel_id());

  $ns->pq_dbh->prepare_cached(qq[
    DELETE FROM Triggers WHERE channel_type = 'S' AND
    trigger_id NOT IN (SELECT trigger_id FROM 
    ChannelTrigger)])->execute();
}

sub triggers_version_update_
{
  my ($self, $ns) = @_;

  $ns->pq_dbh->prepare_cached(qq[
    UPDATE Channel 
    SET triggers_version = now()
    WHERE channel_id = ?])->execute($self->channel_id());
}

sub special_channel_preinit_
{
  my ($self, $args) = @_;
  
  DB::BehavioralChannelBase::behavioral_channel_preinit_($self, $args);
}

sub special_channel_postcreate_
{
  my ($self, $ns) = @_;

  $self->clear_triggers_($ns);
  DB::BehavioralChannelBase::behavioral_channel_postcreate_($self, $ns);

  # Update special channel trigger version
  $self->triggers_version_update_($ns);
}

1;

package DB::NoAdvChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ DB::SpecialChannelBase);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  country_code => DB::Entity::Type::country(nullable => 1, default => undef),
  channel_id => DB::Entity::Type::int(unique => 1, default => 1),
  
  name => 'NO_ADVERTISING',
  account_id => undef,
  status => 'A',
  qa_status => 'A',
  display_status_id => 1,
  flags => 0,
  namespace => 'S',
  channel_type => 'S',
  visibility => 'PUB',
  message_sent => 0
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  DB::SpecialChannelBase::special_channel_preinit_($self, $args);
}

sub postcreate_
{
  my ($self, $ns) = @_;

  DB::SpecialChannelBase::special_channel_postcreate_($self, $ns);
}

1;

package DB::NoTrackChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ DB::SpecialChannelBase);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  country_code => DB::Entity::Type::country(nullable => 1, default => undef),
  channel_id => DB::Entity::Type::int(unique => 1, default => 2),
  
  name => 'NO_TRACKING',
  account_id => undef,
  status => 'A',
  qa_status => 'A',
  display_status_id => 1,
  flags => 0,
  namespace => 'S',
  channel_type => 'S',
  visibility => 'PUB',
  message_sent => 0
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  DB::SpecialChannelBase::special_channel_preinit_($self, $args);
}

sub postcreate_
{
  my ($self, $ns) = @_;
  
  DB::SpecialChannelBase::special_channel_postcreate_($self, $ns);
}

1;
