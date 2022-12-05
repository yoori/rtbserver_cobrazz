#include "PublisherInventoryTest.hpp"
 
REFLECT_UNIT(PublisherInventoryTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  static const size_t ITERS = 100; // must be >4 for correct base scenario working
  const int DAY = 24 * 60 * 60;

  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::DebugInfo::SelectedCreativesList SelectedCreativesList;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::Money Money;

}

void PublisherInventoryTest::add_publisher_inventory_stat(
  const char* description,
  unsigned int tag_id,
  double cpm,
  int imps,
  int requests,
  double revenue,
  AutoTest::Time sdate)
{
  AutoTest::Time date = sdate == Generics::Time::ZERO ? debug_time_ : sdate;
  PublisherInventory stat;
  stat.key().pub_sdate(date).tag_id(tag_id).cpm(cpm);
  stat.description(description);
  stat.select(pgconn_);

  publisher_inventory_stats_.push_back(stat);
  publisher_inventory_diffs_.push_back(Diff().
    imps(imps).
    requests(requests).
    revenue(
      ORM::stats_diff_type(
        revenue, 0.00001)));
}

void PublisherInventoryTest::add_hourly_stat(
  const char* description,
  unsigned int pub_account,
  unsigned int tag_id,
  int requests)
{
  HourlyStat stat(HourlyStat::Key().pub_account_id(pub_account).tag_id(tag_id));
  stat.description(description);
  stat.select(pgconn_);
  hourly_stats_.push_back(stat);

  hourly_diffs_.push_back(HourlyDiff().requests(requests));
}

void PublisherInventoryTest::add_site_user_stat_(
  const char* description,
  unsigned int site_id,
  int unique_users,
  bool set_lad_param,
  const AutoTest::Time& last_appearance_date)
{
  SiteUserStat::Key key;
  key.site_id(site_id);
  if (set_lad_param)
  { key.last_appearance_date(last_appearance_date); }
  SiteUserStat stat(key);
  stat.description(description);
  stat.select(pgconn_);
  site_user_stats_.push_back(stat);

  site_user_diffs_.push_back(SiteUserDiff().unique_users(unique_users));
}

void PublisherInventoryTest::add_channel_inv_stat_(
    const char* description,
    unsigned long channel_id,
    int hits,
    int active_users,
    int total_users)
{
  ChannelInventoryStat stat(
    ChannelInventoryStat::Key().
    channel_id(channel_id));
  stat.description(description);
  stat.select(pgconn_);
  channel_inventory_stats_.push_back(stat);

  channel_inventory_diffs_.push_back(ChannelInventoryDiff().
                                      hits(hits).
                                      active_users(active_users).
                                      total_users(total_users));
}

void
PublisherInventoryTest::base_scenario(AutoTest::AdClient& client,
                                      unsigned int tag_id,
                                      double pub_rate,
                                      unsigned int colo_id)
{
  std::string description("Base test case");
  add_descr_phrase(description);

  std::string keyword1 = fetch_string("BaseCase/Channel1/KEYWORD#1");
  std::string url1 = fetch_string("BaseCase/Channel1/URL#1");
  std::string keyword2 = fetch_string("BaseCase/Channel2/KEYWORD#1");
  std::string url2 = fetch_string("BaseCase/Channel2/URL#1");

  double cpm1_threshold =
    100 * fetch_float("BaseCase/NetCampaign1/CPM") / adv_rate_ ;
  double cpm1 = cpm1_threshold * pub_rate / 100;

  double cpm2_threshold =
    100 * (fetch_float("BaseCase/NetCampaign2/CPM") / adv_rate_);
  double cpm2 = cpm2_threshold * pub_rate / 100;

  std::string cc1 = fetch_string("BaseCase/NetCampaign1/CCID");
  std::string cc2 = fetch_string("BaseCase/NetCampaign2/CCID");

  AutoTest::Time moscow_today(
    debug_time_.
    get_gm_time().
    format("%d-%m-%Y:12-00-00"));

  AutoTest::Time moscow_tomorrow(
    debug_time_.
    get_gm_time().
    format("%d-%m-%Y:23-00-00"));

  AutoTest::Time tomorrow(moscow_today + 24*60*60);

  add_publisher_inventory_stat(description.c_str(), tag_id, cpm1, ITERS - ITERS / 4, 0, (ITERS - ITERS / 4) * cpm1 / 1000, moscow_today);
  add_publisher_inventory_stat(description.c_str(), tag_id, cpm1, ITERS / 4, 0, (ITERS / 4) * cpm1 / 1000, tomorrow);
  add_publisher_inventory_stat(description.c_str(), tag_id, cpm2, ITERS / 2, 0, (ITERS / 2) * cpm2 / 1000, moscow_today);
  add_publisher_inventory_stat(description.c_str(), tag_id, 0, 0, 2 * ITERS - ITERS / 4, 0);
  add_publisher_inventory_stat(description.c_str(), tag_id, 0, 0, ITERS / 4, 0, tomorrow);

  NSLookupRequest request;
  request.tid = tag_id;
  request.tag_inv = 1;
  request.debug_time =  moscow_today;
  if (colo_id) {
    request.colo = colo_id;}
  
  for (size_t i = 0; i < ITERS; ++i)
  {
    request.referer_kw = keyword1;
    request.referer = keyword1 + ".com";
    if (i % 4 == 3)
      request.debug_time =  moscow_tomorrow;

    client.process_request(request, ("request for " + cc1 + " creative").c_str());
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc1,
        client.debug_info.ccid).check(),
      "must got expected ccid");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        204,
        client.req_status()).check(),
      "Server must return 'no content' status");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        Money(cpm1_threshold),
        client.debug_info.cpm_threshold_value).check(),
      "must get expected cpm_threshold in debug-info");

    request.debug_time = moscow_today;

    if (i % 2 == 1)
    {
      request.referer_kw = keyword2;
      request.referer = keyword2 + ".com";
      client.process_request(request, ("request for " + cc2 + " creative").c_str());
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          cc2,
          client.debug_info.ccid).check(),
        "must get expected ccid");
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          204,
          client.req_status()).check(),
        "Server must return 'no content' status");
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          Money(cpm2_threshold),
          client.debug_info.cpm_threshold_value).check(),
      "must get expected cpm_threshold in debug-info");
    }
    else
    {
      request.referer_kw.clear();
      request.referer.clear();
      client.process_request(request, "Request for null creative");
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client.debug_info.selected_creatives.empty()),
        "must get empty ccid");
    }
  }
}

void
PublisherInventoryTest::ta_campaigns_scenario()
{
  std::string description("Text Advertising creatives in inventory tag test case");
  add_descr_phrase(description);

  std::string keyword = fetch_string("TACampaignsCase/BChannel/KEYWORD#1");
  std::string display_keyword = fetch_string("BaseCase/Channel1/KEYWORD#1");
  unsigned int tag_id = fetch_int("TACampaignsCase/Publisher/TAG_ID");

  double ecpm1 = fetch_float("TACampaignsCase/TACampaign1/CPC") * ctr_ * 100000 / adv_rate_ ;
  double ecpm2 = fetch_float("TACampaignsCase/TACampaign2/CPC") * ctr_ * 100000 / adv_rate_;
  double ecpm3 = fetch_float("TACampaignsCase/TACampaign3/CPC") * ctr_ * 100000 / adv_rate_;

  double cpm_threshold = ecpm1 + ecpm2 + ecpm3;

  double cpm = cpm_threshold * pub_rate_ / 100;

  add_publisher_inventory_stat(
    description.c_str(), tag_id,
    cpm, ITERS, 0, ITERS * cpm / 1000);
  
  add_publisher_inventory_stat(
    description.c_str(), tag_id,
    0, 0, ITERS, 0);

  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.referer_kw = keyword + "," + display_keyword;
  request.debug_time = debug_time_;

  std::string exp_ccids[] = {
    fetch_string("TACampaignsCase/TACampaign2/CCID"),
    fetch_string("TACampaignsCase/TACampaign1/CCID"),
    fetch_string("TACampaignsCase/TACampaign3/CCID")
  };

  client.process_request(request,
      "request for making keyword context");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.debug_info.selected_creatives.empty()),
    "must get empty cc ids");

  request.tid = tag_id;
  request.tag_inv = 1;

  for (size_t i = 0 ; i < ITERS; ++i)
  {
    client.process_request(request,
      "request for text creatives");
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "must get expected cc ids in expected order!");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        204,
        client.req_status()).check(),
      "Server must return 'no content' status");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        Money(cpm_threshold),
        client.debug_info.cpm_threshold_value).check(),
      "must get expected cpm_threshold in debug-info");
  }
}

void
PublisherInventoryTest::pub_adv_commission_scenario(unsigned int tag, double pub_rate)
{
  std::string description("Publisher and advertiser commission test case");
  add_descr_phrase(description);
  std::string keyword = fetch_string("GROSSCampaignsCase/Channel/KEYWORD#1");
  std::string cc = fetch_string("GROSSCampaignsCase/GrossCampaign/CCID");

  double cpm_threshold =
    100 * fetch_float("GROSSCampaignsCase/GrossCampaign/CPM") *
      (1 - fetch_float("GROSSCampaignsCase/GrossCampaign/COMMISSION")) /
      adv_rate_;
  double cpm = cpm_threshold / 100 * pub_rate;

  add_publisher_inventory_stat(description.c_str(), tag, cpm, ITERS, 0, ITERS * cpm / 1000);
  add_publisher_inventory_stat(description.c_str(), tag, 0, 0, ITERS, 0);

  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.referer_kw = keyword;
  request.tid = tag;
  request.tag_inv = 1;
  request.debug_time = debug_time_;

  for (size_t i = 0; i < ITERS; ++i)
  {
    client.process_request(request,
      ("request for " + cc + " creative").c_str());
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc,
        client.debug_info.ccid).check(),
      "must get expected ccid");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        204,
        client.req_status()).check(),
      "Server must return 'no content' status");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        Money(cpm_threshold),
        client.debug_info.cpm_threshold_value).check(),
      "must get expected cpm_threshold in debug-info");
  }
}

// Related to ADSC-5384 	
// For more details see
// https://confluence.ocslab.com/display/TDOCDRAFT/REQ-131+Inventory+Estimation+for+Publishers
// (Ad Server Request section)
void
PublisherInventoryTest::virtual_scenario()
{
  // Campaign with FrecCaps.life_count = 2 is used for this test
  std::string description("Virtual frquency caps preserved test case (REQ-1975)");
  add_descr_phrase(description);

  std::string keyword = fetch_string("VirtualCampaignCase/Channel/KEYWORD#1");
  std::string cc = fetch_string("VirtualCampaignCase/VirtualCampaign/CCID");
  unsigned long tag1 = fetch_int("VirtualCampaignCase/VirtPublisher1/TAG_ID");
  unsigned long tag2 = fetch_int("VirtualCampaignCase/VirtPublisher2/TAG_ID");

  double cpm_threshold =
    100 * fetch_float("VirtualCampaignCase/VirtualCampaign/CPM");

  double cpm = cpm_threshold / 100;

  add_publisher_inventory_stat(description.c_str(), tag1, cpm, 1, 0, cpm / 1000);
  add_publisher_inventory_stat(description.c_str(), tag1, 0, 0, 2, 0);
  add_publisher_inventory_stat(description.c_str(), tag2, cpm, 0, 0, 0);
  add_publisher_inventory_stat(description.c_str(), tag2, 0, 0, 0, 0);

  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.referer_kw = keyword;
  request.tid = tag2;
  request.debug_time = debug_time_;
  client.process_request(request);
//  request.debug_time = debug_time_;
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc,
      client.debug_info.ccid).check(),
    "must get expected ccid");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      Money(cpm_threshold),
      client.debug_info.cpm_threshold_value).check(),
    "must get expected cpm_threshold in debug-info");

  request.tid = tag1;
  request.tag_inv = 1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc,
      client.debug_info.ccid).check(),
    "must get expected ccid");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      Money(cpm_threshold),
      client.debug_info.cpm_threshold_value).check(),
    "must get expected cpm_threshold in debug-info");

  client.repeat_request();
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "mustn't get expected ccid");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      0,
      client.debug_info.cpm_threshold_value).check(),
    "must get zero cpm_threshold in debug-info");

  request.tid = tag2;
  request.tag_inv.clear();
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc,
      client.debug_info.ccid).check(),
    "must get expected ccid");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cpm_threshold,
      client.debug_info.cpm_threshold_value).check(),
    "must get expected cpm_threshold in debug-info");

  client.repeat_request();
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "mustn't get expected ccid");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      0,
      client.debug_info.cpm_threshold_value).check(),
    "must get zero cpm_threshold in debug-info");
}

void
PublisherInventoryTest::no_impression_scenario()
{
  std::string description("Test 3.4. Logging without impression test case");
  add_descr_phrase(description);

  unsigned int tag = fetch_int("NoImpCase/NoImpPublisher/TAG_ID");

  add_publisher_inventory_stat(description.c_str(), tag, 0, 0, 1, 0);

  AdClient client(AdClient::create_user(this));

  client.process_request(NSLookupRequest().tid(tag).tag_inv(1));
}

void
PublisherInventoryTest::billing_stats_logging()
{
  std::string description("Test 4. Billing stats logging for Inventory tags test case");
  add_descr_phrase(description);

  unsigned int pub_account = fetch_int("BillingStatsLoggingCase/RatePublisher/ACCOUNT_ID");
  unsigned int norate_tag = fetch_int("BillingStatsLoggingCase/NoRatePublisher/TAG_ID");
  unsigned int rate_tag = fetch_int("BillingStatsLoggingCase/RatePublisher/TAG_ID");
  unsigned int adv_tag = fetch_int("BillingStatsLoggingCase/AdvPublisher/TAG_ID");

  AdClient client(AdClient::create_user(this));

  add_hourly_stat(description.c_str(), pub_account, norate_tag, 1);
  add_hourly_stat(description.c_str(), pub_account, rate_tag, 1);
  add_hourly_stat(description.c_str(), pub_account, adv_tag, 0);

  NSLookupRequest request;
  request.debug_time = debug_time_;
  request.tag_inv = 0;
 
  request.tid = norate_tag;
  client.process_request(request,
    "Test 4.1 Inventory tag with undefined site rate (tag.inv = 0)");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.debug_info.selected_creatives.empty()),
    "must get empty ccid");

  request.tid = rate_tag;
  client.process_request(request,
    "Test 4.2 Inventory tag with defined site rate (tag.inv = 0)");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.debug_info.selected_creatives.empty()),
    "must get empty ccid");

  request.tid = adv_tag;
  request.tag_inv = 1;
  client.process_request(request,
    "Test 4.3. Adserving Tag and tag.inv = 1");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      204,
      client.req_status()).check(),
    "Server must return 'no content' status");
}

void PublisherInventoryTest::non_billing_stats_logging()
{
  std::string description("Test 5. Non-billing stats logging for Inventory tags test case");
  add_descr_phrase(description);

  AutoTest::Time tomorrow = debug_time_ + DAY - 1;

  unsigned int inv_tag = fetch_int("NonBillingStatsLoggingCase/InvPublisher/TAG_ID");
  unsigned int inv_site = fetch_int("NonBillingStatsLoggingCase/InvPublisher/SITE_ID");
  unsigned int adv_tag = fetch_int("NonBillingStatsLoggingCase/AdvPublisher/TAG_ID");
  unsigned int adv_site = fetch_int("NonBillingStatsLoggingCase/AdvPublisher/SITE_ID");

  unsigned long channel1 = fetch_int("NonBillingStatsLoggingCase/KChannel1/ID");
  std::string keyword1 = fetch_string("NonBillingStatsLoggingCase/KChannel1/KEYWORD#1");
  unsigned long channel2 = fetch_int("NonBillingStatsLoggingCase/KChannel2/ID");
  std::string keyword2 = fetch_string("NonBillingStatsLoggingCase/KChannel2/KEYWORD#1");

  unsigned long marker_ch = fetch_int("NonBillingStatsLoggingCase/MarkerChan/ID");
  std::string marker_kw = fetch_string("NonBillingStatsLoggingCase/MarkerChan/KEYWORD#1");
  unsigned int marker_site = fetch_int("NonBillingStatsLoggingCase/MarkerPub/SITE_ID");
  unsigned int marker_tag = fetch_int("NonBillingStatsLoggingCase/MarkerPub/TAG_ID");

  add_site_user_stat_(description.c_str(), inv_site, 1, true, Generics::Time::ZERO);
  add_site_user_stat_(description.c_str(), marker_site, 1, true, Generics::Time::ZERO);
  add_site_user_stat_(description.c_str(), adv_site, 1, true, Generics::Time::ZERO);
  add_site_user_stat_(description.c_str(), adv_site, 1); 
  add_channel_inv_stat_(description.c_str(), channel1, 2, 1, 1);
  add_channel_inv_stat_(description.c_str(), marker_ch, 1, 1, 1);
  add_channel_inv_stat_(description.c_str(), channel2, 1, 1, 1);

  std::string cc1 = fetch_string("NonBillingStatsLoggingCase/TCampaign1/CCID");
  std::string cc2 = fetch_string("NonBillingStatsLoggingCase/TCampaign2/CCID");

  NSLookupRequest request;
  request.debug_time = debug_time_;
  request.tid = inv_tag;
  request.tag_inv = 0;
  request.referer_kw = keyword1;

  // Requests from INVENTORY tag (Test 5.1)
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(request, "tag_inv = 0 from inventory tag");
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        strof(channel1),
        client.debug_info.history_channels).check(),
      "server must return expected history channels");
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.selected_creatives.empty()),
      "mustn't get any creatives");

    request.tag_inv = 1;
    client.process_request(request, "tag_inv = 1 from inventory tag");
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        strof(channel1),
        client.debug_info.history_channels).check(),
      "server must return expected history channels");
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc1,
        client.debug_info.ccid).check(),
      "must get expected ccid");
  }
  // Marker request to check 'no changes' stats for previous request
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(NSLookupRequest().
      referer_kw(marker_kw).
      debug_time(debug_time_).
      tid(marker_tag), "marker request");
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        strof(marker_ch),
        client.debug_info.history_channels).check(),
      "server must return expected history channels");
  }

  // Requests from ADVERTISING tag (Test 5.2)
  request.tid = adv_tag;
  request.referer_kw = keyword2;
  {
    request.tag_inv = 1;
    AdClient client(AdClient::create_user(this));
    client.process_request(request, "tag_inv = 1 from advertising tag");
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        strof(channel2),
        client.debug_info.history_channels).check(),
      "server must return expected history channels");
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.selected_creatives.empty()),
      "mustn't get any creatives");

    request.tag_inv = 0;
    request.debug_time = tomorrow;
    client.process_request(request, "tag_inv = 0 from advertising tag");
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        strof(channel2),
        client.debug_info.history_channels).check(),
      "server must return expected history channels");
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc2,
        client.debug_info.ccid).check(),
      "must get expected ccid");
  }
}

void PublisherInventoryTest::publisher_with_adjustment_coef()
{
  std::string description("Publisher with adjustment coef (ADSC-5502)");
  add_descr_phrase(description);

  unsigned int tag = fetch_int("PublisherAdjustment/InvPublisher/TAG_ID");
  double cpm = fetch_float("PublisherAdjustment/TAG_CPM") / 100; // in dollars

  add_publisher_inventory_stat(description.c_str(), tag, cpm, 3, 0, 3 * cpm / 1000);
  add_publisher_inventory_stat(description.c_str(), tag, 0,   0, 3, 0);

  NSLookupRequest request;
  request.tid = tag;
  request.debug_time = debug_time_;
  request.tag_inv = 1;

  // Request for display campaign
  {
    AdClient client(AdClient::create_user(this));
    request.referer_kw = fetch_string("PublisherAdjustment/DChannel/KEYWORD#1");
    client.process_request(request, "ad request for display cmp");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("PublisherAdjustment/DCampaign/CCID"),
        client.debug_info.ccid).check(),
      "server must return creative for display campaign");
  }

  // Request for channel targeted text campaign
  {
    AdClient client(AdClient::create_user(this));
    request.referer_kw = fetch_string("PublisherAdjustment/CTChannel/KEYWORD#1");
    client.process_request(request, "ad request for channel targeted cmp");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("PublisherAdjustment/CTCampaign/CCID"),
        client.debug_info.ccid).check(),
      "server must return creative for channel targeted text campaign");
  }

  // Request fot text campaign
  {
    AdClient client(AdClient::create_user(this));
    request.referer_kw = fetch_string("PublisherAdjustment/KChannel/KEYWORD#1");
    client.process_request(request, "ad request for channel targeted cmp");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("PublisherAdjustment/KCampaign/CCID"),
        client.debug_info.ccid).check(),
      "server must return creative for text campaign");
  }
}

bool
PublisherInventoryTest::run_test()
{
  ctr_ = fetch_float("CTR");
  pub_rate_ = fetch_float("PUBRATE");
  adv_rate_ = fetch_float("ADVRATE");

  AdClient optin_client(AdClient::create_user(this));
  AdClient optout_client(AdClient::create_optout_user(this));
  AdClient nonoptin_client(AdClient::create_nonoptin_user(this));

  // Scenarios
  base_scenario(optin_client, fetch_int("BaseCase/Publisher4/TAG_ID"), pub_rate_);
  base_scenario(optin_client, fetch_int("BaseCase/Publisher1/TAG_ID"), 1.0);
  base_scenario(optout_client, fetch_int("BaseCase/Publisher2/TAG_ID"), 1.0, fetch_int("ADS_COLO"));
  base_scenario(nonoptin_client, fetch_int("BaseCase/Publisher3/TAG_ID"), 1.0, fetch_int("ADS_COLO"));

  ta_campaigns_scenario();
  pub_adv_commission_scenario(fetch_int("GROSSCampaignsCase/GrossPublisher/TAG_ID"), pub_rate_);
  pub_adv_commission_scenario(fetch_int("GROSSCampaignsCase/NetPublisher/TAG_ID"));
  virtual_scenario();
  
  no_impression_scenario();
  billing_stats_logging();
  non_billing_stats_logging();
  publisher_with_adjustment_coef();

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pgconn_,
        publisher_inventory_diffs_,
        publisher_inventory_stats_)).check(),
    "must get expected changes in PublisherInventory table");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pgconn_,
        hourly_diffs_,
        hourly_stats_)).check(),
    "must get expected changes in RequestStatsHourly table");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pgconn_,
        site_user_diffs_,
        site_user_stats_)).check(),
    "must get expected changes in SiteUserStats table");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pgconn_,
        channel_inventory_diffs_,
        channel_inventory_stats_)).check(),
    "must get expected changes in ChannelInventory table");

  return true;
}
