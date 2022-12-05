

#include "PassbackStatsTest.hpp"
#include <Generics/Time.hpp>

REFLECT_UNIT(PassbackStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  const unsigned int REQUESTCOUNT = 10;
}


void
PassbackStatsTest::process_and_check_passback(
  AdClient &client,
  std::string passback_request,
  size_t request_count)
{
  if (passback_request.empty())
  {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.find_header_value("Location", passback_request)),
      "Client must be redirected to passback frontend");
  }

  if (!request.debug_time.empty() && request_count)
  {
    passback_request +=
      request.params_prefix() + std::string("debug-time=") +
        request.debug_time.str();
  }
  // Makes several requests with the same request_id.
  for (size_t i = 0; i < request_count; ++i)
    client.process_request(passback_request);

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.find_header_value("Location", passback_request)),
    "Client must be redirected to original passback URL");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("REDIRECT_URL"),
      passback_request).check(),
    "Client must be redirected to original passback URL");
}

bool 
PassbackStatsTest::run_test()
{
  // Get default colo_id from server;
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(NSLookupRequest().
                           tid(fetch_string("TAG_0")).
                           testrequest(1).
                           debug_time(today));
    default_colo = valueof<unsigned long>(client.debug_info.colo_id);
  }

  request.referer_kw = fetch_string("EMPTY_CC_KEYWORD");
  request.passback = fetch_string("REDIRECT_URL");
  request.pt = "redir";
  request.format = "unit-test-imp";
  request.debug_time = today;

  FAIL_CONTEXT(
    scenario1(
      false,
      fetch_int("TAG_0"),
      "Simple passback requests"));
  FAIL_CONTEXT(
    scenario1(
      true,
      fetch_int("TAG_1"),
      "Passback requests with same request_ids"));
  FAIL_CONTEXT(scenario2());
  FAIL_CONTEXT(scenario3());
  FAIL_CONTEXT(scenario4());
  FAIL_CONTEXT(scenario5());
  FAIL_CONTEXT(scenario6());
  FAIL_CONTEXT(scenario7());
  // FAIL_CONTEXT(scenario8());

  add_descr_phrase("PassbackStats check.");

  try
  {
    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn_,
          diffs_,
          stats_)).check(),
      "PassbackStats: check stats");
  }
  catch(const eh::Exception&)
  {
    std::for_each(
      requests_.begin(),
      requests_.end(),
      std::bind1st(
        std::mem_fun(
          &PassbackStatsTest::log_pb_profile), this));
    throw;
  }

  return true;
}

void
PassbackStatsTest::log_pb_profile(
  std::string request_id)
{
  if (
    get_config().check_service(
      CTE_ALL, STE_EXPRESSION_MATCHER))
  {
    AutoTest::AdminsArray<AutoTest::PassbackProfileAdmin>
      admins;
    
    admins.initialize(
      this,
      CTE_ALL,
      STE_REQUEST_INFO_MANAGER,
      request_id);
    
    admins.log(AutoTest::Logger::thlog());
  }
}

void PassbackStatsTest::scenario1(
  bool same_id,
  int tag,
  const char* part)
{
  add_descr_phrase(part);

  request.tid = tag;

  PassbackStats stat;
  stat.key().colo_id(default_colo).tag_id(tag).sdate(today);
  stat.description(part);
  stat.select(conn_);
  stats_.push_back(stat);

  for (size_t i = 0; i < REQUESTCOUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(request);

    requests_.push_back(
      client.debug_info.request_id.value().c_str());
    
    process_and_check_passback(client, std::string(), same_id ? 2 : 1);
  }

  diffs_.push_back(PassbackStats::Diffs().requests(REQUESTCOUNT));
}

void
PassbackStatsTest::scenario2()
{
  std::string description("Absent redirect");
  add_descr_phrase(description);

  int tag = fetch_int("TAG_2");

  request.tid         = tag;
  request.referer_kw = fetch_string("AD_KEYWORD");

  PassbackStats stat;
  stat.key().colo_id(default_colo).tag_id(tag).sdate(today);
  stat.description(description);
  stat.select(conn_);
  stats_.push_back(stat);

  for (size_t i = 0; i < REQUESTCOUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.empty()),
      "Server must return any creative.");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        200,
        client.req_status()).check(),
      "Client must not be redirected.");
  }

  request.referer_kw = fetch_string("EMPTY_CC_KEYWORD");

  AdClient client(AdClient::create_user(this));
  client.process_request(request);
  process_and_check_passback(client);

  diffs_.push_back(PassbackStats::Diffs().requests(1));
}

void
PassbackStatsTest::scenario3()
{
  std::string description("Different tags");
  add_descr_phrase(description);

  int tag1 = fetch_int("TAG_3");
  int tag2 = fetch_int("TAG_4");

  request.referer_kw = fetch_string("EMPTY_CC_KEYWORD");
  ORM::StatsArray<PassbackStats, 2>  stats;
  stats[0].key().colo_id(default_colo).tag_id(tag1).sdate(today);
  stats[0].description(description + "#1");
  stats[1].key().colo_id(default_colo).tag_id(tag2).sdate(today);
  stats[1].description(description + "#2");
  stats.select(conn_);
  stats_.push_back(stats[0]);
  stats_.push_back(stats[1]);

  for (size_t i = 0; i < REQUESTCOUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));

    // Redirect in tag#2
    request.tid = tag2;
    client.process_request(request);
    process_and_check_passback(client);

    // Redirect in tag#1
    request.tid = tag1;
    client.process_request(request);
    process_and_check_passback(client);
  }

  diffs_.push_back(PassbackStats::Diffs().requests(REQUESTCOUNT));
  diffs_.push_back(PassbackStats::Diffs().requests(REQUESTCOUNT));
}

void
PassbackStatsTest::scenario4()
{
  std::string description("Different colocations");
  add_descr_phrase(description);

  int tag = fetch_int("TAG_5");
  int colo = fetch_int("COLO");

  request.referer_kw = fetch_string("EMPTY_CC_KEYWORD");
  request.tid = tag;

  ORM::StatsArray<PassbackStats, 2> stats;
  stats[0].key().colo_id(default_colo).tag_id(tag).sdate(today);;
  stats[0].description(description + "#1");
  stats[1].key().colo_id(colo).tag_id(tag).sdate(today);;
  stats[0].description(description + "#2");
  stats.select(conn_);
  stats_.push_back(stats[0]);
  stats_.push_back(stats[1]);

  for (size_t i = 0; i < REQUESTCOUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));

    // Redirect in colo#2
    request.colo = colo;
    client.process_request(request);
    process_and_check_passback(client);

    // Redirect in default colo
    request.colo.clear();
    client.process_request(request);
    process_and_check_passback(client);
  }

  diffs_.push_back(PassbackStats::Diffs().requests(REQUESTCOUNT));
  diffs_.push_back(PassbackStats::Diffs().requests(REQUESTCOUNT));
}

void
PassbackStatsTest::scenario5()
{
  std::string description("Different clients");
  add_descr_phrase(description);

  int tag = fetch_int("TAG_6");
  request.tid = tag ;

  PassbackStats stat;
  stat.key().colo_id(default_colo).tag_id(tag).sdate(today);
  stat.description(description);
  stat.select(conn_);
  stats_.push_back(stat);

  for (size_t i = 0; i < REQUESTCOUNT; ++i)
  {
    AdClient client1(AdClient::create_user(this));
    AdClient client2(AdClient::create_user(this));
    client2.process_request(request);

    std::string redirect_url;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client2.find_header_value("Location", redirect_url)),
      "Client must be redirected to passback frontend");

    process_and_check_passback(client1, redirect_url);
  }

  diffs_.push_back(PassbackStats::Diffs().requests(REQUESTCOUNT));
}

void
PassbackStatsTest::scenario6()
{
  std::string description("Different request dates");
  add_descr_phrase(description);

  Generics::Time tomorrow = today + 24*60*60;

  int tag = fetch_int("TAG_7");

  request.tid = tag;

  ORM::StatsArray<PassbackStats, 2>  stats;
  stats[0].key().colo_id(default_colo).tag_id(tag).sdate(today);
  stats[0].description(description + "#1");
  stats[1].key().colo_id(default_colo).tag_id(tag).sdate(tomorrow);
  stats[1].description(description + "#2");
  stats.select(conn_);
  stats_.push_back(stats[0]);
  stats_.push_back(stats[1]);

  for (size_t i = 0; i < REQUESTCOUNT; ++i)
  {
    request.debug_time = today;
    AdClient client(AdClient::create_user(this));
    
    client.process_request(request);

    std::string redirect_url;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.find_header_value("Location", redirect_url)),
      "Client must be redirected to passback frontend");
    
    if (i % 3 == 2)
    {
      request.debug_time = tomorrow;
    }
    process_and_check_passback(client, redirect_url);
  }

  diffs_.push_back(PassbackStats::Diffs()
    .requests(REQUESTCOUNT - REQUESTCOUNT / 3));
  diffs_.push_back(PassbackStats::Diffs()
    .requests(REQUESTCOUNT / 3));
}

void
PassbackStatsTest::scenario7()
{
  std::string description("Incorrect tag and colo");
  add_descr_phrase(description);

  request.debug_time = today;

  int absent_tag = fetch_int("ABSENT_TAG");
  int tag = fetch_int("TAG_8");
  int absent_colo = fetch_int("ABSENT_COLO");

  ORM::StatsArray<PassbackStats, 3>  stats;
  stats[0].key().colo_id(default_colo).tag_id(absent_tag).sdate(today);
  stats[0].description(description + "#1");
  stats[1].key().colo_id(absent_colo).tag_id(tag).sdate(today);
  stats[1].description(description + "#2");
  stats[2].key().colo_id(default_colo).tag_id(tag).sdate(today);
  stats[2].description(description + "#3");
  stats.select(conn_);
  stats_.push_back(stats[0]);
  stats_.push_back(stats[1]);
  stats_.push_back(stats[2]);

  for (size_t i = 0; i < REQUESTCOUNT; ++i)
  {
    // Invalid tag in request.
    request.tid = absent_tag;
    request.colo.clear();

    AdClient client(AdClient::create_user(this));
    
    client.process_request(request);
    process_and_check_passback(client);

    // Invalid colo in request
    request.tid  = tag;
    request.colo = absent_colo;

    client.process_request(request);
    process_and_check_passback(client);
  }

  diffs_.push_back(PassbackStats::Diffs().requests(0));
  diffs_.push_back(PassbackStats::Diffs().requests(0));
  diffs_.push_back(PassbackStats::Diffs().requests(REQUESTCOUNT));
}

void PassbackStatsTest::scenario8()
{
  std::string description("ADSC-4704");
  add_descr_phrase(description);

  int tag_1 = fetch_int("TAG_10");
  int tag_3 = fetch_int("TAG_9");
  int colo = fetch_int("COLO_2");

  request.referer_kw = fetch_string("EMPTY_CC_KEYWORD");

  ORM::StatsArray<PassbackStats, 3>  stats;
  stats[0].key().colo_id(default_colo).tag_id(tag_1).sdate(today);
  stats[0].description(description + "#1");
  stats[1].key().colo_id(colo).tag_id(tag_1).sdate(today);
  stats[1].description(description + "#2");
  stats[2].key().colo_id(default_colo).tag_id(tag_3).sdate(today);
  stats[2].description(description + "#3");
  stats.select(conn_);
  stats_.push_back(stats[0]);
  stats_.push_back(stats[1]);
  stats_.push_back(stats[2]);

  for (size_t i = 0, j = 1, t = 0; i < 4 * REQUESTCOUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));
    request.testrequest.clear();
    if (j == 4)
    {
      request.tid = tag_3;
      request.colo = default_colo;
      j = 1;
    }
    else if (j == 3)
    {
      request.tid = tag_1;
      request.colo = colo;
      j++;
    }
    else
    {
      request.tid = tag_1;
      request.colo = default_colo;
      if (j == 2)
        request.testrequest = (t++) % 2 + 1;
      j++;
    }
    client.process_request(request);
    process_and_check_passback(client);
  }

  diffs_.push_back(PassbackStats::Diffs().requests(REQUESTCOUNT));
  diffs_.push_back(PassbackStats::Diffs().requests(0));
  diffs_.push_back(PassbackStats::Diffs().requests(0));
}
