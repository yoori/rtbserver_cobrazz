


package PerformanceDB::FreqCap;

use warnings;
use strict;

sub rand_in_range {
  my $min = shift;
  my $max = shift;
  my $v   = int(rand($max-$min+1));
  return $v+$min;
}

sub create {
  my $period        = rand_in_range(5, 120);
  my $window_length = rand_in_range(60, 600);
  my $window_count  = rand_in_range(2, 10);
  my $life_count    = rand_in_range(1, 100);
  return 
    DB::FreqCap->blank(
      period => $period,
      window_length => $window_length, 
      window_count => $window_count,
      life_count => $life_count);
}

1;

package PerformanceDB::Tags;

use warnings;
use strict;
use CampaignConfig;
use DB::Util;

use constant PASSBACK_URL  => "http://www.pt.ocslab.com";

sub new {
  my $self = shift;
  my ($db, $entity_name, $count, $size, $free) = @_;
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{_db}           = $db;
  $self->{_entity_name}  = $entity_name;
  $self->{_count}        = $count;
  $self->{_size}         = $size;
  $self->{_free}         = $free;
  unless (defined $free) {
    $self->{_free}       = 0;
  }
  return $self;
}

sub _create_site {
  my $self = shift;
  my ($ccg_id, $creative_id, $idx) = @_;
  my $entity_name = $self->{_entity_name}  . "-" .$idx ;
  my $site = $self->{_db}->create(Site => {
    name => "$entity_name",
    account_id => $self->{_db}->acc_id });

  $self->{_db}->create(CCGSite => {
    site_id => $site,
    ccg_id => $ccg_id }) if defined $ccg_id;

  $self->{_db}->create(SiteCreativeApproval => {
    site_id => $site,
    creative_id => $creative_id })
      if defined $creative_id;

  return $site;
}

sub _create_tag {
  my $self = shift;
  my ($site_id, $idx) = @_;
  my $entity_name = $self->{_entity_name} . "-" .$idx ;
  return $self->{_db}->create(PricedTag =>
                              { name => "$entity_name",
                                site_id => $site_id,
                                size_id => $self->{_size}});
}

sub create {
  my $self = shift;
  my ($creative, $ccg) = @_;
  for (my $i = 0; $i < $self->{_count}; ++$i)
  {
    my $site     = $self->_create_site($ccg, $creative, $i);
    my $tag      = $self->_create_tag($site, $i);
    $self->{_db}->store_tid($tag->tag_id, $self->{_free});
  }
}

1;

package PerformanceDB::Campaign;

use warnings;
use strict;
use CampaignConfig;
use DB::Defaults;
use DB::Util;

sub new {
  my $self = shift;
  my ($db, $entity_name, $flags) = @_;
  unless (ref $self) {
    $self = bless {}, $self;
  }
  $self->{_db}           = $db;
  $self->{_entity_name}  = $entity_name;
  $self->{_flags}        = $flags;
  return $self;
}

sub _create_campaign {
  my $self = shift;
  my $entity_name = $self->{_entity_name};
  my $flags       = 0;
  my $freq_cap_id = undef;
  if ($self->{_flags} & CampaignConfig::CampaignFlags::CampaignSpecificSitesFlag)
  {
    $flags |= DB::Campaign::INCLUDE_SPECIFIC_SITES;
  }
  if ($self->{_flags} & CampaignConfig::CampaignFlags::RONFlag)
  {
    $flags |= DB::Campaign::RON | DB::Campaign::INCLUDE_SPECIFIC_SITES;
  }
  if ($self->{_flags} & CampaignConfig::CampaignFlags::FreqCapsCampaignFlag)
  {
    $freq_cap_id = PerformanceDB::FreqCap::create;
  }
  return $self->{_db}->create(Campaign =>
                     { name => "$entity_name",
                       account_id => $self->{_db}->acc_id,
                       flags => $flags,
                       freq_cap_id => $freq_cap_id });
}


sub _create_creative {
  my $self = shift;
  my $entity_name = $self->{_entity_name};
  my $creative = $self->{_db}->create(
    Creative => {
      name => "$entity_name",
      size_id => $self->{_db}->size,
      account_id => $self->{_db}->acc_id });
  return $creative;
}


sub _create_campaign_creative_group {
  my $self = shift;
  my ($campaign_id, $channel_id) = @_;
  my $entity_name = $self->{_entity_name};
  my $freq_cap_id = undef;
  if ($self->{_flags} & CampaignConfig::CampaignFlags::FreqCapsCCGFlag)
  {
    $freq_cap_id = PerformanceDB::FreqCap::create;
  }
  return $self->{_db}->create(CampaignCreativeGroup =>
    { name => "CreativeGroup-$entity_name",
      campaign_id => $campaign_id,
      channel_id  => $channel_id,
      freq_cap_id => $freq_cap_id,
      country_code => $self->{_db}->country->{country_code},
      flags => 0});
}

sub _create_campaign_creative {
  my $self = shift;
  my ($ccg_id, $creative_id) = @_;
  my $freq_cap_id = undef;
  if ($self->{_flags} & CampaignConfig::CampaignFlags::FreqCapsCreativeFlag)
  {
    $freq_cap_id = PerformanceDB::FreqCap::create;
  }
  return $self->{_db}->create(CampaignCreative =>
                     { ccg_id => $ccg_id,
                       creative_id => $creative_id,
                       freq_cap_id => $freq_cap_id });
}

sub create {
  my $self = shift;
  my ($channel_id, $tags_count, $site_id)= @_;
  my $campaign = $self->_create_campaign;
  my $creative = $self->_create_creative;
  my $ccg      = $self->_create_campaign_creative_group($campaign, $channel_id);
  my $cc       = $self->_create_campaign_creative($ccg, $creative);

  if ($site_id)
  {
    $self->{_db}->create(CCGSite => {
      site_id => $site_id,
      ccg_id => $ccg });

    $self->{_db}->create(SiteCreativeApproval => {
      site_id => $site_id,
      creative_id => $creative });
  }

  my $tags = new PerformanceDB::Tags(
    $self->{_db}, 
    $self->{_entity_name}, 
    $tags_count, 
    DB::Defaults::instance()->size);
  $tags->create($creative, $ccg);
}

1;





