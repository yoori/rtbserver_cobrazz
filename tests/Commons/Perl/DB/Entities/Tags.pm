
# Site rate
package DB::SiteRate;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  site_rate_id => DB::Entity::Type::sequence(),
  rate_type => DB::Entity::Type::enum(['CPM', 'RS']),
  rate => DB::Entity::Type::float(nullable => 1, default => 0),
  effective_date => DB::Entity::Type::pq_date('now()'),
  tag_pricing_id => DB::Entity::Type::link('DB::TagPricing')
};

1;

# Tag CTR override
package DB::TagCTROverride;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  tag_id => DB::Entity::Type::link('DB::Tags', unique => 1),
  adjustment => DB::Entity::Type::float()
};

1;

# Tag auction settings
package DB::TagAuctionSettings;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  tag_id => DB::Entity::Type::link('DB::Tags', unique => 1),
  max_ecpm_share => DB::Entity::Type::float(default => 0),
  prop_probability_share => DB::Entity::Type::float(default => 0),
  random_share => DB::Entity::Type::float(default => 0),
  version => DB::Entity::Type::pq_date("timestamp 'now'"),
};

1;

# Tag exclusion for creative category
package DB::TagsCreativeCategoryExclusion;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  tag_id => DB::Entity::Type::link('DB::Tags', unique => 1),
  creative_category_id => DB::Entity::Type::link('DB::CreativeCategory', unique => 1),
  approval => DB::Entity::Type::enum( ['R', 'A'])
};

1;

# Tag pricing
package DB::TagPricing;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  tag_pricing_id => DB::Entity::Type::sequence(),
  tag_id => DB::Entity::Type::link('DB::Tags', unique => 1),
  country_code => DB::Entity::Type::country(unique => 1, nullable => 1, default => undef),
  site_rate_id => DB::Entity::Type::link('DB::SiteRate'),
  status => DB::Entity::Type::status(),

  # Private fields
  low_channel_threshold => DB::Entity::Type::int(private => 1),
  high_channel_threshold => DB::Entity::Type::int(private => 1),
  cpm => DB::Entity::Type::float(private => 1, default => 0),
  rate_type => DB::Entity::Type::enum(['CPM', 'RS'], private => 1)
};

sub precreate
{
  my ($self, $ns) = @_;

  if (defined $self->{country_code}) 
  {
    my %country_args = ();
    $country_args{country_code} = $self->{country_code};
    if (defined $self->{low_channel_threshold}) 
    {
      $country_args{low_channel_threshold} = $self->{low_channel_threshold};
    }
    if (defined $self->{high_channel_threshold}) 
    {
      $country_args{high_channel_threshold} = $self->{high_channel_threshold};
    }
    $ns->create(DB::Country->blank(%country_args));
  }
}

sub postcreate_
{
  my ($self, $ns) = @_;

  if (!defined $self->{site_rate_id})
  {
    die "Undefined CPM"  if (not defined $self->{cpm});

    $self->{site_rate_id} = $ns->create(
       DB::SiteRate->blank(
          tag_pricing_id => $self->{tag_pricing_id},
          rate => $self->{cpm},
          rate_type => (defined($self->{rate_type}) ? $self->{rate_type} : 'CPM')));

    $self->__update($ns, { site_rate_id => $self->{site_rate_id} });
  }
}

1;

package DB::TagOptionValue;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  option_id => DB::Entity::Type::link('DB::Option', unique => 1),
  tag_id => DB::Entity::Type::link('DB::Tags', unique => 1),
  value => DB::Entity::Type::string(nullable => 1)
};

1;

# Tags
package DB::Tags;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);


use constant STRUCT => 
{
  tag_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  site_id => DB::Entity::Type::link('DB::Site'),
  size_type_id => DB::Entity::Type::link('DB::SizeType',
    default => sub { DB::Defaults::instance()->other_size_type }),
  flags => DB::Entity::Type::int(default => 0),
  passback => DB::Entity::Type::string(nullable => 1),
  passback_type => DB::Entity::Type::enum(['HTML_URL', 'HTML_CODE', 'JS_CODE']),
  passback_code => DB::Entity::Type::string(nullable => 1),
  marketplace => DB::Entity::Type::enum(['WG', 'OIX', 'ALL'], nullable => 1),
  allow_expandable => 'Y',
  status => DB::Entity::Type::status(),
  
  # Private fields
  ## TagPricing
  country_code => DB::Entity::Type::country(private => 1),
  cpm => DB::Entity::Type::float(private => 1),
  rate_type => DB::Entity::Type::enum(['CPM', 'RS'], private => 1),
  ## Country
  low_channel_threshold => DB::Entity::Type::int(private => 1),
  high_channel_threshold => DB::Entity::Type::int(private => 1),
  ## TagCTROverride
  adjustment => DB::Entity::Type::float(private => 1, default => 1.0),
  ## Options
  size_id => DB::Entity::Type::link_array('DB::CreativeSize',
    private => 1,
    default => sub { DB::Defaults::instance()->size }),
  cradvtrackpixel_value => DB::Entity::Type::string(private => 1),
  publ_tag_track_pixel_value => DB::Entity::Type::string(private => 1),
  max_ads_per_tag_value => DB::Entity::Type::int(private => 1),
  ## TagAuctionSettings 
  max_ecpm_share => DB::Entity::Type::float(private => 1),
  prop_probability_share => DB::Entity::Type::float(private => 1),
  random_share => DB::Entity::Type::float(private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  my $site = ref $args->{site_id}?
     $args->{site_id}->{site_id}: $args->{site_id};

  if ($args->{size_id} && !$args->{size_type_id})
  {
    my $size = ref($args->{size_id}) eq 'ARRAY'?
      $args->{size_id}[0]: $args->{size_id};

    ($args->{size_type_id}) =
      ref $size?
        ($size->size_type_id()):
          $ns->pq_dbh->selectrow_array(
            qq[SELECT size_type_id FROM CreativeSize WHERE size_id = $size]);
  }

  $args->{passback_type} = 'HTML_CODE'
    if !$args->{passback_type} && $args->{passback};

  my ($pub_marketplace) = 
    $ns->pq_dbh->selectrow_array(qq[
      SELECT w.pub_marketplace
      FROM walledgarden w RIGHT JOIN site s on w.pub_account_id = s.account_id
      WHERE site_id = $site ]);

  $args->{marketplace} = $pub_marketplace 
    if $pub_marketplace;

  $self->{options_} = ();

  foreach my $opt (qw(CRADVTRACKPIXEL PUBL_TAG_TRACK_PIXEL 
                      MAX_ADS_PER_TAG)) 
  {
    my $value_name = lc($opt) . "_value";
    if (exists $args->{$value_name} or $opt eq 'MAX_ADS_PER_TAG') 
    {
      push @{$self->{options_}},
        DB::Options->blank(
          token => $opt,
          value => $args->{$value_name});
      delete $args->{$value_name};
    }
  }

}

sub postcreate_
{
  my ($self, $ns) = @_;

  my $size;

  if (defined $self->{size_id})
  {
    my @sizes = ref($self->{size_id}) eq 'ARRAY'?
     @{$self->{size_id}}: ($self->{size_id});

    foreach my $s (@sizes)
    {
      $ns->create(
        DB::Tag_TagSize->blank(
         tag_id => $self->{tag_id},
         size_id => $s ));
      $size =
        ref $s eq 'CODE'?
          $s->($self)->{size_id}:
            ref $s? $s->{size_id}: $s if !$size;
    }
  }
  else
  {
    ($size) = $ns->pq_dbh->selectrow_array(qq[
      SELECT Min(size_id)
      FROM CreativeSize
      WHERE size_type_id = $self->{size_type_id} ]);
  }

  $size = DB::Defaults::instance()->size->{size_id} unless defined $size;

  my $option_group_id = 
    $ns->create(DB::OptionGroup->blank( 
      name => "Size-" . $size,
      type => "Publisher",
      size_id => $size ));

  foreach my $option (@{ $self->{options_} })
  {
    $option->{tag_id} = $self->{tag_id};
    $option->{size_id} = $size;
    $option->{option_group_id} = $option_group_id;
    $ns->create($option);
  }

  my %auction_args = ();
  foreach my $arg (qw(max_ecpm_share prop_probability_share random_share))
  {
    $auction_args{$arg} = $self->{$arg} if $self->{$arg};
  }

  if (keys %auction_args)
  {
    $auction_args{tag_id} = $self->{tag_id};
    $ns->create(
      DB::TagAuctionSettings->blank(
        %auction_args));
  }
}

use constant TAG_LEVEL_EXCLUSION  => 0x01;
use constant INVENORY_ESTIMATION  => 0x02;
use constant ENABLE_ALL_SIZES     => 0x04;

1;

# Priced tag tag with tag pricing
package DB::PricedTag;

use warnings;
use strict;

our @ISA = qw(DB::Tags);

sub _table {
    'Tags'
}

sub postcreate_
{
  my ($self, $ns) = @_;

  $self->SUPER::postcreate_($ns);
  
  my %args = ();
  $args{tag_id} = $self->{tag_id};
  $args{country_code} = $self->{country_code};
  $args{low_channel_threshold} = $self->{low_channel_threshold}
    if defined $self->{low_channel_threshold};;
  $args{high_channel_threshold} = $self->{high_channel_threshold}
    if defined $self->{high_channel_threshold};
  $args{cpm} = $self->{cpm} if defined $self->{cpm};
  $args{rate_type} = $self->{rate_type} if defined $self->{rate_type};
  $self->{tag_pricing_id} = $ns->create(
    DB::TagPricing->blank(%args));
  if (defined $self->{country_code}) 
  {
    my %args_copy = %args;
    $args_copy{country_code} = undef;    
    $args_copy{cpm} = 0;
    $self->{tag_pricing_def_id} = $ns->create(
      DB::TagPricing->blank(%args_copy));
  }
  else
  {
    $self->{tag_pricing_def_id} = $self->{tag_pricing_id};
  }

  $ns->create(
    TagCTROverride => {
      tag_id => $self->{tag_id},
      adjustment => $self->{adjustment} } );

}

1;

package DB::Tag_TagSize;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);


use constant STRUCT => 
{
  tag_id => DB::Entity::Type::link('DB::Tags', unique => 1),
  size_id => DB::Entity::Type::link('DB::CreativeSize', unique => 1)
};

1;
