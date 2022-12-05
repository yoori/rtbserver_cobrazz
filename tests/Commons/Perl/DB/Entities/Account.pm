
package DB::Account::Util;

sub set_field_value
{
  my ($set_fld, $field, $field_values, $value, $default) = @_;
  return if (defined $$set_fld);
  if (not defined $field)
  {
    $$set_fld = $default;
  }
  else 
  {
    foreach my $v (@$field_values)
    {
      if ($v == $field)
      {
        $$set_fld = $value;
        return;
      }
    }
  }
}

1;

package DB::AccountRole;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::DictionaryMixin DB::Entity::PQ);

use constant STRUCT => 
{
  account_role_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::string(unique => 1)
};

1;

package DB::AccountTypeCreativeSize;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  account_type_id => 
    DB::Entity::Type::link('DB::AccountRole', unique => 1),
  size_id => 
    DB::Entity::Type::link('DB::CreativeSize', unique => 1)
};

1;


package DB::AccountType;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant WD_TAGS                          =>  0x001;
use constant ADVERTISER                       =>  0x002;
use constant INVOICING_PER_ADV                =>  0x004;
use constant FREQ_CAPS                        =>  0x008;
use constant GROSS                            =>  0x010;
use constant SITE_TARGETING                   =>  0x020;
use constant PUBLISHER_INVENTORY_ESTIMATION   =>  0x040;
use constant COMPANY_OWNERSHIP                =>  0x800;
use constant INVOICE_COMMISSION               =>  0x1000;

use constant STRUCT => 
{
  account_type_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  account_role_id => DB::Entity::Type::link( 
    'DB::AccountRole',
    default => sub { DB::Defaults::instance()->advertiser_role } ),
  flags => DB::Entity::Type::int(default => 0),
  io_management => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),
  adv_exclusions => DB::Entity::Type::enum(['D', 'S', 'T'], nullable => 1),
  adv_exclusion_approval => DB::Entity::Type::enum(['A', 'R'], nullable => 1),
  show_iframe_tag => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),
  show_browser_passback_tag => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),
  max_keyword_length => DB::Entity::Type::int(nullable => 1),
  max_url_length => DB::Entity::Type::int(nullable => 1),
  max_keywords_per_channel => DB::Entity::Type::int(nullable => 1),
  max_urls_per_channel => DB::Entity::Type::int(nullable => 1),
  max_keywords_per_group  => DB::Entity::Type::int(nullable => 1),
  auction_rate => DB::Entity::Type::enum(['N', 'G']),
  channel_check_on => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),
  campaign_check_on => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),

  # private
  creative_sizes => DB::Entity::Type::link_array('DB::CreativeSize', private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  DB::Account::Util::set_field_value(
    \$args->{io_management},
    $args->{account_role_id},
    [ DB::Defaults::instance()->advertiser_role, 
      DB::Defaults::instance()->agency_role], 'N', 'N');

 DB::Account::Util::set_field_value(
    \$args->{channel_check_on},
    $args->{account_role_id},
    [ DB::Defaults::instance()->internal_role,
      DB::Defaults::instance()->advertiser_role,
      DB::Defaults::instance()->agency_role,
      DB::Defaults::instance()->cmp_role], 'N', 'N');

 DB::Account::Util::set_field_value(
    \$args->{campaign_check_on},
    $args->{account_role_id},
    [ DB::Defaults::instance()->advertiser_role,
      DB::Defaults::instance()->agency_role], 'N', 'N');

  DB::Account::Util::set_field_value(
    \$args->{max_keyword_length},
    $args->{account_role_id},
    [ DB::Defaults::instance()->advertiser_role, 
      DB::Defaults::instance()->agency_role,
      DB::Defaults::instance()->cmp_role], 100, 100);

  DB::Account::Util::set_field_value(
    \$args->{max_url_length},
    $args->{account_role_id},
    [ DB::Defaults::instance()->advertiser_role, 
      DB::Defaults::instance()->agency_role,
      DB::Defaults::instance()->cmp_role], 1000, 1000);

  DB::Account::Util::set_field_value(
    \$args->{max_keywords_per_channel},
    $args->{account_role_id},
    [ DB::Defaults::instance()->advertiser_role,
      DB::Defaults::instance()->agency_role,
      DB::Defaults::instance()->cmp_role], 2000, 2000);

  DB::Account::Util::set_field_value(
    \$args->{max_urls_per_channel},
    $args->{account_role_id},
    [ DB::Defaults::instance()->advertiser_role,
      DB::Defaults::instance()->agency_role,
      DB::Defaults::instance()->cmp_role], 2000, 2000);

  DB::Account::Util::set_field_value(
    \$args->{max_keywords_per_group},
    $args->{account_role_id},
    [ DB::Defaults::instance()->advertiser_role, 
      DB::Defaults::instance()->agency_role], 2000, 2000);

  DB::Account::Util::set_field_value(
    \$args->{adv_exclusions},
    $args->{account_role_id},
   [ DB::Defaults::instance()->publisher_role], 'D', undef);

  DB::Account::Util::set_field_value(
    \$args->{show_iframe_tag},
    $args->{account_role_id},
   [ DB::Defaults::instance()->publisher_role], 'N', undef);

  DB::Account::Util::set_field_value(
    \$args->{show_browser_passback_tag},
    $args->{account_role_id},
   [ DB::Defaults::instance()->publisher_role], 'N', undef);
}

sub postcreate_
{
  my ($self, $ns) = @_;

  if ($self->{creative_sizes})
  {
    for my $size (@{ $self->{creative_sizes }})
    {
      $ns->create(
        DB::AccountTypeCreativeSize->blank(
          account_type_id => $self->{account_type_id},
          size_id => $size));
    }
  }
}

1;

package DB::AccountFinancialData;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  account_id => DB::Entity::Type::link('DB::Account', unique => 1),
  prepaid_amount => DB::Entity::Type::float(),
  not_invoiced =>  DB::Entity::Type::float(),
  total_adv_amount =>  DB::Entity::Type::float()
};

1;

package DB::AccountFinancialSettings;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  account_id => DB::Entity::Type::link('DB::Account', unique => 1),
  commission =>  DB::Entity::Type::float(default => 0),
  default_bill_to_user_id => DB::Entity::Type::int(nullable => 1),
  min_invoice => 25,
  billing_frequency => 'M',
  billing_frequency_offset => 2,
  type => 'A'
};

1;

package DB::AccountAddress;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  address_id => DB::Entity::Type::sequence(),
  line1 => DB::Entity::Type::name(unique => 1),
  city => DB::Entity::Type::string(nullable => 1),
  zip => DB::Entity::Type::string(nullable => 1),
  line2 => DB::Entity::Type::string(nullable => 1),
  line3 => DB::Entity::Type::string(nullable => 1),
  state => DB::Entity::Type::string(nullable => 1),
  province => DB::Entity::Type::string(nullable => 1)
};

1;

package DB::WalledGarden;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  wg_id => DB::Entity::Type::sequence(),
  pub_account_id =>  DB::Entity::Type::link('DB::Account', unique => 1),
  agency_account_id => DB::Entity::Type::link('DB::Account', unique => 1),
  pub_marketplace => DB::Entity::Type::enum(['WG', 'OIX', 'ALL']),
  agency_marketplace => DB::Entity::Type::enum(['WG', 'OIX', 'ALL']),
  version => DB::Entity::Type::pq_timestamp("timestamp 'now'")
};
    
1;

package DB::Action;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT =>
{
  action_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  account_id => DB::Entity::Type::link('DB::Account'),
  url => DB::Entity::Type::string(),
  status =>  DB::Entity::Type::status(),
  display_status_id => DB::Entity::Type::display_status("Action"),
  conv_category_id => DB::Entity::Type::int(default => 0),
  cur_value => DB::Entity::Type::float(default=>0),
  imp_window => 7,
  click_window => 30
};

1;

# Auction settings for internal account
package DB::AuctionSettings;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT =>
{
  account_id => DB::Entity::Type::link('DB::Account', unique => 1),
  max_ecpm_share => DB::Entity::Type::float(default => 0),
  prop_probability_share => DB::Entity::Type::float(default => 0),
  random_share => DB::Entity::Type::float(default => 0),
  max_random_cpm => DB::Entity::Type::float(default => 0)
};

1;

package DB::Account;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant TEST  =>  0x001;

use constant STRUCT =>
{
  account_id => DB::Entity::Type::sequence(),
  name => DB::Entity::Type::name(unique => 1),
  role_id =>
    DB::Entity::Type::link(
    "DB::AccountRole",
    default => sub { DB::Defaults::instance()->advertiser_role }),
  country_code => DB::Entity::Type::country(),
  currency_id =>  
    DB::Entity::Type::link(
      'DB::Currency',
      default => sub { DB::Defaults::instance()->currency->{currency_id} }),
  flags => DB::Entity::Type::int(default => 0),
  internal_account_id =>  
    DB::Entity::Type::link(
      'DB::Account',
      default => sub { DB::Defaults::instance()->internal_account->{account_id} },
      nullable => 1 ),
  legal_name => DB::Entity::Type::string(nullable => 1),
  account_type_id => DB::Entity::Type::link('DB::AccountType', nullable => 1),
  status =>  DB::Entity::Type::status(),
  timezone_id => 
    DB::Entity::Type::link( 
      'DB::TimeZone',  
      default => sub { DB::Defaults::instance()->timezone->{timezone_id} }),
  agency_account_id => DB::Entity::Type::link('DB::Account', nullable => 1),
  display_status_id => DB::Entity::Type::display_status('Account'),
  text_adserving => DB::Entity::Type::enum(['O', 'A', 'M'], nullable => 1),
  passback_below_fold => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),
  use_pub_pixel => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),
  pub_pixel_optin => DB::Entity::Type::string(nullable => 1),
  hid_profile => DB::Entity::Type::enum(['N', 'Y']),
  creative_reapproval => DB::Entity::Type::enum(['N', 'Y'], nullable => 1),
  message_sent => 0,

  # Private
    
  #AccountFinancialData
  prepaid_amount => DB::Entity::Type::float(default => 1_000_000_000, private => 1),
  not_invoiced => DB::Entity::Type::float(private => 1),
  total_adv_amount => DB::Entity::Type::float(private => 1),

  #AccountFinancialSettings
  commission => DB::Entity::Type::float(private => 1),
  default_bill_to_user_id => DB::Entity::Type::int(private => 1),

  #AccountType
  type_flags => DB::Entity::Type::int(private => 1),
  type_adv_exclusions => DB::Entity::Type::enum(['D', 'S', 'T'], private => 1),
  type_io_management  => DB::Entity::Type::enum(['N', 'Y'], private => 1),
  type_creative_sizes => DB::Entity::Type::link_array('DB::CreativeSize', private => 1),

  # AuctionSettings
  max_ecpm_share => DB::Entity::Type::float(private => 1),
  prop_probability_share => DB::Entity::Type::float(private => 1),
  random_share => DB::Entity::Type::float(private => 1),
  max_random_cpm => DB::Entity::Type::float(private => 1)
};

sub preinit_
{
  my ($self, $ns, $args) = @_;

  unless (exists $args->{account_type_id})
  {
    my %role_type = (
      DB::Defaults::instance()->internal_role => DB::Defaults::instance()->internal_type,
      DB::Defaults::instance()->advertiser_role => DB::Defaults::instance()->advertiser_type,
      DB::Defaults::instance()->publisher_role => DB::Defaults::instance()->publisher_type,
      DB::Defaults::instance()->isp_role => DB::Defaults::instance()->isp_type,
      DB::Defaults::instance()->agency_role => DB::Defaults::instance()->agency_type,
      DB::Defaults::instance()->cmp_role => DB::Defaults::instance()->cmp_type);

    $args->{account_type_id} = defined $args->{agency_account_id}? 
       undef: defined $args->{role_id}? $role_type{$args->{role_id}}:
         DB::Defaults::instance()->advertiser_type;
  }

  $args->{use_pub_pixel} = undef
    if not exists $args->{use_pub_pixel};

  $args->{creative_reapproval} = undef
    if not exists $args->{creative_reapproval};

  if (!defined $args->{role_id} ||
      $args->{role_id} == DB::Defaults::instance()->advertiser_role)
  {
    $args->{text_adserving} = 'A'
      if not exists $args->{text_adserving} and
        not defined $args->{agency_account_id};
  }
  elsif ($args->{role_id} == 
    DB::Defaults::instance()->agency_role)
  {
    $args->{text_adserving} = 'O'
      if not exists $args->{text_adserving};
  }
  elsif ($args->{role_id} == 
    DB::Defaults::instance()->publisher_role)
  {
    $args->{text_adserving} = undef
      if not exists $args->{text_adserving};
    $args->{use_pub_pixel} = 'N'
      if not defined $args->{use_pub_pixel};
    $args->{creative_reapproval} = 'N'
      if not defined $args->{creative_reapproval};
    $args->{prepaid_amount} = 0;
  }
  else
  {
    $args->{prepaid_amount} = 0;
  }

  DB::Account::Util::set_field_value(
    \$args->{passback_below_fold},
    $args->{role_id},
    [DB::Defaults::instance()->publisher_role], 'Y', undef);
}

sub precreate_
{
  my ($self, $ns) = @_;

  $self->{legal_name} = $self->{name} 
    if not defined $self->{legal_name};

  my @acc_type_args = 
    qw(type_flags type_adv_exclusions 
       type_creative_sizes type_io_management);

  if (defined $self->{account_type_id} &&
    grep {exists $self->{$_}} @acc_type_args)
  {
    # Need assertion to avoid new type creation,
    #  when non-default type used.

    my %args = (
      name => $self->{__name},
      account_role_id => ref $self->{role_id} eq 'CODE'?
        $self->{role_id}->($self): $self->{role_id});

    for my $fld (@acc_type_args)
    {
      (my $f = $fld) =~ s/^type_(\w+)$/$1/; 
      $args{$f} = $self->{$fld} if exists $self->{$fld};
    }

    $self->{account_type_id} =
      $ns->create(AccountType => \%args);    
  }

  die "$self->{name}:  ACCOUNT_AGENCY_TYPE_ROLE_CH constraint violated" 
    if not ((defined $self->{agency_account_id} and 
      !defined $self->{account_type_id}) || 
          (!defined $self->{agency_account_id} &&
             defined $self->{account_type_id}))
}

sub postcreate_
{
  my ($self, $ns) = @_;

  # create AccountFinancialSettings
  {
    my %args;

    $self->{default_bill_to_user_id} =
      DB::Defaults::instance()->user->{user_id}
        if !exists $self->{default_bill_to_user_id} &&
          $self->{role_id} ==  DB::Defaults::instance()->advertiser_role;

    foreach my $field (qw(account_id commission default_bill_to_user_id))
    {
      $args{$field} = $self->{$field}
        if (exists $self->{$field});
    }

    $ns->create(AccountFinancialSettings => \%args);
  }

  # Create AccountFinancialData
  {
    my %args;

    foreach my $field (qw(account_id prepaid_amount
      not_invoiced total_adv_amount))
    {
      $args{$field} = $self->{$field}
        if (exists $self->{$field});
    }

    $ns->create(AccountFinancialData => \%args);
  }

  # Create AuctionSettings
  {
    my %args;

    foreach my $field (
      qw(max_ecpm_share prop_probability_share random_share max_random_cpm))
    {
      $args{$field} = $self->{$field}
        if (exists $self->{$field});
    }

    if (keys %args)
    {
      $args{account_id} = $self->{account_id};
      $ns->create(AuctionSettings => \%args);
    }
  }

}

1;

package DB::PubAccount;

use warnings;
use strict;

our @ISA = qw(DB::Account);

sub _table {
    'Account'
}

use constant STRUCT =>
{
  %{ DB::Account->STRUCT },

  role_id => 
    DB::Entity::Type::link(
      'DB::AccountRole',
       default => sub { DB::Defaults::instance()->publisher_role }),
  account_type_id =>  
     DB::Entity::Type::link(
       'DB::AccountType',
       default => sub { DB::Defaults::instance()->publisher_type->{account_type_id} }),
  agency_account_id => undef,
  text_adserving => undef,
  passback_below_fold => DB::Entity::Type::enum(['Y', 'N']),
  use_pub_pixel => DB::Entity::Type::enum(['N', 'Y']),
  creative_reapproval => 'N',
     
  # Private
  #AccountFinancialSettings
  default_bill_to_user_id => DB::Entity::Type::int(private => 1, default => undef),

  #AccountFinancialData
  prepaid_amount => DB::Entity::Type::int(private => 1, default => 0)
};

# Do not use Account.preinit_
sub preinit_
{ }

1;


package DB::Internal;

use warnings;
use strict;

our @ISA = qw(DB::Account);

sub _table {
    'Account'
}

use constant STRUCT =>
{
  %{ DB::Account->STRUCT },

  internal_account_id => undef, 

  role_id => 
    DB::Entity::Type::link(
      'DB::AccountRole',
       default => sub { DB::Defaults::instance()->internal_role }),
  account_type_id =>  
     DB::Entity::Type::link(
       'DB::AccountType',
       default => sub { DB::Defaults::instance()->internal_type->{account_type_id} }),

  passback_below_fold => undef,

  prepaid_amount => DB::Entity::Type::int(private => 1, default => 0),
  
};

# Do not use Account.preinit_
sub preinit_
{ }

1;

package DB::Advertiser;

use warnings;
use strict;

our @ISA = qw(DB::Account);

sub _table {
    'Account'
}


use constant STRUCT =>
{
  %{ DB::Account->STRUCT },

  role_id =>  
    DB::Entity::Type::link(
      'DB::AccountRole',
       default => sub { DB::Defaults::instance()->advertiser_role }),
   account_type_id =>
     DB::Entity::Type::link(
       'DB::AccountType',
       default => sub { DB::Defaults::instance()->advertiser_type->{account_type_id} },
       nullable => 1),
   passback_below_fold => undef,
   use_pub_pixel => undef,

  #AccountType
  type_io_management  => DB::Entity::Type::enum(['N', 'Y'], private => 1)
};

# Do not use Account.preinit_
sub preinit_
{ 
  my ($self, $ns, $args) = @_;
  $args->{text_adserving} = 'A'
    if not (exists $args->{text_adserving} || defined $args->{agency_account_id});
  $args->{account_type_id} = undef if $args->{agency_account_id};
}


1;

