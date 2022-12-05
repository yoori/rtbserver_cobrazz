package DB::ExpressionChannel::ExpressionUsedChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  expression_channel_id => DB::Entity::Type::link('DB::ExpressionChannel', unique => 1),
  used_channel_id => DB::Entity::Type::int(unique => 1)
};

1;

package DB::ExpressionChannel;

use warnings;
use strict;
use DB::Entity::PQ;

use constant DISPLAY_STATUS_NOT_LIVE_CHANNELS_NEED_ATTENTION =>
  "Not Live - Channels Needs Attention";
use constant DISPLAY_STATUS_LIVE_CHANNELS_NEED_ATTENTION =>
  "Live - Channels Need Attention";

our @ISA = qw(DB::Entity::PQ DB::CMPChannelBase);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  name => DB::Entity::Type::name(unique => 1),
  channel_id => DB::Entity::Type::sequence(),
  account_id => DB::Entity::Type::link('DB::Account'),
  status =>  DB::Entity::Type::status(),
  qa_status =>  DB::Entity::Type::qa_status(),
  display_status_id => DB::Entity::Type::display_status('ExpressionChannel'),
  country_code => DB::Entity::Type::country(),
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  expression => DB::Entity::Type::string(),
  visibility => DB::Entity::Type::enum(['PUB', 'PRI', 'CMP']),
  # one of PUB, PRI, CMP
  channel_rate_id => DB::Entity::Type::link('DB::BehavioralChannel::Rate', nullable => 1),
  # if channel_rate_id defined visibility must be undefined or equal to 'CMP'
  # if cpm, cpc (one must be = 0) defined channel_rate will be created automaticaly
  channel_type => 'E',
  flags => DB::Entity::Type::int(default => 0),
  namespace => 'A',
  message_sent => 0
};

sub preinit_
{
  my ($self, $ns, $args) = @_;
  
  DB::CMPChannelBase::cmp_channel_preinit_($self, $args);
  
  if(exists $args->{behavioral_parameters})
  {
    $self->{behavioral_parameters} = $args->{behavioral_parameters};
    delete $args->{behavioral_parameters};
  }
  
  if(exists $args->{categories})
  {
    $self->{categories} = $args->{categories};
    delete $args->{categories};
  }
  
  if(exists $args->{keyword_list})
  {
    delete $args->{keyword_list};
  }
  
  if(exists $args->{url_list})
  {
    delete $args->{url_list};
  }
  
  DB::CMPChannelBase::cmp_channel_preinit_($self, $args);
}

sub postcreate_
{
  my ($self, $ns) = @_;
  
  DB::CMPChannelBase::cmp_channel_postcreate_($self, $ns);

  if (defined $self->{expression})
  {
    foreach my $channel_id (grep {$_} split(/\D+/, $self->{expression}))
    {
      $ns->create(DB::ExpressionChannel::ExpressionUsedChannel->blank(
        expression_channel_id => $self->{channel_id},
        used_channel_id => $channel_id));
    }
  }
}

1;
