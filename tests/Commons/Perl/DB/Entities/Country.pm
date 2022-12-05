
package DB::CTRAlgorithm;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::UpdateMixin DB::Entity::PQ);

use constant STRUCT => 
{
  country_code => DB::Entity::Type::string(unique => 1),
  clicks_interval1_days  => DB::Entity::Type::int(),
  clicks_interval1_weight => DB::Entity::Type::float(),
  clicks_interval2_days => DB::Entity::Type::int(),
  clicks_interval2_weight => DB::Entity::Type::float(),
  clicks_interval3_weight => DB::Entity::Type::float(),
  imps_interval1_days => DB::Entity::Type::int(),
  imps_interval1_weight => DB::Entity::Type::int(),
  imps_interval2_days => DB::Entity::Type::int(),
  imps_interval2_weight => DB::Entity::Type::float(),
  imps_interval3_weight => DB::Entity::Type::float(),
  pub_ctr_default => DB::Entity::Type::float(),
  sys_ctr_level => DB::Entity::Type::int(),
  pub_ctr_level => DB::Entity::Type::int(),
  site_ctr_level => DB::Entity::Type::int(),
  tag_ctr_level => DB::Entity::Type::int(),
  kwtg_ctr_default => DB::Entity::Type::float(),
  sys_kwtg_ctr_level => DB::Entity::Type::int(),
  keyword_ctr_level => DB::Entity::Type::int(),
  ccgkeyword_kw_ctr_level => DB::Entity::Type::int(),
  ccgkeyword_tg_ctr_level => DB::Entity::Type::int(),
  tow_raw => DB::Entity::Type::int(),
  sys_tow_level => DB::Entity::Type::int(),
  campaign_tow_level => DB::Entity::Type::int(),
  tg_tow_level => DB::Entity::Type::int(),
  keyword_tow_level => DB::Entity::Type::int(),
  ccgkeyword_kw_tow_level => DB::Entity::Type::int(),
  ccgkeyword_tg_tow_level => DB::Entity::Type::int(),
  cpc_random_imps => DB::Entity::Type::int(),
  cpa_random_imps => DB::Entity::Type::int()
};

1;

package DB::Country;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{ 
  country_id => DB::Entity::Type::raw_sequence(),
  country_code => DB::Entity::Type::string(unique => 1),
  currency_id => DB::Entity::Type::link('DB::Currency'),
  timezone_id => DB::Entity::Type::link(
    'DB::TimeZone',
    default => sub { DB::Defaults::instance()->timezone->{timezone_id} }),
  language => DB::Entity::Type::string(),
  low_channel_threshold => DB::Entity::Type::int(default => 0),
  high_channel_threshold => DB::Entity::Type::int(default => 0),
  max_url_trigger_share => DB::Entity::Type::int(default => 1),
  min_url_trigger_threshold => DB::Entity::Type::int(default => 0),
  min_tag_visibility => DB::Entity::Type::int(default => 0),
  vat_enabled => 0,
  vat_number_input_enabled => 0,
  default_payment_terms => 30,
  
  # Private
  # CTRAlgorithm
  clicks_interval1_days  => DB::Entity::Type::int(private => 1, default => 7),
  clicks_interval1_weight => DB::Entity::Type::float(private => 1, default => 2),
  clicks_interval2_days => DB::Entity::Type::int(private => 1, default => 28),
  clicks_interval2_weight => DB::Entity::Type::float(private => 1, default => 0.3),
  clicks_interval3_weight => DB::Entity::Type::float(private => 1, default => 0.05),
  imps_interval1_days => DB::Entity::Type::int(private => 1, default => 7),
  imps_interval1_weight => DB::Entity::Type::float(private => 1, default => 2),
  imps_interval2_days => DB::Entity::Type::int(private => 1, default => 28),
  imps_interval2_weight => DB::Entity::Type::float(private => 1, default => 0.3),
  imps_interval3_weight => DB::Entity::Type::float(private => 1, default => 0.05),
  pub_ctr_default => DB::Entity::Type::float(private => 1, default => 0.01),
  sys_ctr_level => DB::Entity::Type::int(private => 1, default => 2000),
  pub_ctr_level => DB::Entity::Type::int(private => 1, default => 2000),
  site_ctr_level => DB::Entity::Type::int(private => 1, default => 2000),
  tag_ctr_level => DB::Entity::Type::int(private => 1, default => 2000),
  kwtg_ctr_default => DB::Entity::Type::float(private => 1, default => 0.01),
  sys_kwtg_ctr_level => DB::Entity::Type::int(private => 1, default => 2000),
  keyword_ctr_level => DB::Entity::Type::int(private => 1, default => 2000),
  ccgkeyword_kw_ctr_level => DB::Entity::Type::int(private => 1, default => 1000),
  ccgkeyword_tg_ctr_level => DB::Entity::Type::int(private => 1, default => 1000),
  tow_raw => DB::Entity::Type::int(private => 1, default => 1),
  sys_tow_level => DB::Entity::Type::int(private => 1, default => 2000),
  campaign_tow_level => DB::Entity::Type::int(private => 1, default => 2000),
  tg_tow_level => DB::Entity::Type::int(private => 1, default => 2000),
  keyword_tow_level => DB::Entity::Type::int(private => 1, default => 2000),
  ccgkeyword_kw_tow_level => DB::Entity::Type::int(private => 1, default => 1000),
  ccgkeyword_tg_tow_level => DB::Entity::Type::int(private => 1, default => 1000),
  cpc_random_imps => DB::Entity::Type::int(private => 1, default => 0),
  cpa_random_imps => DB::Entity::Type::int(private => 1, default => 0),

  # Country GEO channel
  name => DB::Entity::Type::string(private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  $args->{language} =  lc($args->{country_code})
    unless defined $args->{language};

  $self->{low_channel_threshold_} = $args->{low_channel_threshold};
  $self->{high_channel_threshold_} = $args->{high_channel_threshold};
  $self->{language_} = $args->{language};
  $self->{currency_id_} = $args->{currency_id};

  if ( not defined $args->{timezone_id} )
  {
    $args->{timezone_id} = DB::Defaults::instance()->timezone;
    $self->{timezone_id_} = $args->{timezone_id};
  }

}

sub postcreate_
{
  my ($self, $ns) = @_;
  
  if (grep { defined $self->{$_} }  
    (qw(low_channel_threshold_ high_channel_threshold_ language_ timezone_id_ currency_id_)))
  {
    $self->__update($ns, { 
      currency_id => 
        defined $self->{currency_id_}?
          $self->{currency_id_}:
            $self->{currency_id},
      low_channel_threshold =>
        defined $self->{low_channel_threshold_}?
          $self->{low_channel_threshold_}:
            $self->{low_channel_threshold},
      high_channel_threshold => 
        defined $self->{high_channel_threshold_}?
          $self->{high_channel_threshold_}:
            $self->{high_channel_threshold},
      timezone_id =>
        defined $self->{timezone_id_}?
          $self->{timezone_id_}:
            $self->{timezone_id},
      language => 
        defined $self->{language_}?
          $self->{language_}:
            $self->{language} });
  }

  #initialize CTRAlgorithm
  {
    my @fields = keys %{ DB::CTRAlgorithm->struct };
    my $args_copy = $self->{args__};
    my %args;
    @args{ @fields } = @$args_copy{ @fields };

    $ns->create(CTRAlgorithm => \%args);
  }

  if (defined $self->{name})
  {
    $ns->create(DB::CountryChannel->blank(
      name => $self->{name},
      country_code => $self->{country_code} ));
  }
}

1;
