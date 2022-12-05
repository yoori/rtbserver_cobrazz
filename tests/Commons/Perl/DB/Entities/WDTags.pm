
package DB::Feed;
use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  feed_id => DB::Entity::Type::sequence(),
  url => DB::Entity::Type::string(unique => 1)
};

1;

package DB::WDTagFeed_OptedIn;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  feed_id => DB::Entity::Type::link('DB::Feed', unique => 1),
  wdtag_id => DB::Entity::Type::link('DB::WDTag', unique => 1)
};

1;

package DB::WDTagFeed_OptedOut;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  feed_id => DB::Entity::Type::link('DB::Feed', unique => 1),
  wdtag_id => DB::Entity::Type::link('DB::WDTag', unique => 1)
};

1;

package DB::WDTag;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  wdtag_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  site_id => DB::Entity::Type::link('DB::Site'), 
  status =>  DB::Entity::Type::status(),
  width => DB::Entity::Type::int(default => 10),
  height => DB::Entity::Type::int(default => 10),
  opted_in_content =>  DB::Entity::Type::enum(['A', 'S']),
  opted_out_content => DB::Entity::Type::enum(['A', 'S', 'P']),
  passback => DB::Entity::Type::string(nullable => 1), 
  template_id =>
    DB::Entity::Type::link(
      "DB::Template",
      default => sub { DB::Defaults::instance()->discover_template }),

  # Private
  opt_out_feed_urls => DB::Entity::Type::link_array(undef, private => 1, default => []),
  opt_in_feed_urls => DB::Entity::Type::link_array(undef, private => 1, default => [])
};

sub postcreate_
{
  my ($self, $ns) = @_;
  
  $self->{opted_in_feeds} = [];
  $self->{opted_out_feeds} = [];

  foreach my $url (@{$self->{opt_in_feed_urls}})
  {
    my $feed = $ns->create( Feed => {url => $url});
    $ns->create(WDTagFeed_OptedIn => {
      feed_id => $feed,
      wdtag_id => $self->{wdtag_id} });
    push (@{$self->{opted_in_feeds}}, $feed);
  }

  foreach my $url (@{$self->{opt_out_feed_urls}})
  {
    my $feed = $ns->create( Feed => {url => $url});
    $ns->create(WDTagFeed_OptedOut => {
      feed_id => $feed,
      wdtag_id => $self->{wdtag_id} });
    push (@{$self->{opted_out_feeds}}, $feed);
  }
}

1;
