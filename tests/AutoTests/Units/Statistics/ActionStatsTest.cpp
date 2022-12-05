
// TODO: remove sleep's - use debug time instead it
#include "ActionStatsTest.hpp"
#include <Generics/Time.hpp>
#include <Generics/Rand.hpp>

REFLECT_UNIT(ActionStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW,
  AUTO_TEST_SERIALIZE
);

namespace
{
  const char* const US = "US";
  const char* const RU = "RU";
  const char* const GB = "GB";
  const char* const GN = "GN";

  const char* const ACTION_REFERER = "http://cs.ocslab.com/cgi-bin/attest.cgi";

  const int MINUTE = 60; // secs

  typedef AutoTest::ORM::ActionStats ActionStats;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  
  const int NULL_DATE = -std::numeric_limits<int>::max();
  const double UNUSED_VALUE = -std::numeric_limits<double>::max();

  const char* CHAR_100 = "b01234567890123456789012345678901234"
    "567890123456789012345678901234567890123456789012345678901234567e";
  const char* CHAR_101 = "b01234567890123456789012345678901234"
    "5678901234567890123456789012345678901234567890123456789012345678e";
  const char* CHAR_101_DB = "b01234567890123456789012345678901234"
    "5678901234567890123456789012345678901234567890123456789012345678";
  const char* CHAR_QUOTED = "\"01234567890123456789012345678901234"
    "567890123456789012345678901234567890123456789012345678901234567\"";

  std::string generate_random_url(size_t length)
  {
      std::string result(length, 0);
      size_t size = 0;

      while (size < length)
      {
        char c = static_cast<char>(Generics::safe_rand(74)) + 48;
        if (String::AsciiStringManip::ALPHA_NUM(c) || c == '_')
        {
          result[size++] = c;
        }
      }

      return result;
  }
}

void
ActionStatsTest::set_up()
{ }

bool
ActionStatsTest::run()
{

  AutoTest::AdClient client(
    AutoTest::AdClient::create_user(this));

  AUTOTEST_CASE(
    referrer_test_(client),
    "Referrer test");

  std::list<std::string> imps;
  std::list<std::string> clicks;
  
  AUTOTEST_CASE(
    base_case_part_1_(client, imps, clicks),
    "Base case");
  AUTOTEST_CASE(
    cross_action_(),
    "Cross action");

  AUTOTEST_CASE(
    imp_update_(),
    "Last impression update");

  AUTOTEST_CASE(
    text_ads_(),
    "Text advertising");

  AUTOTEST_CASE(
    expired_profile_(),
    "Expired profile");

  AUTOTEST_CASE(
    conversation_value_(client),
     "Conversation value");

  AUTOTEST_CASE(
    deleted_action_(),
    "Deleted action");
  
  check();
 
  AUTOTEST_CASE(
    base_case_part_2_(client, imps),
    "Base case");

  check();

  AUTOTEST_CASE(
    base_case_part_3_(client, clicks),
    "Base case");

  check();

  AUTOTEST_CASE(
    base_case_part_4_(client),
    "Base case");

  // Must be last, because change test time
  AUTOTEST_CASE(
    conversation_orderid_(client),
    "Conversation order_id");

  return true;
}


void
ActionStatsTest::tear_down()
{ }

void
ActionStatsTest::initialize_stat_(
  ORM::ActionRequests& stat,
  const CaseStat& expected)
{
  stat.key().
    action_id(fetch_int(expected.action)).
    action_date(base_time + expected.time_ofset).
    colo_id(
      expected.colo?
      fetch_int(expected.colo): 1).
    country_code(
      expected.country?
      expected.country: GN).
    user_status("I");
}

void
ActionStatsTest::initialize_stat_(
  ORM::ActionStats& stat,
  const CaseStat& expected)
{
  stat.key().
    action_id(fetch_int(expected.action)).
    action_date(base_time + expected.time_ofset);

  if (expected.cc && expected.tid)
  {
    stat.key().
      cc_id(fetch_int(expected.cc)).
      tag_id(fetch_int(expected.tid)).
      colo_id(
        expected.colo?
        fetch_int(expected.colo): 1).
      country_code(
        expected.country?
        expected.country: GN).
      imp_date(base_time + expected.imp_ofset);
    
    if (expected.click_ofset != NULL_DATE)
    {
      stat.key().          
        click_date(
          base_time + expected.click_ofset);
    }
    else
    {
      stat.key().click_date_set_null();
    }
  }
}

void
ActionStatsTest::initialize_stat_(
  ORM::ActionStats& stat,
  const ConversationStat& expected)
{

  stat.key().
    action_id(fetch_int(expected.action)).
    action_date(base_time).
    order_id(expected.order_id? expected.order_id: "");
  
  if (expected.cc)
  {
    stat.key().cc_id(fetch_int(expected.cc));
  }
  if (expected.tid)
  {
    stat.key().tag_id(fetch_int(expected.tid));
  }

  if (expected.cur_value != UNUSED_VALUE)
  {
    stat.key().cur_value(expected.cur_value);
  }
}

void
ActionStatsTest::initialize_stat_(
  ORM::ActionRequests& stat,
  const char* action)
{
  stat.key().
    action_id(fetch_int(action)).
    action_date(base_time).
    user_status("I");
}


template<class Stat, class Expected, size_t Count>
void ActionStatsTest::initialize_stats_(
  ORM::StatsList<Stat>& stats_array,
  const Expected(&expected) [Count])
{
  for (unsigned int i = 0; i < Count; ++i)
  {
    Stat stat;

    initialize_stat_(stat, expected[i]);
    
    stat.description("#" + strof(i+1));
    stat.select(pq_conn_);
    stats_array.push_back(stat);
  }
}

template<size_t Count>
void
ActionStatsTest::process_conversations_(
  AdClient& client,
  ConversationRequest::Member param,
  const ConversationAction(&requests) [Count])
{
  for (unsigned int i = 0; i < Count; ++i)
  {
    ConversationRequest request;
    param(request, requests[i].value);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        requests[i].status,
        client.process(
          request.
           convid(fetch_string(requests[i].action)).
           debug_time(base_time),
          true)).check(),
      "Status check#" + strof(i+1));
  }
}


void 
ActionStatsTest::base_case_part_1_(
  AdClient& client,
  std::list<std::string>& imps,
  std::list<std::string>& clicks)
  
{

  ActionStats stats;

  stats.key().
    action_id(fetch_int("Base/ACTION/2")).
    cc_id(fetch_int("Base/CCID/2")).
    tag_id(fetch_int("Base/TID/2")).
    colo_id(1).
    country_code(RU).
    imp_date(base_time).
    action_date(base_time).
    click_date_set_null();

  stats.select(pq_conn_);

  ORM::StatsArray <ActionStats, 3>  unexpected;
  unexpected[0].key().
    action_id(fetch_int("Base/ACTION/1")).
    action_date(base_time);
  unexpected[0].description("#1");
  unexpected[1].key().
    action_id(fetch_int("Base/ACTION/3")).
    action_date(base_time);
  unexpected[1].description("#2");
  unexpected[2].key().
    action_id(fetch_int("Base/ACTION/4")).
    action_date(base_time);
  unexpected[2].description("#3");
  unexpected.select(pq_conn_);

  ORM::StatsArray<ORM::ActionRequests, 3> ar_stats;
  ar_stats[0].key().
    action_id(fetch_int("Base/ACTION/1")).
    country_code(US).
    action_date(base_time).
    user_status("I");
  ar_stats[0].description("#1");
  ar_stats[1].key().
    action_id(fetch_int("Base/ACTION/6")).
    country_code(US).
    colo_id(1).
    action_date(base_time).
    action_referrer_url(ACTION_REFERER).
    user_status("I");
  ar_stats[1].description("#1");
  ar_stats[2].key().
    action_id(fetch_int("Base/ACTION/5")).
    colo_id(1).
    action_date(base_time).
    user_status("I");
  ar_stats[2].description("#2");

  ar_stats.select(pq_conn_);
  
  client.process_request(NSLookupRequest());
  
  client.process_request(
    ActionRequest().
      debug_time(base_time).
      actionid(fetch_int("Base/ACTION/3")).
      country(RU));

  client.process_request(
    ActionRequest().
      debug_time(base_time).
      actionid(fetch_int("Base/ACTION/6")).
      referer(ACTION_REFERER).
      country(US));

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("Base/KEYWORD")).
      tid(fetch_int("Base/TID/1")).
      loc_name(US).
      format("unit-test-imp").
      debug_time(base_time + 5));
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(fetch_int("Base/CCID/1")),
      client.debug_info.ccid).check(),
    "must got expected ccid");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.click_url.empty()),
    "must have debug_info.click_url");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.selected_creatives.first().action_adv_url.empty()),
    "must have debug_info.action_adv_url");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.track_pixel_url.empty()),
    "must got track_pixel_url");

  // Save URLs
  imps.push_back(client.debug_info.track_pixel_url);
  clicks.push_back(client.debug_info.click_url);

  // Process second ad-request
  client.process_request(
    NSLookupRequest().
      referer(fetch_string("Base/URL")).
      tid(fetch_int("Base/TID/2")).
      loc_name(RU).
      format("unit-test").
      debug_time(base_time + 6));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(fetch_int("Base/CCID/2")),
      client.debug_info.ccid).check(),
    "must got expected ccid");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.click_url.empty()),
    "must have debug_info.click_url");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.selected_creatives.first().action_adv_url.empty()),
    "must have debug_info.action_adv_url");

  // Save URLs
  clicks.push_back(client.debug_info.click_url);

  // Process first ad-request
  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("Base/KEYWORD")).
      tid(fetch_int("Base/TID/3")).
      loc_name(US).
      format("unit-test-imp").
      debug_time(base_time));
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(fetch_int("Base/CCID/3")),
      client.debug_info.ccid).check(),
    "must got expected ccid");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.click_url.empty()),
    "must have debug_info.click_url");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.selected_creatives.first().action_adv_url.empty()),
    "must have debug_info.action_adv_url");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.track_pixel_url.empty()),
    "must got track_pixel_url");

  // Save URLs
  imps.push_back(client.debug_info.track_pixel_url);
  clicks.push_back(client.debug_info.click_url);

  client.process_request(
    ActionRequest().
      debug_time(base_time + 10).
      actionid(fetch_int("Base/ACTION/4")).
      country(GB));
  
  client.process_request(
    ActionRequest().
      debug_time(base_time + 10).
      actionid(fetch_int("Base/ACTION/1")).
      country(US));

  client.process_request(
    ActionRequest().
      debug_time(base_time + 10).
      actionid(fetch_int("Base/ACTION/5")).
      country(US));

  client.process_request(
    ActionRequest().
      debug_time(base_time + 10).
      actionid(fetch_int("Base/ACTION/2")).
      country(RU));

  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, 1, stats));

  ADD_WAIT_CHECKER(
    "ActionStats check (unexpected)",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 0, unexpected));

  ADD_WAIT_CHECKER(
    "ActionRequests check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ActionRequests::Diffs().
        count(1),
      ar_stats));

};


void 
ActionStatsTest::base_case_part_2_(
  AdClient& client,
  std::list<std::string>& imps)
{
  ORM::StatsArray<ActionStats, 2> stats;
  stats[0].key().
    action_id(fetch_int("Base/ACTION/1")).
    cc_id(fetch_int("Base/CCID/1")).
    tag_id(fetch_int("Base/TID/1")).
    colo_id(1).
    country_code(US).
    imp_date(base_time).
    action_date(base_time).
    click_date_set_null();
  stats[0].description("#1");
  stats[1].key().
    action_id(fetch_int("Base/ACTION/6")).
    cc_id(fetch_int("Base/CCID/4")).
    tag_id(fetch_int("Base/TID/4")).
    colo_id(1).
    country_code(US).
    imp_date(base_time - 1).
    action_date(base_time).
    action_referrer_url(ACTION_REFERER).
    click_date_set_null();
  stats[1].description("#2");
  stats.select(pq_conn_);
   
  ORM::StatsArray <ActionStats, 3>  unexpected;
  unexpected[0].key().
    action_id(fetch_int("Base/ACTION/2")).
    action_date(base_time);
  unexpected[0].description("#1");
  unexpected[1].key().
    action_id(fetch_int("Base/ACTION/4")).
    action_date(base_time);
  unexpected[1].description("#2");
  unexpected[2].key().
    action_id(fetch_int("Base/ACTION/5"));
  unexpected[2].description("#3");
  unexpected.select(pq_conn_);

  ORM::StatsArray <ORM::ActionRequests, 3> ar_stats;
  ar_stats[0].key().
    action_id(fetch_int("Base/ACTION/1")).
    action_date(base_time);
  ar_stats[0].description("#1");
  ar_stats[1].key().
    action_id(fetch_int("Base/ACTION/5"));
  ar_stats[1].description("#2");
  ar_stats[2].key().
    action_id(fetch_int("Base/ACTION/6"));
  ar_stats[2].description("#3");

  ar_stats.select(pq_conn_);

  // Navigate to track_pixel_url from first ad-request
  {
    std::string debug_time;
    
    String::StringManip::mime_url_encode(
      (base_time + 7).get_gm_time().format(
        AutoTest::DEBUG_TIME_FORMAT),
      debug_time);
  
    client.process_request(
      imps.front() +
      "&debug-time=" +
      debug_time);

    imps.pop_front();
  }

  client.process_request(
    ActionRequest().
      debug_time(base_time + 60).
      actionid(fetch_int("Base/ACTION/1")).
      country(RU));

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("Base/KEYWORD")).
      tid(fetch_int("Base/TID/4")).
      loc_name(US).
      format("unit-test").
      debug_time(base_time-1));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(fetch_int("Base/CCID/4")),
      client.debug_info.ccid).check(),
    "must got expected ccid");

  {
    std::string debug_time;
    
    String::StringManip::mime_url_encode(
      (base_time + 61).get_gm_time().format(
        AutoTest::DEBUG_TIME_FORMAT),
      debug_time);
  
    client.process_request(
      imps.front() +
      "&debug-time=" +
      debug_time);

    imps.pop_front();
  }

  {
    const ActionStats::Diffs diffs[] =
    {
      ActionStats::Diffs(2),
      ActionStats::Diffs(1)
    };
    
    ADD_WAIT_CHECKER(
      "ActionStats check",
      AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats));

  }

  ADD_WAIT_CHECKER(
    "ActionStats check (unexpected)",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 0, unexpected));

  {
    const ORM::ActionRequests::Diffs diffs[] =
    {
      ORM::ActionRequests::Diffs().
        count(1),
      ORM::ActionRequests::Diffs(0),
      ORM::ActionRequests::Diffs(0)
    };

    ADD_WAIT_CHECKER(
      "ActionRequests check",
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        ar_stats));
  }

}


void 
ActionStatsTest::base_case_part_3_(
  AdClient& client,
  std::list<std::string>& clicks)
{
  // Navigate to click_url form 1st and 2nd ad-requests
  client.process_request(clicks.front() +
    "*amp*debug-time*eql*" +
      base_time.get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT));
  clicks.pop_front();
  client.process_request(clicks.front()  +
    "*amp*debug-time*eql*" +
      base_time.get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT));
  clicks.pop_front();

  ORM::StatsArray <ActionStats, 4> stats;

  stats[0].key().
    action_id(fetch_int("Base/ACTION/1")).
    cc_id(fetch_int("Base/CCID/1")).
    tag_id(fetch_int("Base/TID/1")).
    colo_id(1).
    country_code(US).
    imp_date(base_time).
    click_date(base_time).
    action_date(base_time);
  stats[0].description("#1");
  stats[1].key().
    action_id(fetch_int("Base/ACTION/2")).
    cc_id(fetch_int("Base/CCID/2")).
    tag_id(fetch_int("Base/TID/2")).
    colo_id(1).
    country_code(RU).
    imp_date(base_time).
    click_date(base_time).
    action_date(base_time);
  stats[1].description("#2");
  stats[2].key().
    action_id(fetch_int("Base/ACTION/1")).
    action_date(base_time);
  stats[2].description("#3");
  stats[3].key().
    action_id(fetch_int("Base/ACTION/2")).
    action_date(base_time);
  stats[3].description("#4");

  stats.select(pq_conn_);

  client.process_request(
    ActionRequest().
      debug_time(base_time + 120).
      actionid(fetch_int("Base/ACTION/1")).
      country(GB));

  client.process_request(
    ActionRequest().
      debug_time(base_time + 120).
      actionid(fetch_int("Base/ACTION/2")).
      country(GB));

  const ActionStats::Diffs diffs[] =
  {
    // one new record + two changed old records (added click_date)
    ActionStats::Diffs(3),
    // one new record + one changed old record (added click_date)
    ActionStats::Diffs(2),
    // but only one new record for action#1 (others is old with added click_date)
    ActionStats::Diffs(1),
    ActionStats::Diffs(1),
  };

  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void 
ActionStatsTest::base_case_part_4_(
  AdClient& client)
{
  ActionStats stats;
  stats.key().
    action_id(fetch_int("Base/ACTION/2")).
    cc_id(fetch_int("Base/CCID/2")).
    tag_id(fetch_int("Base/TID/2")).
    colo_id(1).
    country_code(RU).
    imp_date(base_time).
    click_date(base_time).
    action_date(base_time);
  stats.select(pq_conn_);

  for (int i = 0; i < 5; ++i)
  {
    client.process_request(
      ActionRequest().
        debug_time(base_time + 180).
        actionid(fetch_int("Base/ACTION/2")).
        country(GB));
  }

  for (int i = 0; i < 3; i++)
  {
    client.process_request(
      ActionRequest().
        debug_time(base_time + 240).
        actionid(fetch_int("Base/ACTION/2")).
        country(GB));
  }

  // two new records for action#2
  // see https://jira.ocslab.com/browse/ADSC-2063
 ADD_WAIT_CHECKER(
   "ActionStats check",
   AutoTest::stats_diff_checker(
     pq_conn_, 2, stats));
};

void
ActionStatsTest::cross_action_()
{
  // Stats initialization
  ORM::StatsList<ORM::ActionStats> stats;

  const CaseStat case_stats[] =
  {
    { "CrossAction/ACTION/2", "CrossAction/CCID/1_1",
      "CrossAction/TID/2", 0, GN, 0, 0, NULL_DATE },
    { "CrossAction/ACTION/2", "CrossAction/CCID/1_2",
      "CrossAction/TID/3", 0, GN, 0, 0, NULL_DATE },
    { "CrossAction/ACTION/2", "CrossAction/CCID/2",
      "CrossAction/TID/1", 0, GN, 0, 0, NULL_DATE },
    { "CrossAction/ACTION/1", "CrossAction/CCID/1_1",
      "CrossAction/TID/2", 0, GN, 0, 0, NULL_DATE },
    { "CrossAction/ACTION/1", "CrossAction/CCID/1_2",
      "CrossAction/TID/3", 0, GN, 0, 0, NULL_DATE }
  };

  initialize_stats_(stats, case_stats);

  // Test requests
  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().referer_kw(
      fetch_string("CrossAction/KEYWORD/2")).
      debug_time(base_time));

  client.process_request(
    NSLookupRequest().tid(
      fetch_int("CrossAction/TID/1")).
    debug_time(base_time));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CrossAction/CCID/2"),
      client.debug_info.ccid).check(),
    "Check CC2");

  client.process_request(
    NSLookupRequest().referer_kw(
      fetch_string("CrossAction/KEYWORD/1")).
    debug_time(base_time));

  client.process_request(
    NSLookupRequest().tid(
      fetch_int("CrossAction/TID/2")).
    debug_time(base_time+20));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CrossAction/CCID/1_1"),
      client.debug_info.ccid).check(),
    "Check CC1_1");

  client.process_request(
    NSLookupRequest().tid(
      fetch_int("CrossAction/TID/3")).
    debug_time(base_time+40));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CrossAction/CCID/1_2"),
      client.debug_info.ccid).check(),
    "Check CC1_2");

  client.process_request(
    ActionRequest().actionid(
      fetch_int("CrossAction/ACTION/2")).
    debug_time(base_time+60));

  client.process_request(
    ActionRequest().actionid(
      fetch_int("CrossAction/ACTION/1")).
    debug_time(base_time+60));

  
  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 1, stats));
    
}

void
ActionStatsTest::imp_update_()
{
  // Stats initialization
  ORM::StatsList<ORM::ActionStats> stats;
  
  const CaseStat case_stats[] =
  {
    { "ImpUpdate/ACTION", "ImpUpdate/CCID",
      "ImpUpdate/TID/1", 0, GN,
      0, 0, 0 },
    { "ImpUpdate/ACTION", "ImpUpdate/CCID",
      "ImpUpdate/TID/2", "ImpUpdate/COLO", GN,
      0, 0, NULL_DATE }
  };

  initialize_stats_(stats, case_stats);

  // Test requests

  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().referer_kw(
      fetch_string("ImpUpdate/KEYWORD")).
      debug_time(base_time));

  client.process_request(
    NSLookupRequest().tid(
      fetch_int("ImpUpdate/TID/1")).
    debug_time(base_time));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("ImpUpdate/CCID"),
      client.debug_info.ccid).check(),
    "Check CC#1");

  std::string click_url(
    client.debug_info.click_url);

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !click_url.empty()),
    "must have debug_info.click_url");

  client.process_request(click_url);

  client.process_request(
    NSLookupRequest().tid(
      fetch_int("ImpUpdate/TID/2")).
    debug_time(base_time+30).
    colo(fetch_int("ImpUpdate/COLO")));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("ImpUpdate/CCID"),
      client.debug_info.ccid).check(),
    "Check CC#2");

  client.process_request(
    ActionRequest().actionid(
      fetch_int("ImpUpdate/ACTION")).
    debug_time(base_time+60));

  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 1, stats));
}

void
ActionStatsTest::text_ads_()
{
  // Stats initialization

  ORM::StatsList<ORM::ActionStats> stats;
  
  const CaseStat case_stats[] =
  {
    { "TextAds/ACTION", "TextAds/CCID/1",
      "TextAds/TID", 0, GN, 0, 0, NULL_DATE },
    { "TextAds/ACTION", "TextAds/CCID/2",
      "TextAds/TID", 0, GN, 0, 0, NULL_DATE }
  };

  initialize_stats_(stats, case_stats);

   // Test requests

  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().referer_kw(
      fetch_string("TextAds/KEYWORD")).
    tid(fetch_int("TextAds/TID")).
    debug_time(base_time));

  client.repeat_request();
  
  std::string exp_ccids[] =
  {
    fetch_string("TextAds/CCID/1"),
    fetch_string("TextAds/CCID/2")
  };

 
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "Check CCs");

  client.process_request(
    ActionRequest().actionid(
      fetch_int("TextAds/ACTION")).
    debug_time(base_time));

  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 1, stats));
}

void
ActionStatsTest::deleted_action_()
{
  ORM::StatsList<ORM::ActionStats> a_stats;
  ORM::StatsList<ORM::ActionRequests> ar_stats;
  
  const CaseStat case_stats[] =
  {
    { "Deleted/ACTION", 0, 0, 0, 0, 0, 0, 0 }
  };

  initialize_stats_(a_stats, case_stats);
  initialize_stats_(ar_stats, case_stats);

  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().referer_kw(
      fetch_string("Deleted/KEYWORD")).
    tid(fetch_int("Deleted/TID")).
    debug_time(base_time));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Deleted/CCID"),
      client.debug_info.ccid).check(),
    "Check CC#2");

  client.process_request(
    ActionRequest().
      actionid(
        fetch_int("Deleted/ACTION")).
      debug_time(base_time));

  
  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 0, a_stats));

  ADD_WAIT_CHECKER(
    "ActionRequest check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ActionRequests::Diffs(0),
      ar_stats));
}

void
ActionStatsTest::expired_profile_()
{
  ORM::StatsList<ORM::ActionStats> a_stats;
  ORM::StatsList<ORM::ActionRequests> ar_stats;

  {
    const CaseStat case_stats[] =
    {
      { "Expired/ACTION", 0, 0, 0, 0, 0, 0, 0 },
      {
        "Expired/ACTION", "Expired/CCID/2",
        "Expired/TID/2", 0, GN, 0, 0, NULL_DATE }
    };

    initialize_stats_(a_stats, case_stats);
  }

  {
    const CaseStat case_stats[] =
    {
      {
        "Expired/ACTION", 0, 0, 0, GN, 0, 0, 0 }
    };

    initialize_stats_(ar_stats, case_stats);
  }

  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().referer_kw(
      fetch_string("Expired/KEYWORD/1")).
    tid(fetch_int("Expired/TID/1")).
    debug_time(base_time - (8*24*60*60 + 18*60*60 + 60*60) ));

  std::string request_id(
    client.debug_info.creative_request_id.value());
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::RequestProfileChecker(
        this,
        "\\" + request_id,
        AutoTest::RequestInfoManager,
        AutoTest::RequestProfileChecker::Expected().
          fraud("0"))).check(),
    "Wait request profile");

  AutoTest::AdminsArray<AutoTest::RequestProfileAdmin> admins;
  
  admins.initialize(
    this,
    CTE_ALL,
    STE_REQUEST_INFO_MANAGER,
    "\\" + request_id);
  
  admins.log(AutoTest::Logger::thlog());

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Expired/CCID/1"),
      client.debug_info.ccid).check(),
    "Check CC#1");  
    
  client.process_request(
    NSLookupRequest().referer_kw(
      fetch_string("Expired/KEYWORD/2")).
    tid(fetch_int("Expired/TID/2")).
    debug_time(base_time));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Expired/CCID/2"),
      client.debug_info.ccid).check(),
    "Check CC#2");

  AutoTest::ClearExpiredProfiles::execute(this);

  admins.log(AutoTest::Logger::thlog());

  client.process_request(
    ActionRequest().
      actionid(
        fetch_int("Expired/ACTION")).
      country(GN).
      debug_time(base_time));

  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 1, a_stats));

  ADD_WAIT_CHECKER(
    "ActionRequests check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ActionRequests::Diffs().
        count(1),
      ar_stats));
}

void
ActionStatsTest::conversation_value_(
  AdClient& client)
{

  ORM::StatsList<ORM::ActionStats> a_stats;
  ORM::StatsList<ORM::ActionRequests> ar_stats;

  {
    const ConversationStat EXPECTED[] =
      {
        { "Conversation/ACTION/1", 0, 0, UNUSED_VALUE, 0},
        { "Conversation/ACTION/2", 0, 0, UNUSED_VALUE, 0},
        { "Conversation/ACTION/3", 0, 0, UNUSED_VALUE, 0},
        { "Conversation/ACTION/4", "Conversation/CCID", "Conversation/TID", 2.3, 0},
        { "Conversation/ACTION/5", "Conversation/CCID", "Conversation/TID", UNUSED_VALUE, 0},
        { "Conversation/ACTION/6", "Conversation/CCID", "Conversation/TID", 5.12345678, 0},
        { "Conversation/ACTION/8", 0, 0, UNUSED_VALUE, 0}
      };

    initialize_stats_(a_stats, EXPECTED);
  }

  {
    const char* EXPECTED[] =
    {
      "Conversation/ACTION/1",
      "Conversation/ACTION/2",
      "Conversation/ACTION/3",
      "Conversation/ACTION/4",
      "Conversation/ACTION/5", 
      "Conversation/ACTION/6",
      "Conversation/ACTION/8"
    };

    initialize_stats_(ar_stats, EXPECTED);
  }

  client.process_request(
    NSLookupRequest().
      referer_kw(
        fetch_string("Conversation/KEYWORD")).
      tid(fetch_int("Conversation/TID")).
      debug_time(base_time));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Conversation/CCID"),
      client.debug_info.ccid).check(),
    "Check CC");  

  const ConversationAction REQUESTS[] =
  {
    { "Conversation/ACTION/1", "", 200 },
    { "Conversation/ACTION/4", "0", 200 },
    { "Conversation/ACTION/5", "5", 200 },
    { "Conversation/ACTION/5", "4", 200 },
    { "Conversation/ACTION/6", "5.12345678", 200 },
    { "Conversation/ACTION/2", "5.123456789", 200 },
    { "Conversation/ACTION/3", "1,2", 200 },
    { "Conversation/ACTION/8", "abc", 200 }
  };

  process_conversations_(client, &ConversationRequest::value, REQUESTS);


  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 1, a_stats));

  {
    typedef ORM::ActionRequests::Diffs Diff;
    ADD_WAIT_CHECKER(
      "ActionRequests check",
      AutoTest::stats_diff_checker(
        pq_conn_,
        {
          Diff().
           count(1).
           cur_value(0),
          Diff().
           count(1).
           cur_value(5.12345679),
          Diff().
           count(1).
           cur_value(1),
          Diff().
            count(1).
            cur_value(2.3),
          Diff().
            count(2).
            cur_value(9),
          Diff().
            count(1).
            cur_value(5.12345678),
          Diff().
            count(1).
            cur_value(0),
          Diff(0).
            count(1).
            cur_value(0)
        },
        ar_stats));
  }
}

void
ActionStatsTest::conversation_orderid_(
 AdClient& client)
{
  ORM::StatsList<ORM::ActionStats> a_stats;
  ORM::StatsList<ORM::ActionRequests> ar_stats;

  base_time = AutoTest::Time();

  {
    const ConversationStat EXPECTED[] =
      {
        { "Conversation/ACTION/1", "Conversation/CCID", "Conversation/TID", 0, 0},
        { "Conversation/ACTION/7", "Conversation/CCID", "Conversation/TID", 1, "abc"},
        { "Conversation/ACTION/4", "Conversation/CCID", "Conversation/TID", 2.3, "\"abc\""},
        { "Conversation/ACTION/5", "Conversation/CCID", "Conversation/TID", 0, "'abc'"},
        { "Conversation/ACTION/6", "Conversation/CCID", "Conversation/TID", 0, "abc,def"},
        { "Conversation/ACTION/2", "Conversation/CCID", "Conversation/TID", 0, "abc%0Adef"},
        { "Conversation/ACTION/3", "Conversation/CCID", "Conversation/TID", 3.7, "abc%0Ddef"},
        { "Conversation/ACTION/8", "Conversation/CCID", "Conversation/TID", 0, "abc%0D%0Adef"},
        { "Conversation/ACTION/9", "Conversation/CCID", "Conversation/TID", 0, CHAR_100},
        { "Conversation/ACTION/10", "Conversation/CCID", "Conversation/TID", 0, CHAR_101_DB},
        { "Conversation/ACTION/11", "Conversation/CCID", "Conversation/TID", 0, CHAR_QUOTED},
        { "Conversation/ACTION/12", "Conversation/CCID", "Conversation/TID", 0,
          fetch_string("Conversation/ORDER/1").c_str()},
        { "Conversation/ACTION/13", "Conversation/CCID", "Conversation/TID", 0,
          fetch_string("Conversation/ORDER/2").c_str()},
        { "Conversation/ACTION/14", "Conversation/CCID", "Conversation/TID", 2, "qwerty"},
      };

    initialize_stats_(a_stats, EXPECTED);
  }

  {
    const char* EXPECTED[] =
    {
      "Conversation/ACTION/1",
      "Conversation/ACTION/7",
      "Conversation/ACTION/4",
      "Conversation/ACTION/5",
      "Conversation/ACTION/6",
      "Conversation/ACTION/2",
      "Conversation/ACTION/3",
      "Conversation/ACTION/8",
      "Conversation/ACTION/9",
      "Conversation/ACTION/10", 
      "Conversation/ACTION/11",
      "Conversation/ACTION/12",
      "Conversation/ACTION/13",
      "Conversation/ACTION/14"
    };

    initialize_stats_(ar_stats, EXPECTED);
  }


  const ConversationAction REQUESTS[] =
  {
    { "Conversation/ACTION/1", "", 200 },
    { "Conversation/ACTION/7", "abc", 200 },
    { "Conversation/ACTION/4", "\"abc\"", 200 },
    { "Conversation/ACTION/5", "'abc'", 200 },
    { "Conversation/ACTION/6", "abc,def", 200 },
    { "Conversation/ACTION/2", "abc\ndef", 200 },
    { "Conversation/ACTION/3", "abc\rdef", 200 },
    { "Conversation/ACTION/8", "abc\r\ndef", 200 },
    { "Conversation/ACTION/9", CHAR_100, 200 },
    { "Conversation/ACTION/10", CHAR_101, 200 },
    { "Conversation/ACTION/11", CHAR_QUOTED, 200 },
    { "Conversation/ACTION/12", fetch_string("Conversation/ORDER/1").c_str(), 200 },
    { "Conversation/ACTION/13", fetch_string("Conversation/ORDER/2").c_str(), 200 }
  };
  
  process_conversations_(client, &ConversationRequest::orderid, REQUESTS);

  client.process_request(
    ConversationRequest().
      convid(fetch_string("Conversation/ACTION/14")).
      orderid("qwerty").
      value(2).
      debug_time(base_time));

  ADD_WAIT_CHECKER(
    "ActionStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_, 1, a_stats));

  {
    typedef ORM::ActionRequests::Diffs Diff;
    ADD_WAIT_CHECKER(
      "ActionRequests check",
      AutoTest::stats_diff_checker(
        pq_conn_,
        {
          // Action#1
          Diff().
            count(1).
            cur_value(0),
          // Action#7 
          Diff().
            count(1).
            cur_value(1),
          // Action#4
          Diff().
            count(1).
            cur_value(2.3),
          // Action#5
          Diff().
            count(1).
            cur_value(0),
          // Action#6
          Diff().
            count(1).
            cur_value(0),
          // Action#2
          Diff().
            count(1).
            cur_value(0),
          // Action#3
          Diff().
            count(1).
            cur_value(3.7),
          // Action#8
          Diff().
            count(1).
            cur_value(0),
          // Action#9
          Diff().
            count(1).
            cur_value(0),
          // Action#10
          Diff().
            count(1).
            cur_value(0),
          // Action#11
          Diff().
            count(1).
            cur_value(0),
          // Action#12
          Diff().
            count(1).
            cur_value(0),
          // Action#13
          Diff().
            count(1).
            cur_value(0),
          // Action#14
          Diff().
            count(1).
            cur_value(2)
        },
        ar_stats));
  }  
}

void
ActionStatsTest::referrer_test_(
 AdClient& client)
{
  const std::string base_url1 = "http://a.com/";
  const std::string base_url2 = "http://c.co.uk/€";

  const std::string url1 = base_url1 + generate_random_url(2047 - base_url1.length());
  const std::string url2 = base_url1 + generate_random_url(2048 - base_url1.length());
  const std::string url3 = base_url1 + generate_random_url(2049 - base_url1.length());
  const std::string url4 = "http://b.net/%D0%B2%D0%BE%D1%99%D1";
  const std::string url5 = "http://b.net/\xD0\xD1%B2%D0%BE%D1%99";
  const std::string url6 = base_url2 + generate_random_url(2047 - base_url2.length());

  const std::string stored_url1 = url1;
  const std::string stored_url2 = url2;
  const std::string stored_url3 = url3.substr(0, url3.size() - 4) + "...";
  const std::string stored_url4 = url4;
  const std::string stored_url5 = "http://b.net/%D0%D1%B2%D0%BE%D1%99";
  const std::string stored_url6 = "http://c.co.uk/%E2%82%AC" + url6.substr(18, 2021) + "...";

  ORM::StatsArray<ORM::ActionRequests, 6> ar_stats;
  ar_stats[0].key().action_id(fetch_int("Referrer/ACTION/1")).action_referrer_url(stored_url1.c_str());
  ar_stats[1].key().action_id(fetch_int("Referrer/ACTION/2")).action_referrer_url(stored_url2.c_str());
  ar_stats[2].key().action_id(fetch_int("Referrer/ACTION/3")).action_referrer_url(stored_url3.c_str());
  ar_stats[3].key().action_id(fetch_int("Referrer/ACTION/4")).action_referrer_url(stored_url4.c_str());
  ar_stats[4].key().action_id(fetch_int("Referrer/ACTION/5")).action_referrer_url(stored_url5.c_str());
  ar_stats[5].key().action_id(fetch_int("Referrer/ACTION/6")).action_referrer_url(stored_url6.c_str());
  ar_stats.select(pq_conn_);

  ORM::StatsArray<ORM::ActionStats, 6> stats;
  stats[0].key().action_id(fetch_int("Referrer/ACTION/1")).action_referrer_url(stored_url1.c_str());
  stats[1].key().action_id(fetch_int("Referrer/ACTION/2")).action_referrer_url(stored_url2.c_str());
  stats[2].key().action_id(fetch_int("Referrer/ACTION/3")).action_referrer_url(stored_url3.c_str());
  stats[3].key().action_id(fetch_int("Referrer/ACTION/4")).action_referrer_url(stored_url4.c_str());
  stats[4].key().action_id(fetch_int("Referrer/ACTION/5")).action_referrer_url(stored_url5.c_str());
  stats[5].key().action_id(fetch_int("Referrer/ACTION/6")).action_referrer_url(stored_url6.c_str());
  stats.select(pq_conn_);

  client.process_request(
    NSLookupRequest().
      referer_kw(
        fetch_string("Referrer/KEYWORD")).
      tid(fetch_int("Referrer/TID")).
      debug_time(base_time));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Referrer/CCID"),
      client.debug_info.ccid).check(),
    "Check CC");

  const ConversationAction REQUESTS[] =
  {
    { "Referrer/ACTION/1", url1.c_str(), 200 },
    { "Referrer/ACTION/2", url2.c_str(), 200 },
    { "Referrer/ACTION/3", url3.c_str(), 200 },
    { "Referrer/ACTION/4", url4.c_str(), 200 },
    { "Referrer/ACTION/5", url5.c_str(), 200 },
    { "Referrer/ACTION/6", url6.c_str(), 200 }
  };

  process_conversations_(client, &ConversationRequest::referer, REQUESTS);

  ADD_WAIT_CHECKER(
    "ActionRequests check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      stats));

  ADD_WAIT_CHECKER(
    "ActionRequests check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ActionRequests::Diffs().count(1),
      ar_stats));
}
