
package DB::CampaignSchedule;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  schedule_id => DB::Entity::Type::sequence(),
  campaign_id => DB::Entity::Type::link('DB::Campaign', unique => 1),
  time_from => DB::Entity::Type::int(unique => 1),
  time_to => DB::Entity::Type::int(unique => 1)
};

1;

package DB::CampaignExcludedChannel;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT =>
{
  campaign_id => DB::Entity::Type::link('DB::Campaign', unique => 1),
  channel_id => DB::Entity::Type::link('DB::CMPChannelBase', unique => 1)
};

1;

package DB::Campaign;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant EXTERNAL                 =>  0x001;
use constant RON                      =>  0x002;
use constant CREATIVE_OPTIMIZATION    =>  0x020;
use constant EXCLUDE_CLICK_URL        =>  0x040;
use constant INCLUDE_SPECIFIC_SITES   =>  0x200;

use constant STRUCT => 
{
  campaign_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  account_id => DB::Entity::Type::link('DB::Account'),
  flags => DB::Entity::Type::int(default => 0),
  freq_cap_id => DB::Entity::Type::link('DB::FreqCap', nullable => 1),
  budget => DB::Entity::Type::int(nullable => 1, default => undef),
  status => DB::Entity::Type::status(),
  sold_to_user_id => 
    DB::Entity::Type::link( 
      'DB::Users',
      default => sub { DB::Defaults::instance()->user }),
  bill_to_user_id => 
    DB::Entity::Type::link( 
      'DB::Users',
      default => sub { DB::Defaults::instance()->user }),
  display_status_id => DB::Entity::Type::display_status('Campaign'),
  commission => DB::Entity::Type::float(default => 0),
  marketplace => DB::Entity::Type::enum(['WG', 'OIX', 'ALL'], nullable => 1),
  campaign_type => DB::Entity::Type::enum(['D', 'T']),
  max_pub_share => DB::Entity::Type::float(default => 1.0),
  date_start => DB::Entity::Type::pq_date("current_date - 1"),
  date_end => DB::Entity::Type::pq_date(undef, nullable => 1),
  delivery_pacing => DB::Entity::Type::enum(['U', 'D', 'F']),
  daily_budget => DB::Entity::Type::float(nullable => 1),

  # Private fields
  excluded_channels => DB::Entity::Type::link_array('DB::CMPChannelBase', private => 1),
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  my $account = ref $args->{account_id}?
      $args->{account_id}->{account_id}: $args->{account_id};
  ($args->{marketplace}) = 
    $ns->pq_dbh->selectrow_array(qq[
      SELECT agency_marketplace
      FROM walledgarden
      WHERE agency_account_id = $account]) 
      if not exists $args->{marketplace};
}

sub postcreate_
{
  my ($self, $ns) = @_;

  if (defined $self->{excluded_channels})
  {
    my @channels = ref $self->{excluded_channels} eq 'ARRAY'
      ? @{$self->{excluded_channels}}
      : ($self->{excluded_channels});

    foreach my $channel (@channels)
    {
      $ns->create(DB::CampaignExcludedChannel->blank(
        campaign_id => $self->{campaign_id},
        channel_id => $channel ));
    }
  }
}

1;

# Text campaign
package DB::Campaign::Text;

use warnings;
use strict;

our @ISA = qw(DB::Campaign);

sub _table {
    'Campaign'
}

use constant STRUCT => 
{
  %{ DB::Campaign->STRUCT },
    
  campaign_type => 'T'
};

1;

# Display campaign
package DB::Campaign::Display;

use warnings;
use strict;

our @ISA = qw(DB::Campaign);

sub _table {
    'Campaign'
}

use constant STRUCT => 
{
  %{ DB::Campaign->STRUCT },
    
  campaign_type => 'D'
};

1;

# Composite entity with site links (mixin)
package DB::SiteLinksMixin;

use warnings;
use strict;

sub links
{
  {
    site_links => [
      DB::PubAccount->blank(),
      DB::Site->blank(),
      DB::CCGSite->blank(),
      DB::SiteCreativeApproval->blank() ]
  }
}

# Save PubAccount
sub store_pubaccount
{
  my ($self,  $publisher, $primary_key, $args) = @_;
  die "Invalid primary key '$primary_key' for PubAccount" 
    if $primary_key ne 'account_id';
  push (@{$self->{PubAccount}}, $publisher);
  $args->{account_id} = $publisher->{account_id};
}

sub __create_entity
{
  my ($self, $ns, $blank, $args, $own_args, $counters, $setter) = @_;

  # Avoid accounts name clash
  if (defined $own_args->{name} && $own_args->{name} eq $args->{name})
  {
    $own_args->{name} = 'Pub-' . $own_args->{name};
  }

  # Special case for SiteCreativeApproval
  if ( $blank->{__entity} eq 'DB::SiteCreativeApproval' and
       not defined $args->{creative_id})
  {
    return;
  }

  # Only own args for PubAcccount
  if ($blank->{__entity} eq 'DB::PubAccount')
  {
    return if defined $own_args->{account_id} || defined $own_args->{site_id};
    DB::Entity::Composite::__create_entity(
     $self, $ns, $blank, $own_args, {}, $counters, \&store_pubaccount);
  }
  else
  {
    DB::Entity::Composite::__create_entity(
      $self, $ns, $blank, $args, $own_args, $counters, $setter);
  }

  # Create Creative_TagSize links
  my %args_copy = (%$args, %$own_args);
  if (defined  $self->{creative_id} && defined $args_copy{site_id} && 
      !$args_copy{creative_tag_sizes} && !$args_copy{creative_tag_size_types})
  {
    if (defined $own_args->{size_id})
    {
      $ns->create(
        DB::Creative_TagSize->blank(
          creative_id => $self->{creative_id},
          size_id => $own_args->{size_id} ));
    }
    else
    {
      my $stmt = $ns->pq_dbh->prepare_cached(
       qq[SELECT size_id FROM Tag_TagSize WHERE tag_id IN
         (SELECT tag_id FROM Tags WHERE site_id = ?)]);

      my $site = ref $args_copy{site_id}?
        $args_copy{site_id}->{site_id}: $args_copy{site_id};

      $stmt->execute($site);

      while (my $res = $stmt->fetchrow_arrayref)
      {
        $ns->create(
          DB::Creative_TagSize->blank(
            creative_id => $self->{creative_id},
            size_id => $res->[0] ));
      }
    }
  }
}

1;

# Keyword text campaign composite
package DB::TextAdvertisingCampaign;

use warnings;
use strict;
use DB::Entity::Composite;

our @ISA = qw(DB::SiteLinksMixin DB::Entity::Composite);

sub tables
{
 (
   DB::Advertiser->blank(),
   DB::Template->blank(),
   DB::TemplateFile->blank(
     size_id => sub { DB::Defaults::instance()->text_size },
     flags => 0,
     template_type => 'X',
     template_file => 'UnitTests/textad.xsl'),
   DB::Creative->blank(
     size_id => sub { DB::Defaults::instance()->text_size } ),
   DB::Campaign::Text->blank(
     commission => 0),
   DB::CampaignCreativeGroup::Keyword->blank(
     delivery_pacing => 'F',
     daily_budget => 1000000,
     cpm => undef,
     cpc => 0.0001,
     cpa => undef,
     flags => DB::Campaign::INCLUDE_SPECIFIC_SITES),
   DB::CampaignCreative->blank(),
   DB::CCGKeyword->blank(
     click_url => 'http://test.com',
     ctr => 0.01)
  )
}

1;

# Channel text campaign composite
package DB::ChannelTargetedTACampaign;

use warnings;
use strict;
use DB::Entity::Composite;

our @ISA = qw(DB::SiteLinksMixin DB::Entity::Composite);

sub tables
{
 (
   DB::Advertiser->blank(),
   DB::Template->blank(),
   DB::TemplateFile->blank(
     size_id => sub { DB::Defaults::instance()->text_size },
     template_type => 'X',
     template_file => 'UnitTests/textad.xsl'),
   DB::Creative->blank(
     size_id => sub { DB::Defaults::instance()->text_size } ),
   DB::Campaign::Text->blank(
     commission => 0),
   DB::BehavioralChannel->blank(),
   DB::CampaignCreativeGroup::Channel->blank(
     delivery_pacing => 'F',
     daily_budget => 1000000,
     cpm => undef,
     cpc => undef,
     cpa => undef,
     flags => DB::Campaign::INCLUDE_SPECIFIC_SITES),
   DB::CampaignCreative->blank()
  )
}

1;

# Display campaign composite
package DB::DisplayCampaign;

use warnings;
use strict;

our @ISA = qw(DB::SiteLinksMixin DB::Entity::Composite);

sub tables
{
 (
   DB::Advertiser->blank(),
   DB::Creative->blank(),
   DB::Campaign::Display->blank(
     commission => 0),
   DB::BehavioralChannel->blank(),
   DB::CampaignCreativeGroup::Display->blank(
     flags => DB::Campaign::INCLUDE_SPECIFIC_SITES),
   DB::CampaignCreative->blank()
  )
}

1;
