#include "StatsHourlyActionsTest.hpp"

REFLECT_UNIT(StatsHourlyActionsTest) (
  "Statistics",
  AUTO_TEST_SLOW);

namespace
{

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ActionRequest ActionRequest;
  typedef AutoTest::AdClient AdClient;
}

void
StatsHourlyActionsTest::set_up()
{ }

bool
StatsHourlyActionsTest::run()
{
  //2
  AUTOTEST_CASE(
    case_action_from_different_campaign(),
    "Action from different campaign");
  //5
  AUTOTEST_CASE(
    case_one_action_for_multiple_creatives_in_campaign(),
    "One action for multiple creatives in campaign");
  //6
  AUTOTEST_CASE(
    case_action_for_display_creative_group_with_cpc_rate(),
    "Action for display creative group with cpc rate");
  //1
  AUTOTEST_CASE(
    case_base_functionality(),
    "Base functionality (single action after click)");
  //2.2
  AUTOTEST_CASE(
    case_triple_action(),
    "Triple action");
  //3
  AUTOTEST_CASE(
    case_action_before_click(),
    "Action before click");
  //4
  AUTOTEST_CASE(
    case_action_before_impression_confirmation(),
    "Action before impression confirmation");
  //7
  AUTOTEST_CASE(
    case_actions_for_text_ad_group(),
    "Actions for text ad group");
  
  return true;
}

void
StatsHourlyActionsTest::tear_down()
{ }

void
StatsHourlyActionsTest::case_base_functionality()
{

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.tid(fetch_string("Tag Id/1"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo"));
  request.debug_time(target_request_time_);

  HourlyStats stat(
    HourlyStats::Key().
      cc_id(fetch_int("CC Id/1")).
      num_shown(1).
      stimestamp(target_request_time_));

  stat.select(pq_conn_);

  AutoTest::CreativeList exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/1"));

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::TRACK);
  actions.push_back(AutoTest::CLICK);
  actions.push_back(AutoTest::WAIT);
  actions.push_back(AutoTest::ACTION);

  AdClient client(AdClient::create_user(this));

  FAIL_CONTEXT(
    client.do_ad_requests(
      request, exp_ccids, actions));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        HourlyStats::Diffs().
          imps(1).
          requests(1).
          clicks(1).
          actions(1).
          adv_amount(1000).
          adv_comm_amount(0).
          pub_amount(0.02).
          pub_comm_amount(0).
          isp_amount(499.99),
        stat)));
}

void
StatsHourlyActionsTest::case_action_from_different_campaign()
{
  ORM::StatsList<HourlyStats> stats;
  std::list<HourlyStats::Diffs> diffs;
  
  {
    HourlyStats stat(
      HourlyStats::Key().
        cc_id(fetch_int("CC Id/2/1")).
        num_shown(1).
        stimestamp(target_request_time_));

    stat.description("#1");
    stat.select(pq_conn_);

    stats.push_back(stat);
    diffs.push_back(
      HourlyStats::Diffs().
        imps(1).
        requests(1).
        clicks(1).
        actions(0).
        adv_amount(0).
        adv_comm_amount(0).
        pub_amount(0.02).
        pub_comm_amount(0).
        isp_amount(-0.01) );
  }
  {
    HourlyStats stat(
      HourlyStats::Key().
        cc_id(fetch_int("CC Id/2/2")).
        stimestamp(target_request_time_));

    stat.description("#2");
    stat.select(pq_conn_);

    stats.push_back(stat);
    diffs.push_back(
      HourlyStats::Diffs(0) );
  }

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.tid(fetch_string("Tag Id/2"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo"));
  request.debug_time(target_request_time_);

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::TRACK);
  actions.push_back(AutoTest::CLICK);

  AdClient client(AdClient::create_user(this));

  AutoTest::CreativeList exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/2/1"));
  
  FAIL_CONTEXT(
    client.do_ad_requests(
      request, exp_ccids, actions));

  ActionRequest action_request;
  action_request.cid = fetch_string("CCG Id/2/2");
  client.process_request(action_request);

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        stats)));
}

void
StatsHourlyActionsTest::case_triple_action()
{
  HourlyStats stat(
    HourlyStats::Key().
      cc_id(fetch_int("CC Id/2x2")).
      num_shown(1).
      stimestamp(target_request_time_));

  stat.select(pq_conn_);

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.tid(fetch_string("Tag Id/2x2"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo"));
  request.debug_time(target_request_time_);

  AutoTest::CreativeList exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/2x2"));

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::TRACK);
  actions.push_back(AutoTest::CLICK);
  actions.push_back(AutoTest::ACTION);
  actions.push_back(AutoTest::ACTION);
  actions.push_back(AutoTest::ACTION);

  AdClient client(AdClient::create_user(this));
  
  FAIL_CONTEXT(
    client.do_ad_requests(
      request, exp_ccids, actions));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        HourlyStats::Diffs().
          imps(1).
          requests(1).
          clicks(1).
          actions(3).
          adv_amount(3000).
          adv_comm_amount(0).
          pub_amount(0.02).
          pub_comm_amount(0).
          isp_amount(1499.99),
        stat)));
}

void
StatsHourlyActionsTest::case_action_before_click()
{
  HourlyStats stat(
    HourlyStats::Key().
      cc_id(fetch_int("CC Id/3")).
      num_shown(1).
      stimestamp(target_request_time_));

  stat.select(pq_conn_);

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.tid(fetch_string("Tag Id/3"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo"));
  request.debug_time(target_request_time_);

  AutoTest::CreativeList exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/3"));

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::TRACK);
  actions.push_back(AutoTest::ConsequenceAction(
    AutoTest::ACTION, target_request_time_));
  actions.push_back(AutoTest::WAIT);
  actions.push_back(AutoTest::ConsequenceAction(
    AutoTest::CLICK, target_request_time_ + 1));

  AdClient client(AdClient::create_user(this));
  
  FAIL_CONTEXT(
    client.do_ad_requests(
      request, exp_ccids, actions));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        HourlyStats::Diffs().
          imps(1).
          requests(1).
          clicks(1).
          actions(1).
          adv_amount(1000).
          adv_comm_amount(0).
          pub_amount(0.02).
          pub_comm_amount(0).
          isp_amount(499.99),
        stat)));
}

void
StatsHourlyActionsTest::case_action_before_impression_confirmation()
{
  HourlyStats stat(
    HourlyStats::Key().
      cc_id(fetch_int("CC Id/4")).
      num_shown(1).
      stimestamp(target_request_time_));

  stat.select(pq_conn_);

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.tid(fetch_string("Tag Id/4"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo"));
  request.debug_time(target_request_time_);

  AutoTest::CreativeList exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/4"));

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::ConsequenceAction(
    AutoTest::ACTION, target_request_time_));
  actions.push_back(AutoTest::ConsequenceAction(
    AutoTest::ACTION, target_request_time_));
  actions.push_back(AutoTest::ConsequenceAction(
    AutoTest::TRACK, target_request_time_ + 1));
  actions.push_back(AutoTest::ConsequenceAction(
    AutoTest::CLICK, target_request_time_ + 2));

  AdClient client(AdClient::create_user(this));
  
  FAIL_CONTEXT(
    client.do_ad_requests(
      request, exp_ccids, actions));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        HourlyStats::Diffs().
          imps(1).
          requests(1).
          clicks(1).
          actions(2).
          adv_amount(2000).
          adv_comm_amount(0).
          pub_amount(0.02).
          pub_comm_amount(0).
          isp_amount(999.99),
        stat)));
}

void
StatsHourlyActionsTest::case_one_action_for_multiple_creatives_in_campaign()
{
  HourlyStats stat(
    HourlyStats::Key().
      adv_account_id(fetch_int("Adv/5")).
      stimestamp(target_request_time_));

  stat.select(pq_conn_);

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo"));
  request.debug_time(target_request_time_);

  AdClient client(AdClient::create_user(this));
  for (int i = 0; i < 2; ++i) //repeat 2 times
  {
    {
      request.tid(fetch_string("Tag Id/5/1"));
      AutoTest::CreativeList exp_ccids;
      exp_ccids.push_back(fetch_string("CC Id/5/1"));

      AutoTest::ConsequenceActionList actions;
      actions.push_back(AutoTest::TRACK);
      actions.push_back(AutoTest::CLICK);
      
      FAIL_CONTEXT(
        client.do_ad_requests(
          request, exp_ccids, actions));
    }
    {
      request.tid(fetch_string("Tag Id/5/2"));
      AutoTest::CreativeList exp_ccids;
      exp_ccids.push_back(fetch_string("CC Id/5/2"));
      
      AutoTest::ConsequenceActionList actions;
      actions.push_back(AutoTest::TRACK);
      actions.push_back(AutoTest::CLICK);
      
      FAIL_CONTEXT(
        client.do_ad_requests(
          request, exp_ccids, actions));
    }
    ActionRequest action_request;
    action_request.cid = fetch_string("CCG Id/5");
    client.process_request(action_request, "other cmp action request");
  }

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        HourlyStats::Diffs().
          imps(4).
          requests(4).
          clicks(4).
          actions(2).
          adv_amount(2000).
          adv_comm_amount(0).
          pub_amount(0.08).
          pub_comm_amount(0).
          isp_amount(999.96),
        stat)));
}

void
StatsHourlyActionsTest::case_action_for_display_creative_group_with_cpc_rate()
{
  HourlyStats stat(
    HourlyStats::Key().
      cc_id(fetch_int("CC Id/6")).
      num_shown(1).
      stimestamp(target_request_time_));

  stat.select(pq_conn_);

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.tid(fetch_string("Tag Id/6"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo"));
  request.debug_time(target_request_time_);

  AutoTest::CreativeList exp_ccids;
  exp_ccids.push_back(fetch_string("CC Id/6"));

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::TRACK);
  actions.push_back(AutoTest::CLICK);
  actions.push_back(AutoTest::ACTION);

  AdClient client(AdClient::create_user(this));
  
  FAIL_CONTEXT(
    client.do_ad_requests(
      request, exp_ccids, actions));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        HourlyStats::Diffs().
          imps(1).
          requests(1).
          clicks(1).
          actions(0).
          adv_amount(500).
          adv_comm_amount(0).
          pub_amount(0.02).
          pub_comm_amount(0).
          isp_amount(249.99),
        stat)));
}

void
StatsHourlyActionsTest::case_actions_for_text_ad_group()
{
  ORM::StatsList<HourlyStats> stats;
  std::list<HourlyStats::Diffs> diffs;

  AutoTest::NSLookupRequest request;
  request.referer("http://www.act.com");
  request.format("unit-test-imp");
  request.tid(fetch_string("Tag Id/7"));
  request.referer_kw(fetch_string("Keyword"));
  request.colo(fetch_string("Colo"));
  request.debug_time(target_request_time_);

  AutoTest::CreativeList exp_ccids;

  const char* CREATIVES[] =
  {
    "CC Id/7/1",
    "CC Id/7/2",
    "CC Id/7/3"
  };

  for (size_t i = 0; i < countof(CREATIVES); ++i)
  {
    HourlyStats stat(
    HourlyStats::Key().
      cc_id(fetch_int(CREATIVES[i])).
      num_shown(3).
      stimestamp(target_request_time_));

    stat.description(" #" + strof(i+1));
    stat.select(pq_conn_);
    
    exp_ccids.push_back(fetch_string(CREATIVES[i]));    
    stats.push_back(stat);
    diffs.push_back(
      HourlyStats::Diffs().
        imps(1).
        requests(1).
        clicks(1).
        actions(!i? 1: 0).
        adv_amount(!i? 1000: 500).
        adv_comm_amount(0).
        pub_amount(0.0066667).
        pub_comm_amount(0).
        isp_amount(!i? 499.9966666: 249.9966666) );
  }

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::TRACK);
  actions.push_back(AutoTest::CLICK);
  actions.push_back(AutoTest::ACTION);

  AdClient client(AdClient::create_user(this));

  FAIL_CONTEXT(
    client.do_ad_requests(
      request, exp_ccids, actions));


  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        stats)));
}

