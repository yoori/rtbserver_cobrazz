package DB::Defaults;

use warnings;
use strict;
use DB::Entity::Oracle;
use DB::EntitiesImpl;

use vars qw(@ISA @EXPORT %EXPORT_TAGS);

use Exporter;

@ISA = qw(Exporter);

@EXPORT = qw(GLOBAL_CTR DISPLAY_STATUS_INACTIVE DISPLAY_STATUS_LIVE);

%EXPORT_TAGS = (constants => [qw(GLOBAL_CTR DISPLAY_STATUS_INACTIVE DISPLAY_STATUS_LIVE)]);

#Constants
use constant GLOBAL_CTR => 1;
use constant APP_FORMAT_NOTRACK => 'unit-test';
use constant APP_FORMAT_TRACK => 'unit-test-imp';

# Display status
use constant DISPLAY_STATUS_INACTIVE => "Inactive";
use constant DISPLAY_STATUS_LIVE => "Live";
use constant DISPLAY_STATUS_DECLINED => "Not Live - Declined";
use constant DISPLAY_STATUS_PENDING_OIX_APPROVAL =>
  "Not Live - Pending Approval by OIX";
use constant DISPLAY_STATUS_DELETED => "Deleted";
use constant DISPLAY_STATUS_NOT_ENOUGH_USERS =>
  "Not Live - Not Enough Unique Users";

sub country
{
  # GUINEA - is a default country
  $_[0]->{__default_country} ||= 
     $_[0]->{namespace}->create(
       Country => { 
         country_code => 'GN',
         currency_id => $_[0]->currency,
         low_channel_threshold => 0,
         high_channel_threshold => 0 })
}

sub ip_address
{
  # maps to GUINEA
  q(197.149.192.10)
}

sub test_country_1
{
  # LUXEMBOURG 
  $_[0]->{__test_country_1} ||= 
    $_[0]->{namespace}->create(Country => { 
      country_code => "LU",
      low_channel_threshold => 0,
      high_channel_threshold => 0 })
}

sub test_country_2
{
  # UGANDA 
  $_[0]->{__test_country_2} ||= 
    $_[0]->{namespace}->create(Country => { 
      country_code => "UG",
      low_channel_threshold => 0,
      high_channel_threshold => 0 })
}

sub cpm
{
  DB::CCGRate::DEFAULT_CPM
}

sub advertiser_role
{
  $_[0]->{__advertiser_role} ||= 
    $_[0]->{namespace}->create(
       AccountRole => { name => 'Advertiser' })
}

sub advertiser_type
{
  my $self = shift;
  $self->{__advertiser_type} ||= 
    $self->{namespace}->create(AccountType => {
      name => 'Advertiser',
      account_role_id => $self->advertiser_role,
      flags => 0 })
}

sub publisher_role
{
  $_[0]->{__publisher_role} ||= 
    $_[0]->{namespace}->create(
       AccountRole => { name => 'Publisher' })
}

sub publisher_type
{
  my $self = shift;
  $self->{__publisher_type} ||= 
    $self->{namespace}->create(AccountType => {
      name => 'Publisher',
      account_role_id => $self->publisher_role,
      flags => 0 })
}

sub isp_role
{
  $_[0]->{__isp_role} ||= 
    $_[0]->{namespace}->create(
       AccountRole => { name => 'ISP' })
}

sub isp_type
{
  my $self = shift;
  $self->{__isp_type} ||= 
    $self->{namespace}->create(AccountType => {
      name => 'ISP',
      account_role_id => $self->isp_role,
      flags => 0 })
}

sub cmp_role
{
  $_[0]->{__cmp_role} ||= 
    $_[0]->{namespace}->create(
       AccountRole => { name => 'CMP' })
}

sub cmp_type
{
  my $self = shift;
  $self->{__cmp_type} ||= 
    $self->{namespace}->create(AccountType => {
      name => 'CMP',
      account_role_id => $self->cmp_role,
      flags => 0 })
}

sub agency_role
{
  $_[0]->{__agency_role} ||= 
    $_[0]->{namespace}->create(
       AccountRole => { name => 'Agency' })
}

sub agency_type
{
  my $self = shift;
  $self->{__agency_type} ||= 
    $self->{namespace}->create(AccountType => {
      name => 'Agency',
      account_role_id => $self->agency_role,
      flags => 0 })
}

sub agency_gross
{
  my $self = shift;
  if ( not defined $self->{__agency_gross})
  {
    my $acc_type = 
        $self->{namespace}->create(AccountType => {
          name => 'AgencyGross',
          account_role_id => $self->agency_role,
          flags => DB::AccountType::GROSS });

    $self->{__agency_gross} = 
        $self->{namespace}->create(Account => {
          name => 'DefaultAgencyGross',
          role_id => DB::Defaults::instance()->advertiser_role,
          account_type_id => $acc_type });
  }
  return $self->{__agency_gross};
}

sub internal_role
{
  $_[0]->{__internal_role} ||= 
    $_[0]->{namespace}->create(
       AccountRole => { name => 'Internal' })
}

sub internal_type
{
  my $self = shift;
  $self->{__internal_type} ||= 
    $self->{namespace}->create(
      AccountType => {
        name => 'Internal',
        flags => 0,
        account_role_id => $self->internal_role });
}

sub display_template
{
  $_[0]->{__default_display_template} ||=
    $_[0]->{namespace}->create(Template => {
      name => 'DefTempl',
      template_file => 'UnitTests/banner_img_clk.html',
      flags => 0 });
}

sub text_template
{
  my $self = shift;
  local *get_ta_template = sub {
    
  my $stmt = $self->{namespace}->pq_dbh->prepare(q{ 
      SELECT TEMPLATE_ID from TEMPLATE WHERE NAME='Text' });

    $stmt->execute;
    my $res = $stmt->fetchrow_arrayref;
    $stmt->finish;
    return $res->[0] if $res;
    die "Error: Get creative template from TEMPLATE { Text }";
  };
  $self->{__default_text_template} ||= get_ta_template()
}

sub tag 
{
  my $self = shift;
  local *create_default_tag = sub {

    my $size = 
      $self->{namespace}->create(CreativeSize => { 
        name => "popup",
        width => 1,
        height => 1});

    my $publisher =
      $self->{namespace}->create(Publisher => { 
        name => "DefaultTag",
        pricedtag_size_id => $size,
        pricedtag_passback => 'http://www.unittest.com' });
    
    my $advertiser = 
      $self->{namespace}->create(Advertiser => { 
        name => "DefaultTag-Adv" });

    my $channel = $self->{namespace}->create(
      DB::BehavioralChannel->blank(
       name => 'DefaultTag',
       account_id => $advertiser,
       url_list => "http://www.act.com",
       behavioral_parameters => [
         DB::BehavioralChannel::BehavioralParameter->blank(
           trigger_type => 'U')]));

    $self->{namespace}->create(DisplayCampaign => { 
      name => "DefaultTag",
      account_id => $advertiser,
      size_id => $size,
      campaign_freq_cap_id =>
        DB::FreqCap->blank(period => 2592000),
      channel_id => $channel,
      campaigncreativegroup_flags =>
        DB::Campaign::INCLUDE_SPECIFIC_SITES |
          DB::Campaign::EXCLUDE_CLICK_URL,
      site_links => [ { site_id => $publisher->{site_id} } ]});
   
    $publisher->{tag_id}
  };
  $self->{__default_tag} ||= create_default_tag()
}

sub isp
{
  my $self = shift;
  local *create_default_colo = sub {
    my ($isp) = @_;
    my $colo_id = 1;
    my $stmt = $self->{namespace}->pq_dbh->prepare(
      "SELECT colo_id, account_id From Colocation where colo_id = $colo_id");
    $stmt->execute;
    my @default_colo = $stmt->fetchrow_array;
    $stmt->finish;
    if (@default_colo)
    {
      ($isp->{colo_id}, $isp->{account_id}) = @default_colo;
    }
    else
    {
      $isp->{account_id} = 
        $self->{namespace}->create(Account => { 
          name => "Default-Colo-Account-$colo_id",
          role_id => $self->isp_role });

      $isp->{colo_id} = 
         $self->{namespace}->create(Colocation => { 
           name => "Default-$colo_id",
           colo_id => $colo_id,
           account_id => $isp->{account_id},
           revenue_share => 0.5});
    }
  };

  if (not defined $self->{__default_isp})
  {
    $self->{__default_isp} = {};
    create_default_colo($self->{__default_isp});
  }
  $self->{__default_isp}
}

sub internal_account
{
  my $self = shift;
  $self->{__internal_account} ||= 
    $self->{namespace}->create(Account => { 
      name => "Internal",
      legal_name => "Internal",
      country_code => $self->country()->{country_code},
      currency_id => $self->currency,
      role_id => $self->internal_role,
      account_type_id => $self->internal_type,
      status => 'A',
      flags => 0,
      internal_account_id => undef,
      timezone_id => $self->timezone,
      default_bill_to_user_id => undef,
      display_status_id =>
        $self->live_display_status("Account") })
}

sub publisher_account
{
  $_[0]->{__publisher_account} ||= 
    $_[0]->{namespace}->create(PubAccount => { 
      name => "Default-Publisher"});
}

sub advertiser
{
  $_[0]->{__advertiser} ||= 
    $_[0]->{namespace}->create(Advertiser => { 
      name => "Default-Advertiser"});
}

sub user
{
  my $self = shift;
  if ( not defined $self->{__default_user})
  {
    my $acc = 
      $self->{namespace}->create(Account => { 
        internal_account_id => $self->internal_account,
        name => "Default-User",
        legal_name => "Default-User",
        country_code => $self->country()->{country_code},
        currency_id => $self->currency,
        flags => 0,
        role_id => $self->publisher_role,
        account_type_id => $self->publisher_type,
        flags => 0,
        status => 'A',
        use_pub_pixel => 'N',
        prepaid_amount => 1_000_000_000,
        timezone_id => $self->timezone,
        passback_below_fold => 'Y',
        default_bill_to_user_id => undef,
        display_status_id =>
          $self->live_display_status("Account")});

    my $user_role = 
      $self->{namespace}->create(UserRole => { 
        name => "Advertiser Account Administrator",
        account_role_id => $self->advertiser_role });

    $self->{__default_user} = 
      $self->{namespace}->create(Users => { 
        ldap_dn => 'Default',
        first_name => 'AutoTest',
        last_name => 'AutoTest',
        status => 'A',
        account_id => $acc,
        email => $self->{database}->_namespace . "\@fake.com",
        phone => '1-2-3',
        user_role_id => $user_role,
        language => "EN",
        auth_type => "NONE" });
  }

  $self->{__default_user}
}

sub remote_isp
{
  $_[0]->{__remote_isp} ||=
    $_[0]->{namespace}->create(Isp => {
      name => "RemoteISPColo",
      account_timezone_id => 
        DB::TimeZone->blank(tzname => 'Europe/Moscow') })
}


sub optin_only_isp
{
  $_[0]->{__optin_only_isp} ||=
    $_[0]->{namespace}->create(Isp => {
      name => "OPTIN_ONLY",
      colocation_optout_serving => 'OPTIN_ONLY' })
}

sub ads_isp
{
  $_[0]->{__ads_isp} ||=
    $_[0]->{namespace}->create(Isp => {
      name => "ALL",
      colocation_optout_serving => 'ALL' })
}

sub non_optout_isp
{
  $_[0]->{__non_optout_isp} ||=
    $_[0]->{namespace}->create(Isp => {
      name => "NON_OPTOUT" })
}

sub no_ads_isp
{  
  $_[0]->{__no_ads_isp} ||=
    $_[0]->{namespace}->create(Isp => {
      name => "NONE",
      colocation_optout_serving => 'NONE' })
}

sub deleted_colo_isp
{
  $_[0]->{__deleted_colo_isp} ||=
    $_[0]->{namespace}->create(Isp => {
      name => "Inactive",
      colocation_status => 'D' })
}

sub test_isp
{
  $_[0]->{__test_isp} ||=
    $_[0]->{namespace}->create(Isp => { 
      name => "Test-Colo",
      account_flags => DB::Account::TEST,
      colocation_revenue_share => 0.1})
}

sub openrtb_isp
{
  $_[0]->{__openrtb_isp} ||=
    $_[0]->{namespace}->create(Isp => { 
      name => "OpenRtb-Colo",
      colocation_optout_serving => 'ALL',
      colocation_revenue_share => 0.5})
}

sub no_margin_internal_account
{
  my $self = shift;
  $self->{__no_margin_internal_account} ||=
    $self->{namespace}->create(Account => { 
      name => "NoMarginInternal",
      account_type_id => $self->internal_type,
      role_id => $self->internal_role,
      internal_account_id => undef })
}

sub no_margin_isp
{
  my $self = shift;
  $self->{__no_margin_isp} ||= 
    $self->{namespace}->create(Isp => {
      name => "NO_MARGIN",
      account_internal_account_id => 
        $self->no_margin_internal_account })
}

sub currency
{
 $_[0]->{__default_currency} ||= 
     $_[0]->{namespace}->create(
        Currency => { rate => 1 })
}

sub timezone
{
  $_[0]->{__default_timezone} ||= 
    $_[0]->{namespace}->create(
      TimeZone => { tzname => 'GMT' })
}

sub other_size_type
{
  $_[0]->{__other_size_type} ||=
    $_[0]->{namespace}->create(SizeType => {
      name => "Other"});
}

sub size
{
  $_[0]->{__size_468x60d} ||= 
    $_[0]->{namespace}->create(CreativeSize => {
      name => "468x60",
      max_text_creatives => 2 })
}

sub size_300x250
{
  $_[0]->{__size_300x250} ||= 
    $_[0]->{namespace}->create(CreativeSize => {
      name => "300x250",
      width => 300,
      height => 250 });
}

sub text_size
{
  $_[0]->{__text_size} ||=
     $_[0]->{namespace}->create(CreativeSize => {
      name => "Text",
      width => undef,
      height => undef })
}

sub app_format_no_track
{
  $_[0]->{__app_format_no_track} ||=
     $_[0]->{namespace}->create(
        DB::AppFormat->blank(
          name => APP_FORMAT_NOTRACK,
          mime_type => 'text/html'))
}

sub app_format_track
{
  $_[0]->{__app_format_track} ||=
     $_[0]->{namespace}->create(
        DB::AppFormat->blank(
          name => APP_FORMAT_TRACK,
          mime_type => 'text/html'))
}

sub html_format
{
  $_[0]->{__html_format} =
     $_[0]->{namespace}->create(
        DB::AppFormat->blank(
          name => 'html',
          mime_type => 'text/html'))
}

sub js_format
{
  $_[0]->{__js_format} =
     $_[0]->{namespace}->create(
        DB::AppFormat->blank(
          name => 'js',
          mime_type => 'text/javascript'))
}

sub creative_category_type
{
  $_[0]->{__creative_category_type} ||=
     $_[0]->{namespace}->create(
       DB::CreativeCategoryType->blank( name => 'Visual'))
}

sub content_category
{
  $_[0]->{__content_category} ||=
    $_[0]->{namespace}->create(
       DB::CreativeCategory->blank(
        name => 'DefaultContent',
        cct_id => DB::CreativeCategoryType->blank( name => 'Content')))
}

sub openx_connector
{
  $_[0]->{__openx_connector} ||=
    $_[0]->{namespace}->create(
       DB::RTBConnector->blank( name => 'OPENX'))
}

sub tanx_connector
{
  $_[0]->{__tanx_connector} ||=
    $_[0]->{namespace}->create(
       DB::RTBConnector->blank( name => 'TANX'))
}

sub iab_connector
{
  $_[0]->{__iab_connector} ||=
    $_[0]->{namespace}->create(
       DB::RTBConnector->blank( name => 'IAB'))
}

sub allyes_connector
{
  $_[0]->{__allyes_connector} ||=
    $_[0]->{namespace}->create(
       DB::RTBConnector->blank( name => 'ALLYES'))
}

sub baidu_connector
{
  $_[0]->{__baidu_connector} ||=
    $_[0]->{namespace}->create(
       DB::RTBConnector->blank( name => 'BAIDU'))
}

sub default_ctr
{
  return 0.01;
}

sub text_option_group
{
  my $self = shift;
   $self->{__text_option_group} ||=
     $self->{namespace}->create(OptionGroup => {
      name => $self->text_template,
      template_id => $self->text_template })
}

sub no_adv_channel
{
  $_[0]->{__no_adv_channel} ||=
    $_[0]->{namespace}->create(
      DB::NoAdvChannel->blank(
        keyword_list => 
         "drugs\n" .
         "sport box banned channel",
        url_list => 
          "http://alcoholnews.org\n" .
          "opt-out-guns.com",
       url_kwd_list => "bannedAdvURLKeyword"))
}

sub no_track_channel
{
  $_[0]->{__no_track_channel} ||=
    $_[0]->{namespace}->create(
      DB::NoTrackChannel->blank(
        keyword_list => 
         "murder\n" .
         "sport box banned channel",
        url_list => "http://www.tobacco.org",
        url_kwd_list => "bannedTrackURLKeyword"))
}

sub device_channel
{
  my $self = shift;
  if (not defined $self->{__device_channel})
  {
    my $platform = $self->{namespace}->create(
      DB::DeviceChannel::Platform->blank(
        name => 'Windows',
        type => 'OS'));

    $self->{__device_channel} = $self->{namespace}->create(
      DB::DeviceChannel->blank(
        name => 'Test',
        expression => $platform->platform_id ));
  }
  return $self->{__device_channel}
}

sub geo_country
{
  $_[0]->{__geo_country} ||=
      $_[0]->{namespace}->create(
      DB::CountryChannel->blank(
        name => 'Guinea' ));
}

sub geo_gb_country
{
  $_[0]->{__geo_gb_country} ||=
      $_[0]->{namespace}->create(
      DB::CountryChannel->blank(
        name => 'United Kingdom',
        country_code => 'GB' ));
}

sub geo_ru_country
{
  $_[0]->{__geo_ru_country} ||=
      $_[0]->{namespace}->create(
      DB::CountryChannel->blank(
        name => 'Russia',
        country_code => 'RU' ));
}

sub geo_fr_country
{
  $_[0]->{__geo_fr_country} ||=
      $_[0]->{namespace}->create(
      DB::CountryChannel->blank(
        name => 'France',
        country_code => 'FR' ));
}

sub geo_us_country
{
  $_[0]->{__geo_us_country} ||=
      $_[0]->{namespace}->create(
      DB::CountryChannel->blank(
        name => 'USA',
        country_code => 'US' ));
}

sub geo_hk_country
{
  $_[0]->{__geo_hk_country} ||=
      $_[0]->{namespace}->create(
      DB::CountryChannel->blank(
        name => 'Hong Kong',
        country_code => 'HK' ));
}

sub geo_channel
{
  $_[0]->{__geo_channel} ||=
    $_[0]->{namespace}->create(
      DB::GEOChannel->blank(
        name => 'Fria',
        parent_channel_id => $_[0]->geo_country->{channel_id},
        geo_type => 'STATE'));
}

sub geo_location
{
  'gn/' . $_[0]->geo_channel->{name}
}

sub live_display_status
{
  my ($self, $object) = @_;
  my $class = "DB::$object";

  $class->get_display_status(
    $self->{namespace},  DISPLAY_STATUS_LIVE)
}

sub wdtag_mapping_optin
{
  $_[0]->{__wdtag_mapping_optin} ||=
    $_[0]->{namespace}->create(WDRequestMapping => { 
      name => 'autotest-WDTag-optin',
      description => 'WDTag mapping recommendations',
      request => 'g1.r=1'})
}

sub wdtag_mapping_optout
{
  $_[0]->{__wdtag_mapping_optout} ||=
    $_[0]->{namespace}->create(WDRequestMapping => { 
      name => 'autotest-WDTag-optout',
      description => 'WDTag mapping random',
      request => 'g1.r=0'})
}


sub rnd_xy_lang_mapping
{
 $_[0]->{__rnd_xy_lang_mapping} ||=
    $_[0]->{namespace}->create(WDRequestMapping => { 
      name => 'autotest-rnd-news-with-xy-lang',
      description => 'request for random news with xy language',
      request => 'g1.l=xy&g1.r=0'})
}

sub discover_template
{
 $_[0]->{__discover_template} ||=
    $_[0]->{namespace}->create(Template => {
      name=> 'Default-Discover-Template',
      status => 'A',
      template_type => 'DISCOVER'})
}

sub global_fcap
{
  my $self = shift;
  if (not defined $self->{__global_fcap})
  {
    my $fc = 
      $self->{namespace}->create(FreqCap => { 
        period => 30 } );

    $self->{__global_fcap} = 
      $self->{namespace}->create(AdsConfig => {
        param_name => 'GLOBAL_FCAP_ID',
        param_value => $fc->freq_cap_id });

  }
  $self->{__global_fcap}
}

sub tanx_account
{
  my $self = shift;
  if (not defined $self->{__tanx_account})
  {
    my $type =
      $self->{namespace}->create(AccountType => {
        name => 'Company-Web-Publisher-Banners',
        account_role_id => $self->publisher_role,
        flags => 
            DB::AccountType::WD_TAGS |
            DB::AccountType::FREQ_CAPS |
            DB::AccountType::PUBLISHER_INVENTORY_ESTIMATION });

    $self->{__tanx_account} =
        $self->{namespace}->create(Account => { 
          name => 'Tanx-Publisher',
          account_type_id => $type,
          role_id => $self->publisher_role,
          country_code => 'GN',
          internal_account_id => $self->internal_account,
          timezone_id => DB::TimeZone->blank(tzname => 'Asia/Shanghai'),
          currency_id => DB::Currency->blank(rate => 10) });
  }
  $self->{__tanx_account}
}

sub openrtb_currency
{
  $_[0]->{__openrtb_currency} ||=
      $_[0]->{namespace}->create(DB::Currency->blank(rate => 35.2));
}

sub openrtb_account
{
  my $self = shift;
  if (not defined $self->{__openrtb_account})
  {
    $self->{__openrtb_account} =
        $self->{namespace}->create(Account => { 
          name => 'RTB-Publisher',
          account_type_id => $self->tanx_account->{account_type_id},
          role_id => $self->publisher_role,
          country_code => 'GN',
          internal_account_id => $self->internal_account,
          currency_id => $self->openrtb_currency });
  }
  $self->{__openrtb_account}
}

sub baidu_account
{
  $_[0]->{__baidu_account} ||= 
    $_[0]->{namespace}->create(PubAccount => { 
      name => 'Baidu-Publisher',
      country_code => 'GN',
      internal_account_id => $_[0]->internal_account,
      timezone_id => DB::TimeZone->blank(tzname => 'Asia/Shanghai'),
      currency_id => DB::Currency->blank(rate => 10) });
}

sub openx_account
{
  $_[0]->{__openx_account} ||=
    $_[0]->{namespace}->create(PubAccount => {
      name => 'OpenX-Publisher',
      country_code => 'GN',
      internal_account_id => $_[0]->internal_account,
      timezone_id => DB::TimeZone->blank(tzname => 'Europe/Moscow'),
      currency_id => DB::Currency->blank(rate => 65) });
}

sub allyes_account
{
  $_[0]->{__allyes_account} ||=
    $_[0]->{namespace}->create(PubAccount => {
      name => 'Allyes-Publisher',
      country_code => 'GN',
      internal_account_id => $_[0]->internal_account,
      timezone_id => DB::TimeZone->blank(tzname => 'Asia/Shanghai'),
      currency_id => DB::Currency->blank(rate => 10) });
}

sub initialize
{
  my $self = shift;
  ($self->{database}) = @_;
  $self->{namespace} = $self->{database}->namespace('Global');

  # Create countries, using in the tests with default settings
  $self->{namespace}->create(Country => { 
    country_code => 'US',
    language => 'en', 
    low_channel_threshold => 0,
    high_channel_threshold => 0 });
  $self->{namespace}->create(Country => { country_code => 'RU' });
  $self->{namespace}->create(Country => { country_code => 'KR' });
  $self->{namespace}->create(Country => 
    { country_code => 'GB',
      language => 'en',
      low_channel_threshold => 10,
      high_channel_threshold => 20 });

 $self->{namespace}->create(Country => 
    { country_code => 'CN',
      language => 'zn',
      low_channel_threshold => 0,
      high_channel_threshold => 0 });

  $self->{namespace}->create(Country => { country_code => 'FR' });
  $self->{namespace}->create(Country => { country_code => 'HK' });
  $self->{namespace}->create(Country => { country_code => 'TF' });

  my @template_files = (
    [ 
      $self->text_template, $self->size(), 'UnitTests/textad.xsl', 
      $self->app_format_no_track(), 'X' 
    ],
    [ 
      $self->text_template, $self->size(), 'UnitTests/textad.xsl', 
      $self->app_format_track(), 'X' 
    ],
    [ 
      $self->text_template, $self->text_size, 'UnitTests/textad.xsl', 
      $self->app_format_no_track(), 'X' 
    ],
    [ 
      $self->text_template, $self->text_size, 'UnitTests/textad.xsl', 
      $self->app_format_track(), 'X' 
    ],
    [ 
      $self->text_template, $self->text_size, 'UnitTests/textad.xsl', 
      $self->html_format(), 'X' 
    ],
    [ 
      $self->display_template, $self->size_300x250, 'UnitTests/banner_img_clk.html', 
      $self->app_format_no_track(), 'T' 
    ],
    [ 
      $self->display_template, $self->size, 'UnitTests/img_clk.foros-ui', 
      $self->app_format_no_track(), 'T' 
    ],
    [ 
      $self->display_template, $self->size, 'UnitTests/img_clk.foros-ui', 
      $self->app_format_track(), 'T' 
    ],
    # for RTB's
    [ 
      $self->display_template, $self->size, 'UnitTests/banner_img_clk.html',
      $self->html_format(), 'T' 
    ],
);

  foreach my $tfile (@template_files)
  {
    my ($template, $size, $filename, $format, $type) = @$tfile;
    $self->{namespace}->create(TemplateFile => {
      template_id => $template,
      size_id => $size,
      template_file => $filename,
      flags => $format == $self->app_format_track()?  
        DB::TemplateFile::PIXEL_TRACKING: 0,
      app_format_id => $format,
      template_type => $type});
  }

  $self->{namespace}->create(WDRequestMapping => { 
    name => 'autotest-global-1',
    description => 'autotests mapping 1',
    request => 'g1.r=1&g1.s=e'});

  $self->{namespace}->create(WDRequestMapping => { 
    name => 'autotest-global-2',
    description => 'autotests mapping 2',
    request => 'g1.r=1&g1.s=r'});

  $self->{namespace}->create(WDRequestMapping => { 
    name => 'autotest-global-3',
    description => 'autotests mapping 3',
    request => 'g1.r=1&g1.s=t'});

  # ISPs
  $self->{namespace}->output("RemoteISPColo", $self->remote_isp->{colo_id});
  $self->{namespace}->output("OptoutColo", $self->no_ads_isp->{colo_id});

  # RTB accounts
  $self->{namespace}->output("TanxAccount", $self->tanx_account);
  $self->{namespace}->output("OpenRtbAccount", $self->openrtb_account);
  $self->{namespace}->output("BaiduAccount", $self->baidu_account);
  $self->{namespace}->output("OpenxAccount", $self->openx_account);
  $self->{namespace}->output("OpenRtbColo", $self->openrtb_isp->{colo_id});
}

sub close
{
  undef $_[0]->{namespace};
}

my $__instance;

sub instance {
  $__instance ||= DB::Defaults->__new;
}

sub __new
{
  my $class = shift;
  return bless {}, $class;
}

1;

