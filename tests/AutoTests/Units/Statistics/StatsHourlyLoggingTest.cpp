#include "StatsHourlyLoggingTest.hpp"
#include <Generics/Rand.hpp>
#include <cmath>

REFLECT_UNIT(StatsHourlyLoggingTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ImpressionRequest ImpressionRequest;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;

  const unsigned int DEFAULT_CPM = 11;

  double revenue(const unsigned int& imps,
                 const unsigned int& cpm = DEFAULT_CPM)
  {
    return (double) imps*cpm/1000;
  }

  const unsigned int REPEAT_COUNT = 1000;
  const unsigned int PUB_CPM = 7;
  const unsigned int TAG_CURRENCY = 2;
  const unsigned int COLO_CURRENCY = 3;
  const double REVENUE_SHARE = 0.3;
  const double ADV_COMMISSION = 0.3;
  const double PUB_COMMISSION = 0.2;
  const double ADV_CPA = 1000;

  const double REQ_REVENUE = revenue(REPEAT_COUNT);
  const double PUB_AMOUNT = PUB_CPM * ( 1 - PUB_COMMISSION);
  const double PUB_REVENUE =  REPEAT_COUNT * PUB_AMOUNT / 1000;
  const double PUB_COMM_AMOUNT =  PUB_CPM * PUB_COMMISSION;

  double isp_ammount(bool gross_cmp)
  {
    double adv_comission = gross_cmp?  ADV_COMMISSION: 0;
    
    return
      (REQ_REVENUE * (1 - adv_comission) - PUB_REVENUE / TAG_CURRENCY) *
      COLO_CURRENCY * REVENUE_SHARE;
  }

}

void 
StatsHourlyLoggingTest::set_up()
{ }

bool 
StatsHourlyLoggingTest::run()
{

  AUTOTEST_CASE(
    case_absent_imp_req_id(),
    "Request for tracking without request-id");

  AUTOTEST_CASE(
    case_all_counters_imptrack_disabled(),
    "All counters started (impression tracking disabled)");

  RequestList noimp_req_list;
  ORM::StatsArray<HourlyStats, 2> noimp_stats;

  noimp_stats[0].key().
    cc_id(0).
    num_shown(1).
    colo_id(fetch_int("Colo Id/2")).
    stimestamp(target_request_time_);

  noimp_stats[1].key().
    cc_id(fetch_int("CC Id/2")).
    num_shown(1).
    colo_id(fetch_int("Colo Id/2")).
    stimestamp(target_request_time_);

  noimp_stats.select(pq_conn_);
    
  AUTOTEST_CASE(
    case_clicks_actions_noimpressions_part_1(
      noimp_stats, noimp_req_list),
    "Clicks and actions, but no impressions");
  
  AUTOTEST_CASE(
    case_ammounts("TC1"),
    "Ammounts net campaign");
  
  AUTOTEST_CASE(
    case_ammounts("TC2"),
    "Ammounts gross campaign");

  AUTOTEST_CASE(
    case_with_sdate_tinkling(),
    "Sdate tinkling");

  AUTOTEST_CASE(
    case_template_level_disabled_imptrack(),
    "Impression tracking disabled at template level");

  AUTOTEST_CASE(
    case_multiple_confirmation_of_creative(),
    "Multiple confirmation of creative");

  check();

  AUTOTEST_CASE(
    case_clicks_actions_noimpressions_part_2(
      noimp_stats, noimp_req_list),
    "Clicks and actions, but no impressions");

  return true;
}

void 
StatsHourlyLoggingTest::tear_down()
{ }

void
StatsHourlyLoggingTest::case_all_counters_imptrack_disabled()
{
  ORM::StatsArray<HourlyStats, 2> stats;

  stats[0].key().
    cc_id(fetch_int("CC Id/1")).
    num_shown(1).
    stimestamp(target_request_time_);
  stats[1].key().
    cc_id(fetch_int("CC Id/1")).
    tag_id(fetch_int("Tag Id/1")).
    stimestamp(target_request_time_);

  stats.select(pq_conn_);
  
  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test");
  request.tid(fetch_string("Tag Id/1"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("StatsHourlyLoggingTest/Colo"));
  request.debug_time(target_request_time_);

  // This request shouldn't return any ccid and shouldn't create any log
  AutoTest::NSLookupRequest zero_request;
  zero_request.referer("http://www.act.com");
  zero_request.format("unit-test");
  zero_request.tid(fetch_string("Tag Id/1"));
  zero_request.referer_kw("fake_request");
  zero_request.colo(fetch_string("StatsHourlyLoggingTest/Colo"));
  zero_request.debug_time(target_request_time_);

  StrVector exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/1"));

  for (unsigned int i = 0; i < REPEAT_COUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));

    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "unexpected creatives");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.click_url.empty()), 
      "must have debug_info.click_url");
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.first().action_adv_url.empty()),
      "must have debug_info.action_adv_url");

    std::string action_adv_url = client.debug_info.selected_creatives.first().action_adv_url;
    std::string click_url = client.debug_info.click_url;

    if (i % 3 == 2)
    {
      client.process_request(click_url);
    }

    if (i % 5 == 4)
    {
      client.process_request(action_adv_url);
    }

    client.process_request(zero_request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "must have empty ccid");
  }

  {
    const HourlyStats::Diffs diffs[] =
    {
      HourlyStats::Diffs().
        requests(REPEAT_COUNT),
      HourlyStats::Diffs().
          imps(REPEAT_COUNT).
          requests(REPEAT_COUNT).
          clicks(REPEAT_COUNT / 3).
          actions(REPEAT_COUNT / 3 / 5)
    };
    
    ADD_WAIT_CHECKER(
      "RequestStatsHourly check",
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        stats));
  }
}

// ADSC-5572. test was changed according to "Test3" from
// https://confluence.ocslab.com/display/QA/Click+Tracking+Test+Plan+%28ADSC%29
void
StatsHourlyLoggingTest::case_clicks_actions_noimpressions_part_1(
  ORM::StatsArray<HourlyStats, 2>& stats,
  RequestList& req_list)
{
  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.tid(fetch_string("Tag Id/2"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo Id/2"));
  request.debug_time(target_request_time_);

  AutoTest::CreativeList exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/2"));

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::CLICK);
  actions.push_back(AutoTest::ACTION);

  for (unsigned int i = 0; i < REPEAT_COUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(request);
    
    std::string imp_url =
      client.debug_info.track_pixel_url.value();

    FAIL_CONTEXT(
      client.do_ad_requests(
        exp_ccids, actions));

    if (i % 2 == 0)
    {
      RequestPair req_pair(client, imp_url);
      req_list.push_back(req_pair);
    }
  }
  
  {
    const HourlyStats::Diffs diffs[] =
    {
      HourlyStats::Diffs(0),
      HourlyStats::Diffs().
        requests(REPEAT_COUNT)
    };

    ADD_WAIT_CHECKER(
      "RequestStatsHourly check",
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        stats));
  }
}

void
StatsHourlyLoggingTest::case_clicks_actions_noimpressions_part_2(
  ORM::StatsArray<HourlyStats, 2>& stats,
  RequestList& req_list)
{
  stats.select(pq_conn_);
  
  for (RequestList::iterator req_pair = req_list.begin();
       req_pair != req_list.end(); ++req_pair)
  {
    req_pair->first.process_request(req_pair->second);
  }

  {
    const HourlyStats::Diffs diffs[] =
    {
      HourlyStats::Diffs(0),
      HourlyStats::Diffs().
        imps(REPEAT_COUNT / 2).
        requests(0).
        clicks(REPEAT_COUNT / 2).
        actions(REPEAT_COUNT / 2).
        adv_amount(
          ORM::stats_diff_type(
            REPEAT_COUNT * ADV_CPA / 2, 0.1))
    };

    ADD_WAIT_CHECKER(
      "RequestStatsHourly check",
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        stats));
  }
}

void
StatsHourlyLoggingTest::case_ammounts(const std::string& name_prefix)
{
  HourlyStats stat;

  stat.key().
    cc_id(fetch_int("CC Id/" + name_prefix)).
    num_shown(1).
    stimestamp(target_request_time_);

  stat.select(pq_conn_);
 
  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test"); // no track
  request.tid(fetch_string((std::string("Tag Id/") + name_prefix).c_str()));
  request.referer_kw(name_prefix);
  request.colo(fetch_string((std::string("Colo/") + name_prefix).c_str()));
  request.debug_time(target_request_time_);

  AutoTest::CreativeList exp_ccids;
  exp_ccids.push_back(fetch_string(std::string("CC Id/") + name_prefix));

  AutoTest::ConsequenceActionList actions1;
  actions1.push_back(AutoTest::CLICK);

  AutoTest::ConsequenceActionList actions2;
  actions2.push_back(AutoTest::CLICK);
  actions2.push_back(AutoTest::CLICK);

  for (unsigned int i = 0; i < REPEAT_COUNT / 2; ++i)
  {
    AdClient client(AdClient::create_user(this));    
    FAIL_CONTEXT(
      client.do_ad_requests(
        request, exp_ccids, actions2));
  }

  for (unsigned int i = 0; i < REPEAT_COUNT / 2; ++i)
  {
    AdClient client(AdClient::create_user(this));
    FAIL_CONTEXT(
      client.do_ad_requests(
        request, exp_ccids, actions1));
  }

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      name_prefix == "TC1"?
        HourlyStats::Diffs().
          imps(REPEAT_COUNT).
          requests(REPEAT_COUNT).
          clicks(REPEAT_COUNT).
          actions(0). // no actions : CPA = 0
          adv_amount(ORM::stats_diff_type(REQ_REVENUE, 0.1)).
          adv_comm_amount(
            ORM::stats_diff_type(REQ_REVENUE * ADV_COMMISSION /
              (1 - ADV_COMMISSION), 0.1)).
          pub_amount(ORM::stats_diff_type(PUB_AMOUNT, 0.1)).
          pub_comm_amount(ORM::stats_diff_type(PUB_COMM_AMOUNT, 0.1)).
          isp_amount(ORM::stats_diff_type(isp_ammount(false), 0.1)):
          HourlyStats::Diffs().
            imps(REPEAT_COUNT).
            requests(REPEAT_COUNT).
            clicks(REPEAT_COUNT).
            actions(0). // no actions : CPA = 0
            adv_amount(
              ORM::stats_diff_type(
                REQ_REVENUE * (1 - ADV_COMMISSION), 0.1)).
            adv_comm_amount(
              ORM::stats_diff_type(
                REQ_REVENUE * ADV_COMMISSION , 0.1)).
            pub_amount(ORM::stats_diff_type(PUB_AMOUNT, 0.1)).
            pub_comm_amount(ORM::stats_diff_type(PUB_COMM_AMOUNT, 0.1)).
            isp_amount(ORM::stats_diff_type(isp_ammount(true), 0.1)),
      stat));  
}

void
StatsHourlyLoggingTest::case_with_sdate_tinkling()
{

  AutoTest::Time base_time(
    ((target_request_time_ - Generics::Time::ONE_DAY * 2).
      get_gm_time().format("%d-%m-%Y") + ":" + "12-00-00").c_str());

  AutoTest::Time hour_later(
    base_time + Generics::Time::ONE_HOUR);
  
  AutoTest::Time day_later(
    base_time + Generics::Time::ONE_DAY);
  
  ORM::StatsArray<HourlyStats, 3> stats;
  
  stats[0].key().
    cc_id(fetch_int("CC Id/6")).
    num_shown(1).
    stimestamp(base_time);
  stats[1].key().
    cc_id(fetch_int("CC Id/6")).
    num_shown(1).
    stimestamp(hour_later);
  stats[2].key().
    cc_id(fetch_int("CC Id/6")).
    num_shown(1).
    stimestamp(day_later);

  stats.select(pq_conn_);

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test"); // no track
  request.tid(fetch_string("Tag Id/6"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("StatsHourlyLoggingTest/Colo"));

  StrVector exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/6"));

  for (unsigned int i = 0; i < 1000; ++i)
  {
    AdClient client(AdClient::create_user(this));

    // Requests from past
    if (i % 20 == 0)
    {
      request.debug_time = hour_later;
    }
    else if (i % 10 == 0)
    {
      request.debug_time = day_later;
    }
    else
    {
      request.debug_time = base_time;
    }
    
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "unexpected creatives");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.click_url.empty()), 
      "must have debug_info.click_url");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.empty()),
      "Server has returned selected creatives");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.first().action_adv_url.empty()),
      "must have debug_info.action_adv_url");

    std::string action_adv_url = client.debug_info.selected_creatives.first().action_adv_url;
    action_adv_url += "*amp*debug-time*eql*" + request.debug_time.str();
    
    client.process_request(client.debug_info.click_url);

    client.process_request(action_adv_url);     
  }

  {
    const HourlyStats::Diffs diffs[] =
    {
      HourlyStats::Diffs().
        imps(900).
        requests(900).
        clicks(900).
         actions(900),
      HourlyStats::Diffs().
        imps(50).
        requests(50).
        clicks(50).
        actions(50),
      HourlyStats::Diffs().
        imps(50).
        requests(50).
        clicks(50).
        actions(50)
    };

    ADD_WAIT_CHECKER(
      "RequestStatsHourly check",
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        stats));
  }
}

void
StatsHourlyLoggingTest::case_multiple_confirmation_of_creative()
{
  HourlyStats stat;

  stat.key().
    cc_id(fetch_int("CC Id/4")).
    num_shown(1).
    stimestamp(target_request_time_);

  stat.select(pq_conn_);
 
  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");    
  request.tid(fetch_string("Tag Id/4"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("StatsHourlyLoggingTest/Colo"));
  request.debug_time(target_request_time_);

  RequestList req_list;

  for (unsigned int i = 0; i < REPEAT_COUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));

    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.track_pixel_url.empty()), 
      "must have debug_info.track_pixel_url");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.empty()),
      "Server has returned selected creatives");

    std::string imp_url =
      client.debug_info.track_pixel_url.value();

    RequestPair req_pair(client, imp_url);
    req_list.push_back(req_pair);
    if ( i  % 2 == 0 )
    {
      req_list.push_back(req_pair);
    }
  }

  for (RequestList::iterator req_pair = req_list.begin();
       req_pair != req_list.end(); ++req_pair)
  {
    req_pair->first.process_request(req_pair->second);
  }
  
  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      HourlyStats::Diffs().
        imps(REPEAT_COUNT).
        requests(REPEAT_COUNT),
      stat));
}

void
StatsHourlyLoggingTest::case_template_level_disabled_imptrack()
{
  HourlyStats stat;

  stat.key().
    cc_id(fetch_int("CC Id/3")).
    num_shown(1).
    stimestamp(target_request_time_);

  stat.select(pq_conn_);
  
  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test");
  request.tid(fetch_string("Tag Id/3"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("StatsHourlyLoggingTest/Colo"));
  request.debug_time(target_request_time_);

  StrVector exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/3"));

  add_descr_phrase("Send clicks & actions before impression");
  for (unsigned int i = 0; i < REPEAT_COUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "unexpected creatives");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.track_pixel_url.empty()), 
      "must not have debug_info.track_pixel_url");
  }

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      HourlyStats::Diffs().
        imps(REPEAT_COUNT).
        requests(REPEAT_COUNT),
      stat));
}

void
StatsHourlyLoggingTest::case_absent_imp_req_id()
{
  HourlyStats stat;

  stat.key().
    cc_id(fetch_int("CC Id/5")).
    num_shown(1).
    stimestamp(target_request_time_);

  stat.select(pq_conn_);

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");    
  request.tid(fetch_string("Tag Id/5"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("StatsHourlyLoggingTest/Colo"));
  request.debug_time(target_request_time_);

  StrVector exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/5"));

  for (unsigned int i = 0; i < 10; ++i)
  {
    AdClient client(AdClient::create_user(this));

    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "unexpected creatives");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.track_pixel_url.empty()), 
      "must have debug_info.track_pixel_url");

    ImpressionRequest impression;
    impression.ccid = fetch_string("CC Id/5");
    client.process_request(impression);
  }

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      HourlyStats::Diffs().
        imps(0).
        requests(10).
        clicks(0).
        actions(0).
        adv_amount(0).
        adv_comm_amount(0).
        pub_amount(0).
        pub_comm_amount(0).
        isp_amount(0),
      stat));
}
