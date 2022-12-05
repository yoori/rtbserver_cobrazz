/* $Id: OptOutLoggingTest.cpp,v 1.5 2008-03-27 08:19:17 artem_nikitin Exp $
  Artem V. Nikitin
  OptOutLogging testing
 */

#include "OptOutLoggingTest.hpp"

REFLECT_UNIT(OptOutLoggingTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

//OptOutStatsDBTable=======================================================================

namespace
{

  const char* SUCCESS_URL = "http://cs.ocslab.com/opt-in/success.html";
  const char* ALREADY_URL = "http://cs.ocslab.com/opt-in/already.html";
  const char* FAIL_URL = "http://cs.ocslab.com/opt-in/fail.html";
  const unsigned long HOUR = 60*60;

  typedef AutoTest::OptOutRequest OptOutRequest;
  typedef AutoTest::RedirectChecker RedirectChecker;
}


namespace ORM = ::AutoTest::ORM;

//OptOutTest=======================================================================

bool 
OptOutLoggingTest::run_test()
{

  non_test_mode_();
  test_mode_();
  account_timezone_();

  add_descr_phrase("Check OptoutStats");
  // Check results
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_, diffs_, stats_)).check(),
    "OptOutStats: check results");

  return true;
}

void OptOutLoggingTest::non_test_mode_()
{
  std::string description("In non-test mode.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {time_, 'I', 11, 2},
    {time_, 'I', 2, 1},
    {time_, 'I', 10, 1},
    {time_, 'O', 11, 1},
    {time_, 'O', 2, 1},
    {time_, 'O', 10, 1}
  };

  initialize_stats_(
    description,
    fetch_int("COLO"),
    EXPECTED);
  
  {
    AdClient client(AdClient::create_nonoptin_user(this));

    const Request REQUESTS[] =
    {
      {time_, "in", SUCCESS_URL }, // status 11
      {time_, "in", ALREADY_URL }, // status 2
      {time_, "out", SUCCESS_URL }, // status 11
      {time_, "out", ALREADY_URL }, // status 2
      {time_, "in", SUCCESS_URL }, // status 10
    };

    process_requests_(
      description + " Part#1.",
      client,
      fetch_int("COLO"),
      REQUESTS);
  }

  {
    AdClient client(AdClient::create_nonoptin_user(this));
    
    const Request REQUESTS[] =
    {
      {time_, "out", SUCCESS_URL } // status 10
    };

    process_requests_(
      description + " Part#2.",
      client,
      fetch_int("COLO"),
      REQUESTS);
  }

  {
    AdClient client(AdClient::create_nonoptin_user(this));

    client.set_probe_uid();
    
    const Request REQUESTS[] =
    {
      {time_, "in", SUCCESS_URL } // status 11
    };

    process_requests_(
      description + " Part#3.",
      client,
      fetch_int("COLO"),
      REQUESTS);
  }
 
}

void OptOutLoggingTest::test_mode_()
{
  std::string description("In test mode.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {time_, 'I', 11, 1},
    {time_, 'I', 2, 1},
    {time_, 'I', 10, 1},
    {time_, 'O', 11, 1},
    {time_, 'O', 2, 1},
    {time_, 'O', 10, 1}
  };

  initialize_stats_(
    description,
    fetch_int("COLO"),
    EXPECTED,
    true);

  {
    AdClient client(AdClient::create_nonoptin_user(this));

    const Request REQUESTS[] =
    {
      {time_, "in", SUCCESS_URL }, // status 11
      {time_, "in", ALREADY_URL }, // status 2
      {time_, "out", SUCCESS_URL }, // status 11
      {time_, "out", ALREADY_URL }, // status 2
      {time_, "in", SUCCESS_URL }, // status 10
    };

    process_requests_(
      description + " Part#1.",
      client,
      fetch_int("COLO"),
      REQUESTS,
      true);
  }

  {
    AdClient client(AdClient::create_nonoptin_user(this));
    
    const Request REQUESTS[] =
    {
      {time_, "out", SUCCESS_URL } // status 10
    };

    process_requests_(
      description + " Part#2.",
      client,
      fetch_int("COLO"),
      REQUESTS,
      true);
  }

}

void OptOutLoggingTest::account_timezone_()
{
  std::string description("Account timezone.");
  add_descr_phrase(description);

  AutoTest::Time tz_ofset(
    AutoTest::ORM::get_tz_ofset(
      this, fetch_string("TZ")));

  AutoTest::Time time(
    AutoTest::Time().get_gm_time().get_date());


  const Expected EXPECTED[] =
  {
    {time + 6 * HOUR + tz_ofset, 'I', 11, 1},  // today 17:00
    {time + 10 * HOUR + tz_ofset, 'I', 11, 1}, // today 21:00
    {time + 15 * HOUR + tz_ofset, 'I', 11, 1}  // tomorrow 02:00
  };

  initialize_stats_(
    description,
    fetch_int("COLO/TZ"),
    EXPECTED);

  const unsigned long ofsets[] =
    { 6 * HOUR, 15 * HOUR, 10 * HOUR };

  for (size_t i = 0; i < countof(ofsets); ++i)
  {
    AdClient client(AdClient::create_nonoptin_user(this));
    
    const Request REQUESTS[] =
    {
      {time + ofsets[i], "in", SUCCESS_URL } // status 11
    };

    process_requests_(
      description + " Part#" + strof(i+1),
      client,
      fetch_int("COLO/TZ"),
      REQUESTS);
  }
}

template <size_t Count>
void OptOutLoggingTest::process_requests_(
  const std::string& description,
  AdClient& client,
  unsigned long colo,
  const Request (&requests) [Count],
  bool test)
{
  for (size_t i = 0; i < Count; ++i)
  {
    OptOutRequest request(false);
    
    request.
      op(requests[i].operation).
      colo(colo).
      debug_time(requests[i].time).
      success_url(SUCCESS_URL).
      already_url(ALREADY_URL).
      fail_url(FAIL_URL);

    if (test)
    {
      request.testrequest = true;
    }
    
    client.process_request(request);

    FAIL_CONTEXT(
      RedirectChecker(
        client,
        requests[i].redirect).check(),
      description +
      " Redirect check#" + strof(i+1));
  }
}

template <size_t Count>
void OptOutLoggingTest::initialize_stats_(
  const std::string& description,
  unsigned long colo,
  const Expected (&expects) [Count],
  bool test)
{
  for (size_t i = 0; i < Count; ++i)
  {
    Stat stat;
    
    stat.key().
      isp_sdate(expects[i].time).
      colo_id(colo).
      operation(std::string(1, expects[i].operation)).
      status(expects[i].status). 
      test(test? "Y": "N");

    stat.description(
      description + " #" + strof(i+1));

    stat.select(conn_);
    
    stats_.push_back(stat);

    diffs_.push_back(
      Diff().count(expects[i].count));
  }  
}

