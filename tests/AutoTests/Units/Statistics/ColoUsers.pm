package ColoUsers;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub create_common_data
{
  my ($self, $ns) = @_;
  # Unexist tag
  my $stmt = $ns->pq_dbh->prepare("SELECT MAX(tag_id) FROM Tags");
  $stmt->execute();
  my ($max_tag_id) = $stmt->fetchrow_array;
  $stmt->finish();

  my $inventory_publisher = $ns->create(Publisher => {
    name => 'Pub',
    pricedtag_flags => DB::Tags::INVENORY_ESTIMATION });

  my $deleted_tag = $ns->create(PricedTag => {
    name => 'Deleted',
    site_id => $inventory_publisher->{site_id},
    status => 'D'});

  my $account = $ns->create(Account => {
      name => "tuid_case",
      role_id => DB::Defaults::instance()->advertiser_role });

  my $keyword = make_autotest_name($ns, "tuid_case");

  my $behavioral_channel = $ns->create(DB::BehavioralChannel->blank(
    name => "tuid_case",
    account_id => $account,
    keyword_list => $keyword,
    behavioral_parameters => [
      DB::BehavioralChannel::BehavioralParameter->blank(
        trigger_type => "P",
        time_to => 172800)] ));

  $ns->output("Channel", $behavioral_channel->channel_id());
  $ns->output("Keyword", $keyword);

  $ns->output("Tags/DEFAULT", DB::Defaults::instance()->tag);
  $ns->output("Tags/Inventory", $inventory_publisher->{tag_id});
  $ns->output("Tags/UNEXIST", $max_tag_id + 10000);
  $ns->output("Tags/DELETED", $deleted_tag);

  my $ad_publisher = $ns->create(Publisher => {
    name => 'Ad' });

  my $campaign = $ns->create(DisplayCampaign => {
    name => "ad",
    account_id => $account,
    campaign_flags => 0,
    campaigncreativegroup_flags => 0,
    channel_id => DB::BehavioralChannel->blank(
      name => 'adchannel',
      account_id => $account,
      keyword_list => $keyword,
      behavioral_parameters => [
        DB::BehavioralChannel::BehavioralParameter->blank(trigger_type => 'P') ] ) } );

  $ns->output("Campaign/KEYWORD", $keyword);
  $ns->output("Campaign/CC_ID", $campaign->{cc_id});
  $ns->output("Tags/AD", $ad_publisher->{tag_id});
}

sub create_test_case_data
{
  my ($self, $namespace, $prefix, $colocations) = @_;

  my $ns = $namespace->sub_namespace($prefix);

  foreach my $c (@$colocations)
  {
    my $colo = $ns->create(Isp => {
      name => 
        $c->{name}? 
          $c->{name}: $c->{oo_serving},
      colocation_optout_serving => $c->{oo_serving},
      account_hid_profile => $c->{hid_profile}? $c->{hid_profile}: 'N',
      account_timezone_id => 
        $c->{tz_name}? 
          DB::TimeZone->blank(tzname => $c->{tz_name}):
            DB::Defaults::instance()->timezone->{timezone_id},
      account_internal_account_id =>
        DB::Defaults::instance()->no_margin_internal_account->{account_id} });
    
    $ns->output(
      'COLO/' . ($c->{name}? $c->{name}: $c->{oo_serving}), 
      $colo->{colo_id});
  }
}

sub unique_users
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'UNIQUE_USERS',
     [ { oo_serving => 'NON_OPTOUT' },
       { oo_serving => 'OPTIN_ONLY' },
       { oo_serving => 'ALL' } ]);
}

sub unique_hids
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'UNIQUE_HIDS',
     [ { oo_serving => 'NONE', hid_profile => 'Y' },
       { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub create_and_last_appearance
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'CREATE',
     [ { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub non_gmt_timezone
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'NON_GMT',
     [ { oo_serving => 'ALL', hid_profile => 'Y', tz_name => 'America/Los_Angeles' } ]);
}

sub basic_assync
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'BASE_ASSYNC',
     [ { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub big_date_difference
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'BIG_DATE',
     [ { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub ad_merge
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'AD_MERGE',
     [ { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub discover_merge
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'DISCOVER',
     [ { name => 'ALL1', oo_serving => 'ALL', hid_profile => 'Y' },
       { name => 'ALL2', oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub user_pref_merge
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'USER_PREF',
     [ { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub create_date
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'CREATE_DATE',
     [ { name => 'ALL1', oo_serving => 'ALL', hid_profile => 'Y' },
       { name => 'ALL2', oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub invalid_merge
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'INVALID_MERGE',
     [ { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub optout
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'OPTOUT',
     [ { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub not_serialized
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'NOT_SERIALIZED',
     [ { oo_serving => 'ALL', hid_profile => 'Y' } ]);
}

sub pub_inventory
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'PUB_INV',
     [ { oo_serving => 'ALL', hid_profile => 'Y' },
       { oo_serving => 'NON_OPTOUT', hid_profile => 'Y' } ]);
}

sub oo_service
{
  my ($self, $ns) = @_;

  $self->create_test_case_data(
     $ns, 'OO',
     [ { name => 'ALL1', oo_serving => 'ALL' },
       { name => 'ALL2', oo_serving => 'ALL' },
       { name => 'ALL3', oo_serving => 'ALL' } ]);
}

sub init
{
  my ($self, $ns) = @_;

  $self->create_common_data($ns);
  $self->unique_users($ns);
  $self->unique_hids($ns);
  $self->create_and_last_appearance($ns);
  $self->non_gmt_timezone($ns);
  $self->basic_assync($ns);
  $self->big_date_difference($ns);
  $self->ad_merge($ns);
  $self->discover_merge($ns);
  $self->user_pref_merge($ns);
  $self->create_date($ns);
  $self->invalid_merge($ns);
  $self->optout($ns);
  $self->not_serialized($ns);
  $self->pub_inventory($ns);
  $self->oo_service($ns);
}

1;
