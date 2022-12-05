#include "OIXTestModeTest.hpp"

REFLECT_UNIT(OIXTestModeTest) (
  "CreativeSelection",
  AUTO_TEST_FAST | AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ActionRequest ActionRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::precisely_number precisely_number;
  typedef AutoTest::TagPassbackChecker TagPassbackChecker;
  typedef AutoTest::RedirectChecker RedirectChecker;

  struct TestCase
  {
    const char* description;
    const char* referer_kw;
    const char* tid;
    const char* colo;
    size_t testrequest;
    const char* exp_ccid;
  };

  const TestCase test_cases[] = {

    {"Request with testrequest=2, test tag and non test colo for non test campaign",
     "KW-NON-TEST", "TAG-TEST", "COLO-NON-TEST", 2, "CC-NON-TEST"},

    {"Request with testrequest=2, test tag and non test colo for test campaign",
     "KW-TEST", "TAG-TEST", "COLO-NON-TEST", 2, "CC-TEST"},

    {"Request with testrequest=2, non test tag and colo for test campaign",
     "KW-TEST", "TAG-NON-TEST", "COLO-NON-TEST", 2, 0},

    {"Request with testrequest=2, non test tag and test colo for non test campaign",
     "KW-NON-TEST", "TAG-NON-TEST", "COLO-TEST", 2, "CC-NON-TEST"},

    {"Request with testrequest=2, non test tag and test colo for test campaign",
     "KW-TEST", "TAG-NON-TEST", "COLO-TEST", 2, "CC-TEST"},

    {"Passback on request with testrequest=2, test tag and non test colo",
     0, "TAG-TEST", "COLO-NON-TEST", 2, 0},

    {"Passback on request with testrequest=2, non test tag and test colo",
     0, "TAG-NON-TEST", "COLO-TEST", 2, 0},

    {"Test request for non test entities",
     "KW-NON-TEST", "TAG-NON-TEST", "COLO-NON-TEST", 1, "CC-NON-TEST"},

    {"Test request for non test entities - no impression",
     "KW-NON-TEST", "TAG-EXP", "COLO-NON-TEST", 1, 0},

    {"Non test request (testrequest=2) for non test entities",
     "KW-NON-TEST", "TAG-NON-TEST", "COLO-NON-TEST", 2, "CC-NON-TEST"},

    {"Test request with non test tag and colo for test campaign",
     "KW-TEST", "TAG-NON-TEST", "COLO-NON-TEST", 1, "CC-TEST"},

    {"Passback on request with testrequest=1, non test tag and colo",
     0, "TAG-NON-TEST", "COLO-NON-TEST", 1, 0},

    {"Regular request with test tag and non test colo for non test campaign",
     "KW-NON-TEST", "TAG-TEST", "COLO-NON-TEST", 0, "CC-NON-TEST"},

    {"Regular request with test tag and non test colo for test campaign",
     "KW-TEST", "TAG-TEST", "COLO-NON-TEST", 0, "CC-TEST"},

    {"Regular request with non test tag and test colo for non test campaign",
     "KW-NON-TEST", "TAG-NON-TEST", "COLO-TEST", 0, "CC-NON-TEST"},

    {"Regular request with non test tag and test colo for test campaign",
     "KW-TEST", "TAG-NON-TEST", "COLO-TEST", 0, "CC-TEST"},

    {"Passback on request with test tag and non test colo",
     0, "TAG-TEST", "COLO-NON-TEST", 0, 0},

    {"Passback on request with non test tag and test colo",
     0, "TAG-NON-TEST", "COLO-TEST", 0, 0},

    {"Regular request with non test tag and colo for test campaign",
     "KW-TEST", "TAG-NON-TEST", 0, 0, 0}
  };
}

namespace ORM = AutoTest::ORM;

void
OIXTestModeTest::pre_condition()
{
  add_descr_phrase("Pre condition: save state of statistical tables");

  Generics::Time target_sdate = now_.get_gm_time().get_date() +
    Generics::Time::ONE_HOUR * now_.get_gm_time().tm_hour;

  non_test_ch_stats_[0].key().channel_id(fetch_int("CHANNEL-NON-TEST"));
  non_test_ch_stats_[1].key().channel_id(fetch_int("CHANNEL-NON-TEST")).
                            exclude_colo(fetch_int("COLO-DEFAULT"));
  non_test_ch_stats_[2].key().channel_id(fetch_int("K-CHANNEL-NON-TEST"));
  non_test_ch_stats_[3].key().channel_id(fetch_int("K-CHANNEL-NON-TEST")).
    exclude_colo(fetch_int("COLO-DEFAULT"));
  non_test_ch_stats_.select(pq_conn_);

  test_ch_stats_[0].key().channel_id(fetch_int("CHANNEL-TEST"));
  test_ch_stats_[1].key().channel_id(fetch_int("CHANNEL-TEST")).
                    exclude_colo(fetch_int("COLO-DEFAULT"));
  test_ch_stats_[2].key().channel_id(fetch_int("K-CHANNEL-TEST"));
  test_ch_stats_[3].key().channel_id(fetch_int("K-CHANNEL-TEST")).
                    exclude_colo(fetch_int("COLO-DEFAULT"));
  test_ch_stats_.select(pq_conn_);

  ctx_ch_stats_[0].key().channel_id(fetch_int("CTX/CHANNEL"));
  ctx_ch_stats_[1].key().channel_id(fetch_int("CTX/CHANNEL")).
    exclude_colo(fetch_int("COLO-DEFAULT"));
  ctx_ch_stats_.select(pq_conn_);

  colo_stats_[0].key(fetch_int("COLO-TEST"));
  colo_stats_[1].key(fetch_int("COLO-NON-TEST"));
  colo_stats_.select(pq_conn_);

  site_stats_[0].key().site_id(fetch_int("SITE-NON-TEST"));
  site_stats_[1].key().site_id(fetch_int("SITE-TEST"));
  site_stats_[2].key().site_id(fetch_int("SITE-NON-TEST")).
    exclude_colo(fetch_int("COLO-DEFAULT"));
  site_stats_.select(pq_conn_);

  tag_stats_[0].key().tag_id(fetch_int("TAG-NON-TEST"));
  tag_stats_[1].key().tag_id(fetch_int("TAG-TEST"));
  tag_stats_[2].key().tag_id(fetch_int("TAG-NON-TEST")).
    exclude_colo(fetch_int("COLO-DEFAULT"));
  tag_stats_[3].key().tag_id(fetch_int("PUBINV/TAG"));
  tag_stats_.select(pq_conn_);

  test_stats_[0].
    table(ORM::HourlyStats::RequestStatsHourlyTest).
    key().
    cc_id(fetch_int("DISP/CC-NON-TEST")).
    num_shown(1).
    fraud_correction(false).
    stimestamp(target_sdate);
  test_stats_[1].
    table(ORM::HourlyStats::RequestStatsHourlyTest).
    key().
    cc_id(fetch_int("TEXT/CC-NON-TEST")).
    num_shown(1).
    fraud_correction(false).
    stimestamp(target_sdate);
  test_stats_[2].
    table(ORM::HourlyStats::RequestStatsHourlyTest).
    key().
    cc_id(fetch_int("DISP/CC-TEST")).
    num_shown(1).
    fraud_correction(false).
    stimestamp(target_sdate);
  test_stats_[3].
    table(ORM::HourlyStats::RequestStatsHourlyTest).
    key().
    cc_id(fetch_int("TEXT/CC-TEST")).
    num_shown(1).
    fraud_correction(false).
    stimestamp(target_sdate);
  
  test_stats_.select(pq_conn_);

  // OVERLAP-CHANNEL1 & OVERLAP-CHANNEL2 matched on test request
  const char* OVERLAP_CHANNELS[] = {
    "OVERLAP-CHANNEL1", "OVERLAP-CHANNEL2" };

  for (size_t i = 0; i < countof(OVERLAP_CHANNELS); ++i)
  {
    ORM::ChannelOverlapUserStats::Key key;
    key.channel1(fetch_int(OVERLAP_CHANNELS[i]));
    key.sdate(now_);

    ORM::ChannelOverlapUserStats stat(key);
    
    stat.select(pq_conn_);

    overlap_empty_stats_.push_back(stat);
  }

  // OVERLAP-CHANNEL3 & OVERLAP-CHANNEL4 matched on ordinary request
  ORM::ChannelOverlapUserStats::Key key;
  key.channel1(fetch_int("OVERLAP-CHANNEL3"));
  key.channel2(fetch_int("OVERLAP-CHANNEL4"));
  key.sdate(now_);
  overlap_stats_.key(key);
  overlap_stats_.select(pq_conn_);
}

void
OIXTestModeTest::process_test_case_(size_t index)
{
  add_descr_phrase(test_cases[index].description);

  std::string d_cc_id, t_cc_id, keyword;
  keyword = fetch_string("CTX/KEYWORD") +
            "," +
            fetch_string("HALO-TRIGGER");
            
  if (test_cases[index].referer_kw)
  {
    keyword += "," + fetch_string(test_cases[index].referer_kw) + "-d";
  }

  if (test_cases[index].exp_ccid)
  {
    d_cc_id = fetch_string(std::string("DISP/") + test_cases[index].exp_ccid);
    t_cc_id = fetch_string(std::string("TEXT/") + test_cases[index].exp_ccid);
  }

  NSLookupRequest request;
  request.format("unit-test-imp"); // all cases expect impr tracking
  request.debug_time(now_);
  request.tid = fetch_string(test_cases[index].tid);
  if (test_cases[index].colo)
  {
    request.colo = fetch_string(test_cases[index].colo);
  }

  if (test_cases[index].testrequest)
  {
    request.testrequest = test_cases[index].testrequest;
  }

  AdClient user_d(AdClient::create_user(this));
  request.referer_kw = keyword;
  user_d.process_request(request, "display");

  NOSTOP_FAIL_CONTEXT(make_derivative_requests_(
    user_d, test_cases[index].testrequest, d_cc_id, true));

  if (test_cases[index].referer_kw)
  {
    request.referer_kw = keyword.replace(keyword.end() - 1,
                                         keyword.end(), "t");
  }
  AdClient user_t(AdClient::create_user(this));
  user_t.process_request(request, "text");
  user_t.repeat_request();
  NOSTOP_FAIL_CONTEXT(make_derivative_requests_(
    user_t,
    test_cases[index].testrequest,
    t_cc_id,
    false // no action tracking for text ads
    ));
}

void
OIXTestModeTest::both_campaigns_can_match_()
{
  add_descr_phrase("Test request for test and non test campaigns: "
    "test campaign wins by ecpm");

  NSLookupRequest request;
  request.debug_time(now_);
  request.tid = fetch_string("TAG-NON-TEST");
  request.colo = fetch_string("COLO-TEST");
  request.referer_kw = fetch_string("KW-TEST") + "-d," +
    fetch_string("KW-NON-TEST") + "-d," + fetch_string("HALO-TRIGGER");

  AdClient user(AdClient::create_user(this));
  user.process_request(request, "request for test campaign");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("DISP/CC-TEST"),
      user.debug_info.ccid).check(),
    "must select expected creative");

  add_descr_phrase("Regular request for test and non test campaigns: "
    "show non test campaign, "
    "test campaign blocked for regular request");
  request.colo.clear();
  user.process_request(request, "request for non test campaign");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("DISP/CC-NON-TEST"),
      user.debug_info.ccid).check(),
    "must select expected creative");
}

void
OIXTestModeTest::inventory_mode_tag_()
{
  add_descr_phrase("Test request for tag in inventory estimation mode");

  AdClient user(AdClient::create_user(this));

  user.process_request(NSLookupRequest().tid(fetch_string("PUBINV/TAG")).
                                         debug_time(now_).
                                         referer_kw(fetch_string("PUBINV/KW")).
                                         tag_inv(1).
                                         testrequest(1));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("PUBINV/CC"),
      user.debug_info.ccid).check(),
    "must select expected creative");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      204,
      user.req_status()).check(),
    "Server must return 'no content' status");

  precisely_number expected_cpm_threshold(
    fetch_float("PUBINV/THRESHOLD_CPM"), 0.00000001);

  precisely_number got_cpm_threshold(
    user.debug_info.cpm_threshold, 0.00000001);
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      expected_cpm_threshold,
      got_cpm_threshold).check(),
    "must get expected cpm_threshold in debug-info");
}

void OIXTestModeTest::channel_overlaps_()
{
  std::string description("Channel overlaps in test mode.") ;
  add_descr_phrase(description);

  NSLookupRequest request;
  request.debug_time(now_);
  request.referer_kw = fetch_string("OVERLAP-KWD1");
  request.testrequest = 1;

  {
    AdClient user(AdClient::create_user(this));
    user.process_request(request);
    
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "OVERLAP-CHANNEL1,OVERLAP-CHANNEL2",
        user.debug_info.history_channels).check(),
      description +
        " Expected history");
  }

  request.referer_kw = fetch_string("OVERLAP-KWD2");
  request.testrequest.clear();

  {
    AdClient user(AdClient::create_user(this));
    user.process_request(request);
    
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "OVERLAP-CHANNEL3,OVERLAP-CHANNEL4",
        user.debug_info.history_channels).check(),
      description +
        " Expected history");
  }  
}
 
bool 
OIXTestModeTest::run()
{
  add_descr_phrase("Run test");

  NOSTOP_FAIL_CONTEXT(channel_overlaps_());
  
  for (size_t i = 0; i < countof(test_cases); ++i)
  {
    NOSTOP_FAIL_CONTEXT(process_test_case_(i));
  }
  NOSTOP_FAIL_CONTEXT(both_campaigns_can_match_());
  NOSTOP_FAIL_CONTEXT(inventory_mode_tag_());

  return true;
}

void
OIXTestModeTest::post_condition()
{
  add_descr_phrase("Post condition: check there are no new statistic "
                   "for test requests");
  // There are no diffs for test request (only for regular)
  ORM::ChannelIdBasedStats::Diffs non_test_ch_diff[4] = {
    ORM::ChannelIdBasedStats::Diffs()
    .channelimpinventory_imps (1)
    .channelimpinventory_clicks (0)
    .channelimpinventory_actions (0)
    .channelimpinventory_revenue (0)        
    .channelimpinventory_impops_user_count (2)
    .channelimpinventory_imps_user_count (1)       
    .channelimpinventory_imps_other (2)
    .channelimpinventory_imps_other_user_count (1)
    .channelimpinventory_impops_no_imp (0)
    .channelimpinventory_impops_no_imp_user_count (0)
    .channelimpinventory_impops_no_imp_value (0)
    .channelinventory_active_user_count (1)
    .channelinventory_hits (1)
    .channelinventory_hits_urls (0)
    .channelinventory_hits_kws (1)
    .channelinventory_hits_search_kws (0)
    .channelinventory_hits_url_kws (0)
    .channelinventorybycpm_user_count (1),
    ORM::ChannelIdBasedStats::Diffs(0),
    ORM::ChannelIdBasedStats::Diffs(0),
    ORM::ChannelIdBasedStats::Diffs(0)
  };

  //ORM::stats_diff_type test_ch_diff[4][31] = {
  ORM::ChannelIdBasedStats::Diffs test_ch_diff[4] = {
    ORM::ChannelIdBasedStats::Diffs()
    .channelimpinventory_imps (0)
    .channelimpinventory_clicks (0)
    .channelimpinventory_actions (0)
    .channelimpinventory_revenue (0)        
    .channelimpinventory_impops_user_count (4)
    .channelimpinventory_imps_user_count (0)       
    .channelimpinventory_imps_value (0)
    .channelimpinventory_imps_other (3)
    .channelimpinventory_imps_other_user_count (2)
    .channelimpinventory_impops_no_imp (3)
    .channelimpinventory_impops_no_imp_user_count (2)
    .channelinventory_active_user_count (2)
    .channelinventory_hits (2)
    .channelinventory_hits_urls (0)
    .channelinventory_hits_kws (2)
    .channelinventory_hits_search_kws (0)
    .channelinventory_hits_url_kws (0)
    .channelinventorybycpm_user_count (2),
    ORM::ChannelIdBasedStats::Diffs(0),
    ORM::ChannelIdBasedStats::Diffs()
    .channelimpinventory_imps (0)
    .channelimpinventory_clicks (0)
    .channelimpinventory_actions (0)
    .channelimpinventory_revenue (0)        
    .channelimpinventory_impops_user_count (2)
    .channelimpinventory_imps_user_count (0)       
    .channelimpinventory_imps_value (0)
    .channelimpinventory_imps_other (0)
    .channelimpinventory_imps_other_user_count (0)
    .channelimpinventory_impops_no_imp (3)
    .channelimpinventory_impops_no_imp_user_count (2)
    .channelinventory_active_user_count (1)
    .channelinventory_hits (2)
    .channelinventory_hits_urls (0)
    .channelinventory_hits_kws (2)
    .channelinventory_hits_search_kws (0)
    .channelinventory_hits_url_kws (0)
    .channelinventorybycpm_user_count (0),
    ORM::ChannelIdBasedStats::Diffs(0)
  };

  ORM::ChannelIdBasedStats::Diffs ctx_ch_diff[2] = {
    ORM::ChannelIdBasedStats::Diffs()
    .channelimpinventory_imps (0)
    .channelimpinventory_clicks (0)
    .channelimpinventory_actions (0)
    .channelimpinventory_revenue (0)        
    .channelimpinventory_impops_user_count (4)
    .channelimpinventory_imps_user_count (0)       
    .channelimpinventory_imps_value (0)
    .channelimpinventory_imps_other (0)
    .channelimpinventory_imps_other_user_count (0)
    .channelimpinventory_impops_no_imp (9)
    .channelimpinventory_impops_no_imp_user_count (4)
    .channelinventory_active_user_count (2)
    .channelinventory_total_user_count (2)
    .channelinventory_hits (3)
    .channelinventory_hits_urls (0)
    .channelinventory_hits_kws (3)
    .channelinventory_hits_search_kws (0)
    .channelinventory_hits_url_kws (0)
    .channelinventorybycpm_user_count (2),
    ORM::ChannelIdBasedStats::Diffs(0)
  };

  ORM::ColoIdBasedStats::Diffs colo_diff[2] =
  {
    ORM::ColoIdBasedStats::Diffs(0).
      requeststatshourly_imps(ORM::any_stats_diff).
      requeststatshourly_requests(ORM::any_stats_diff).
      requeststatshourly_clicks(ORM::any_stats_diff).
      requeststatshourly_actions(ORM::any_stats_diff).
      requeststatshourly_adv_amount(ORM::any_stats_diff).
      requeststatshourly_adv_amount_global(ORM::any_stats_diff).
      requeststatshourly_isp_amount(ORM::any_stats_diff).
      requeststatshourly_isp_amount_global(ORM::any_stats_diff).
      requeststatshourly_pub_amount(ORM::any_stats_diff).
      requeststatshourly_pub_amount_global(ORM::any_stats_diff),
    ORM::ColoIdBasedStats::Diffs(0).
      requeststatshourly_imps(ORM::any_stats_diff).
      requeststatshourly_requests(ORM::any_stats_diff).
      requeststatshourly_clicks(ORM::any_stats_diff).
      requeststatshourly_actions(ORM::any_stats_diff).
      requeststatshourly_adv_amount(ORM::any_stats_diff).
      requeststatshourly_adv_amount_global(ORM::any_stats_diff).
      requeststatshourly_isp_amount(ORM::any_stats_diff).
      requeststatshourly_isp_amount_global(ORM::any_stats_diff).
      requeststatshourly_pub_amount(ORM::any_stats_diff).
      requeststatshourly_pub_amount_global(ORM::any_stats_diff)
  };

  ORM::SiteIdBasedStats::Diffs site_diff[3] = {
    ORM::SiteIdBasedStats::Diffs()
    .siteuserstats_unique_users(3)
    .pageloadsdaily_page_loads(4)
    .pageloadsdaily_utilized_page_loads(1),
    ORM::SiteIdBasedStats::Diffs(0),
    ORM::SiteIdBasedStats::Diffs(0)
  };
  
  ORM::TagIdBasedStats::Diffs tag_diff[4] = {
    ORM::TagIdBasedStats::Diffs()
    .tagauctionstats_requests(4)
    .publisherinventory_imps(0)
    .publisherinventory_requests(0)
    .publisherinventory_revenue(0),
    ORM::TagIdBasedStats::Diffs(0),
    ORM::TagIdBasedStats::Diffs(0),
    ORM::TagIdBasedStats::Diffs()
    .tagauctionstats_requests(0)
    .publisherinventory_imps(1)
    .publisherinventory_requests(1)
    .publisherinventory_revenue(ORM::stats_diff_type(
       static_cast<double>(
         fetch_float("PUBINV/THRESHOLD_CPM") / 100 / 1000),
       0.000001))
  };

  // There are diffs only for test stats
  ORM::HourlyStats::Diffs test_diff[4]= {
    ORM::HourlyStats::Diffs().imps(6).clicks(6).actions(6).requests(6),
    ORM::HourlyStats::Diffs().imps(6).clicks(6).actions(0).requests(6),
    ORM::HourlyStats::Diffs().imps(6).clicks(5).actions(5).requests(6),
    ORM::HourlyStats::Diffs().imps(5).clicks(5).actions(0).requests(5)
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        non_test_ch_diff,
        non_test_ch_stats_)).check(),
    "channel id based stats check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        test_ch_diff, 
        test_ch_stats_)).check(),
    "channel id based stats check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        ctx_ch_diff,
        ctx_ch_stats_)).check(),
    "channel id based stats check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        colo_diff,
        colo_stats_)).check(),
    "colo id based stats tables stored statistic for test requests");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        site_diff,
        site_stats_)).check(),
    "site id based stats tables stored statistic for test requests");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        tag_diff,
        tag_stats_)).check(),
    "tag id based stats tables stored statistic for test requests");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        test_diff,
        test_stats_)).check(),
    "wait expected test stats");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        ORM::ChannelOverlapUserStats::Diffs().users(1),
        overlap_stats_)).check(),
    "wait expected channels overlap stats");
    
  FAIL_CONTEXT(
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      AutoTest::ORM::stats_diff_type(0),
      overlap_empty_stats_).check(),
      "unexpected channels overlap stats");
}

void
OIXTestModeTest::tear_down()
{};

void OIXTestModeTest::make_derivative_requests_(
  AdClient& client,
  size_t testrequest,
  const std::string& exp_ccid,
  bool expected_adv_action_url)
{
  if (exp_ccid.empty())
  {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.selected_creatives.empty()),
      "Server must return empty creative");

    TagPassbackChecker checker(
      client,
      fetch_string("ORIGINAL_URL"));
    
    FAIL_CONTEXT(
      checker.check(),
      "Passback check");

    client.process_request(checker.tokens().pixel());
     
  }
  else
  {
    std::list<std::string> expected_ccs;
    expected_ccs.push_back(exp_ccid);

    AutoTest::ConsequenceActionList actions;

    actions.push_back(
      AutoTest::ConsequenceAction(
        AutoTest::TRACK, now_));

    actions.push_back(
      AutoTest::ConsequenceAction(
        AutoTest::CLICK, now_));

    if(expected_adv_action_url)
    {
      actions.push_back(
        AutoTest::ConsequenceAction(
          AutoTest::ACTION, now_));
    }

    FAIL_CONTEXT(
      client.do_ad_requests(
        expected_ccs, actions),
      "Unexpected ad sequence");

    ActionRequest custom_action_req;
    custom_action_req.actionid = fetch_string("CustomAction");
    custom_action_req.country = "GN";
    custom_action_req.testrequest = testrequest;
    custom_action_req.debug_time = now_;
    client.process_request(custom_action_req, "test custom action request");
  }
};
