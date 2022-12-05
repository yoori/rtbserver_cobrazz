
package MinUidAge;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub init {
  my ($self, $ns) = @_;

  my $passback = "http://minuidage.test.com/";

  my $publisher = 
    $ns->create(Publisher => { 
      name => "Publisher",
      pricedtag_passback => $passback });

  my %cases = (
   'MINAGE_24' => {min_uid_age => 24},
   'MINAGE_0' => {min_uid_age => 0, cpm => 100},
   'MINAGE_10000' => {min_uid_age => 10000, cpm => 200},
   'NONOPTIN' => {min_uid_age => 12, optin_status_targeting => 'YYY'},
   'TEMPORARY' => {min_uid_age => 1});

  while (my ($k, $v) = each %cases )
  {
    my $keyword = make_autotest_name($ns, "KWD-" . $k);

    my $advertiser = $ns->create(Account => {
      name => 'Advertiser-' . $k,
      role_id => DB::Defaults::instance()->advertiser_role,
      account_type_id => DB::Defaults::instance()->advertiser_type });

    my $campaign = $ns->create(DisplayCampaign => {
      name => 'Campaign-' . $k,
      account_id => $advertiser,
      optin_status_targeting => $v->{optin_status_targeting},
      min_uid_age => $v->{min_uid_age},
      campaigncreativegroup_cpm => $v->{cpm}? $v->{cpm}: 100,
      channel_id => 
        DB::BehavioralChannel->blank(
          account_id => $advertiser,
          name => 'Channel-' . $k,
          keyword_list => $keyword,
          behavioral_parameters => [
            DB::BehavioralChannel::BehavioralParameter->blank(
              trigger_type => 'P', 
              time_to => 3*24*60*60) ]),
      site_links => [{site_id => $publisher->{site_id}}] });

    $ns->output("CHANNEL/" . $k, $campaign->{channel_id});
    $ns->output("CC/" . $k, $campaign->{cc_id});
    $ns->output("KWD/" . $k, $keyword);
  }

  $ns->output("TAG", $publisher->{tag_id});
  $ns->output("PASSBACK", $passback);
  $ns->output("ALLCOLO", DB::Defaults::instance()->ads_isp->{colo_id});
  $ns->output("TESTCOLO", DB::Defaults::instance()->test_isp->{colo_id});

}

1;
