#include "ActionRequestsTests.hpp"

REFLECT_UNIT(ActionRequestsTests) (
  "Statistics",
  AUTO_TEST_SLOW
);
 
namespace
{
  const char* const LU = "LU";
  const char* const UG = "UG";
  const char* const GB = "GB";

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ActionRequest ActionRequest;
  typedef AutoTest::OptOutRequest OptOutRequest;
  typedef AutoTest::AdClient AdClient;

  const char* REFERER1    = "www.referer1.actionrequesttest.com";
  const char* REFERER2    = "www.referer2.actionrequesttest.com";
}

namespace ORM = AutoTest::ORM;

bool 
ActionRequestsTests::run_test()
{
  AutoTest::DBC::Conn conn(open_pq());

  test_part1 (conn);

  return true;
}


void make_request_series (AdClient& client, 
                          ActionRequest request,
                          int count)
{
  for(int i = 0; i < count; ++i)
  {
    client.process_request(request);
  }
}

void make_request_series (
  AdClient& client, 
  int count, 
  unsigned long action, 
  const char* country,
  const AutoTest::Time& debug_time)
{
  ActionRequest request;
  request.actionid  = action;
  request.country.clear();
  if (country)
  {
    request.country   = country;
  }
  request.debug_time = debug_time;
  make_request_series(client, request, count);
}

void make_request_series (
  AdClient& client, 
  int count, 
  unsigned long action, 
  const char* country,
  const char* referer,
  const AutoTest::Time& debug_time)
{
  ActionRequest request;
  request.actionid  = action;
  request.country   = country;
  request.debug_time = debug_time;
  request.referer = referer;
  for(int i = 0; i < count; ++i)
  {
    client.process_request(request);
  }
}

void
ActionRequestsTests::test_part1 (AutoTest::DBC::IConn& conn)
{
  AdClient client(AdClient::create_nonoptin_user(this));
  
  unsigned long tid1  = fetch_int("Tag1");
  unsigned long colo1 = 1;
  unsigned long colo2 = fetch_int("COLO");
  unsigned long action1 = fetch_int("Action1");
  unsigned long action2 = fetch_int("Action2");
  unsigned long action3 = 999999;

  std::string keyword1 = fetch_string("Keyword1");
  
  ORM::StatsArray<ORM::ActionRequests, 10> stats;
  stats[0].key().
    colo_id(colo1).
    action_id(action1).
    country_code(UG).
    user_status("P").
    action_date(base_time);
  stats[1].key().
    colo_id(colo1).
    action_id(action1).
    country_code("").
    user_status("U").
    action_date(base_time);
  stats[2].key().
    colo_id(colo1).
    action_id(action2).
    country_code(LU).
    user_status("P").
    action_date(base_time);
  stats[3].key().
    colo_id(colo1).
    action_id(action1).
    country_code(LU).
    user_status("I").
    action_date(base_time);
  stats[4].key().
    colo_id(colo1).
    action_id(action2).
    country_code(UG).
    user_status("I").
    action_date(base_time);
  stats[5].key().
    colo_id(colo1).
    action_id(action1).
    country_code(UG).
    user_status("O").
    action_date(base_time);
  stats[6].key().
    colo_id(colo1).
    action_id(action2).
    country_code(LU).
    user_status("O").
    action_date(base_time);
  stats[7].key().
    colo_id(colo1).
    action_id(action3).
    country_code(LU).
    user_status("U").
    action_date(base_time);
  stats[8].key().
    colo_id(colo1).
    action_id(action3).
    country_code(LU).
    user_status("I").
    action_date(base_time);
  stats[9].key().
    colo_id(colo1).
    action_id(action3).
    country_code(LU).
    user_status("O").
    action_date(base_time);

  stats.select(conn);
  add_descr_phrase("Undefined user");
  make_request_series(client,
    1, action1, 0, base_time);

  add_descr_phrase("Probe user");
  client.set_probe_uid();
 
  make_request_series(client, 6, action1, UG, base_time);
  make_request_series(client, 5, action2, LU, base_time);
  add_descr_phrase("Persistent user");
  client.process_request(OptOutRequest().op("in"));
  // unexisting action id - oo (N)
  make_request_series(client, 1, action3, LU, base_time);

  {
    NSLookupRequest request;
    request.tid(tid1);
    request.colo(colo2);
    request.referer_kw(keyword1);
    request.debug_time = base_time;

    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        1,
        client.debug_info.selected_creatives.size()).check(),
      "must got 1 creative");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.click_url.empty()),
      "response must have valid click_url debug info value");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.first().action_adv_url.empty()),
      "response must have valid action_adv_url debug info value");
  }
  make_request_series(client, 4, action1, LU, REFERER1, base_time);

  make_request_series(client, 3, action2, UG, base_time);
  // unexisting action id - oo (I)
  make_request_series(client, 1, action3, LU, base_time);

  add_descr_phrase("Optout user");
  {
    OptOutRequest request;
    request.op = "out";
    client.process_request(request);
  }
  make_request_series(client, 2, action1, UG, REFERER2, base_time);
  make_request_series(client, 1, action2, LU, base_time);
  // unexisting action id - oo 
  make_request_series(client, 1, action3, LU, base_time);

  add_descr_phrase("Check table");

  typedef   ORM::ActionRequests::Diffs Diff;
  const Diff diff[] =
  {
    Diff().count(6),
    Diff().count(1),
    Diff().count(5),
    Diff().count(4),
    Diff().count(3),
    Diff().count(2),
    Diff().count(1),
    Diff(0),
    Diff(0),
    Diff(0)
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn, diff, stats)).check(),
    "ActionRequests check");
}
