package OldLogsLoadingTest;

use strict;
use warnings;
use POSIX qw(strftime);
use DB::Util;
use DB::Defaults;
use Net::Domain qw(hostfqdn);

sub make_CreativeStat
{
  my $data = shift;

  my %ret = %$data;
  unless (defined $ret{colo_id}) {
    die "colo_id not defined";
  }
  unless (defined $ret{tag_id}) {
    die "tag_id not defined";
  }
  unless (defined $ret{country_code}) {
    $ret{country_code} = DB::Defaults::country();
  }
  unless (defined $ret{cc_id}) {
    $ret{cc_id} = 0;
  }
  unless (defined $ret{ccg_rate_id}) {
    die "ccg_rate_id not defined";
  }
  unless (defined $ret{colo_rate_id}){
    die "colo_rate_id not defined";
  }
  unless (defined $ret{site_rate_id}){
    die "site_rate_id not defined";
  }
  unless (defined $ret{currency_exchange_id}){
    die "currency_exchange_id not defined";
  }
  unless (defined $ret{num_shown}){
    $ret{num_shown} = 1;
  }
  unless (defined $ret{position}){
    $ret{position} = 1;
  }
  unless (defined $ret{test}){
    $ret{test} = 0;
  }

  unless (defined $ret{fraud}){
    $ret{fraud} = 0;
  }

  unless (defined $ret{delivery_threshold}){
    $ret{delivery_threshold} = 1;
  }
  
  # walled garden 1.7
  unless (defined $ret{walled_garden}){
    $ret{walled_garden} = 0;
  }

  # user_status 1.7
  unless (defined $ret{user_status}){
    $ret{user_status} = "I";
  }

  unless (defined $ret{geo_channel_id}) {
    $ret{geo_channel_id} = "-";
  }

  # device_channel_id 2.5
  unless (defined $ret{device_channel_id}) {
    $ret{device_channel_id} = "-";
  }

  # hid_profile 2.6
  unless (defined $ret{hid_profile}) {
    $ret{hid_profile} = 0;
  }

  # ctr_reset_id 3.0
  unless (defined $ret{ctr_reset_id}) {
    $ret{ctr_reset_id} = 0;
  }

  unless (defined $ret{unverified_imps}){
    $ret{unverified_imps} = 0;
  }
  unless (defined $ret{imps}){
    $ret{imps} = 0;
  }
  unless (defined $ret{clicks}){
    $ret{clicks} = 0;
  }
  unless (defined $ret{actions}){
    $ret{actions} = 0;
  }
  unless (defined $ret{virtual_imps}){
    $ret{virtual_imps} = 0;
  }
  unless (defined $ret{adv_amount}){
    $ret{adv_amount} = 0;
  }
  unless (defined $ret{pub_amount}){
    $ret{pub_amount} = 0;
  }
  unless (defined $ret{isp_amount}){
    $ret{isp_amount} = 0;
  }
  unless (defined $ret{adv_comm_amount}){
    $ret{adv_comm_amount} = 0;
  }
  unless (defined $ret{pub_comm_amount}){
    $ret{pub_comm_amount} = 0;
  }

  unless (defined $ret{adv_payable_comm_amount}) {
    $ret{adv_payable_comm_amount} = 0;
  }

  unless (defined $ret{pub_advcurrency_amount}) {
    $ret{pub_advcurrency_amount} = 0;
  }

  unless (defined $ret{isp_advcurrency_amount}) {
    $ret{isp_advcurrency_amount} = 0;
  }
  return %ret;
}

sub write_CreativeStat_3_0
{
  my ($ns, $path, $date1, $date2, $date2h, $date3,
    $datain, $num) = @_;
  my $fnum = sprintf("%08d", $num);
  my $filename = "CreativeStat.$date1" . ".000000.$fnum.1.0";
  my $file = $path . $filename;
  my $commit_file = $path . "~" . $filename . ".commit." . hostfqdn;
  my %data = make_CreativeStat($datain);

  {
    open(S, ">$file") || die "Cann't open file " . $file . "\n";
    print S "CreativeStat\t3.0\n";
    print S "$date2h\n";
    print S "$date2h\n";
    my $record = "$data{colo_id}\t$data{publisher_account_id}\t$data{tag_id}\t" .
      "$data{country_code}\t$data{adv_account_id}\t$data{campaign_id}\t" .
      "$data{ccg_id}\t$data{cc_id}\t$data{ccg_rate_id}\t$data{colo_rate_id}\t" .
      "$data{site_rate_id}\t$data{currency_exchange_id}\t" .
      "$data{delivery_threshold}\t$data{position}\t$data{num_shown}\t" .
      "$data{test}\t$data{fraud}\t$data{walled_garden}\t$data{user_status}\t" .
      "$data{geo_channel_id}\t$data{device_channel_id}\t" .
      "$data{ctr_reset_id}\t$data{hid_profile}\t$data{unverified_imps}\t$data{imps}\t" .
      "$data{clicks}\t$data{actions}\t$data{adv_amount}\t" .
      "$data{pub_amount}\t$data{isp_amount}\t$data{adv_comm_amount}\t" .
      "$data{pub_comm_amount}\t$data{adv_payable_comm_amount}\t" .
      "$data{pub_advcurrency_amount}\t$data{isp_advcurrency_amount}"; 
    print S "$record\n";
    close(S);
  }

  {
    open(S, ">$commit_file") || die "Cann't open file " . $commit_file . "\n";
    close(S);
  }

  $ns->output("CreativeStatFile$num",  $file);
  $ns->output("CreativeStatCommitFile$num",  $commit_file);
  $ns->output("CreativeStatDate$num",  $date3);
  $ns->output("CreativeStatCC$num", $data{cc_id});
  $ns->output("CreativeStatNumShown$num", $data{num_shown});
  $ns->output("CreativeStatImps$num", $data{imps});

  return $file;
}

sub init
{
  my ($self, $ns) = @_;
  my $file_path = $ns->options()->{'local_data_path'};
  my @time = gmtime();
  my $date1 = strftime("%Y%m%d.%H%M%S", @time);
  my $date2 = strftime( "%Y-%m-%d", @time);
  my $date2h = strftime( "%Y-%m-%d:%H", @time);
  my $date3 = strftime("%d-%m-%Y:%H-%M-%S", @time);
  my $date4 = strftime("%Y%m%d", @time);
  my $num_shown = 1;

  my $country_code = DB::Defaults::instance()->test_country_2->{country_code};
  my $colo_cur = $ns->create(Currency => { rate => 3 });
  my $acc = $ns->create(Account => {
    name => "isp",
    country_code => $country_code,
    currency_id => $colo_cur,
    role_id => DB::Defaults::instance()->publisher_role });
  
  my $colo = DB::Defaults::instance()->deleted_colo_isp;

  $ns->output("ColoId", $colo->{colo_id});

  ## CreativeStat logs (separated by colo id)
  my $keyword = make_autotest_name($ns, "Keyword");
  my $cpm = 10;
  my $cpc = 20;
  my $campaign = $ns->create(DisplayCampaign => {
    name => 1,
    country_code => $country_code,
    behavioralchannel_keyword_list => $keyword,
    campaigncreativegroup_cpm => 0,
    campaigncreativegroup_cpc => $cpc,
    campaigncreativegroup_cpa => 0,
    campaigncreativegroup_status => 'D',
    site_links => [{name => 1}] });
  $ns->create(
    DB::BehavioralChannel::BehavioralParameter->blank(
      channel_id => $campaign->{channel_id},
      trigger_type => "P" ));
  my $foros_min_margin = get_config($ns->pq_dbh, 'OIX_MIN_MARGIN') / 100.00;
  my $tag_id = $ns->create(PricedTag => {
    name => 1,
    site_id => $campaign->{Site}[0]->{site_id},
    cpm =>  ($cpm) * (1 - $foros_min_margin) / 2 });

  my $tag_cur = $ns->create(Currency => { rate => 2 });
  my $site_rate = $ns->create(SiteRate => {tag_pricing_id => $tag_id->{tag_pricing_id}});
  my $currency_exchange_id = 0;
  my $stmt = $ns->pq_dbh->prepare_cached(q|
    SELECT MAX(currency_exchange_id)
    FROM CurrencyExchangeRate
    WHERE currency_id=?|, undef);

  $stmt->execute(DB::Defaults::instance()->currency()->{currency_id});
  my @result_row = $stmt->fetchrow_array();
  if ( @result_row )
  {
    $currency_exchange_id = $result_row[0];
  }

 
  my %data = (
    colo_id => DB::Util::unwrap($colo->{colo_id}),
    colo_rate_id => DB::Util::unwrap($colo->{Colocation}->{colo_rate_id}),
    publisher_account_id => DB::Util::unwrap( $campaign->{Site}[0]->{account_id}),
    adv_account_id => DB::Util::unwrap($campaign->{account_id}),
    campaign_id => DB::Util::unwrap($campaign->{campaign_id}),
    ccg_id => DB::Util::unwrap($campaign->{ccg_id}),
    cc_id => DB::Util::unwrap($campaign->{cc_id}),
    ccg_rate_id => DB::Util::unwrap($campaign->{CampaignCreativeGroup}->{ccg_rate_id}),
    tag_id => DB::Util::unwrap($tag_id),
    currency_exchange_id => $currency_exchange_id,
    site_rate_id =>  DB::Util::unwrap($site_rate),
    country_code => lc($country_code),
    num_shown => $num_shown,
    imps => 2);

  write_CreativeStat_3_0($ns, $file_path, $date1, $date2, $date2h, $date3, \%data, 1);
}

1;
