
package DB::DeviceChannel::Platform;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

sub _table
{
  'Platform'
}

use constant STRUCT => 
{
  platform_id => DB::Entity::Type::sequence('channel_channel_id_seq'),
  name => DB::Entity::Type::string(unique => 1),
  type => DB::Entity::Type::string(unique => 1),

  # Private
  priority => DB::Entity::Type::int(private => 1),
  match_marker => DB::Entity::Type::string(private => 1),
  match_regexp => DB::Entity::Type::string(private => 1),
  output_regexp => DB::Entity::Type::string(private => 1)
};

sub postcreate_ 
{
  my ($self, $ns) = @_;

  my @defined_args = 
    grep {exists $self->{$_}}
      qw(prority match_marker match_regexp output_regexp);

  if (@defined_args)
  {
    my @args_list = (@defined_args, "platform_id");
    my %args;
    @args{@args_list} = @$self{@args_list};
    $ns->create(DB::DeviceChannel::PlatformDetector->blank(%args));
  }
}

1;

package DB::DeviceChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

sub _table
{
  'Channel'
}

use constant STRUCT => 
{
  name => DB::Entity::Type::name(unique => 1),
  channel_id => DB::Entity::Type::sequence(),
  account_id => undef, 
  status => DB::Entity::Type::status(),
  qa_status => DB::Entity::Type::qa_status(),
  display_status_id => DB::Entity::Type::display_status('DeviceChannel'),
  country_code => undef,
  status_change_date => DB::Entity::Type::pq_timestamp("timestamp 'now'"),
  expression => DB::Entity::Type::string(nullable => 1),
  channel_type => 'V',
  flags => 0,
  namespace => 'V',
  visibility => 'PUB',
  message_sent => 0
};

package DB::DeviceChannel::PlatformDetector;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  platform_detector_id => DB::Entity::Type::raw_sequence(),
  platform_id => DB::Entity::Type::link('DB::Platform'),
  priority => DB::Entity::Type::int(default => 1),
  match_marker => DB::Entity::Type::string(nullable => 1),
  match_regexp => DB::Entity::Type::string(nullable => 1),
  output_regexp => DB::Entity::Type::string(nullable => 1),
  last_updated => DB::Entity::Type::pq_timestamp("timestamp 'now'")
};


1;
