
#include "StatsHourlyNumShowingTest.hpp"
#include <math.h>

REFLECT_UNIT(StatsHourlyNumShowingTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  const unsigned short REPEAT_COUNT = 10;

  double round_pub_amount(double amount)
  {
    return ceil(amount * 100000) / 100000;
  }
}

bool 
StatsHourlyNumShowingTest::run_test()
{
  set_up();

  NOSTOP_FAIL_CONTEXT(num_shown_one_case_());
  NOSTOP_FAIL_CONTEXT(num_shown_two_case_());
  NOSTOP_FAIL_CONTEXT(num_shown_two_pub_specific_currency_case_());
  NOSTOP_FAIL_CONTEXT(num_shown_two_track_imp_case_());

  add_descr_phrase("Check RequestStatsHourly.");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        diffs_,
        stats_)).check(),
    "RequestStatsHourly: check stats");
  
  return true;
}

void StatsHourlyNumShowingTest::set_up()
{
}

void
StatsHourlyNumShowingTest::make_requests_(
  const std::string& description,
  const NSLookupRequest& request,
  const std::list<std::string>& expected_ccs,
  bool impression)
{
  AdClient client(AdClient::create_user(this));

  client.process_request(request);

  for (unsigned long i = 0; i < REPEAT_COUNT; i++)
  {
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected_ccs,
        SelectedCreativesCCID(client)).check(),
      description + " Check CCs#" + strof(i+1));

    if (impression)
    {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.track_pixel_url.empty()),
      description +
        " Check impression#" + strof(i+1));
      if (i % 2)
      {
        client.process_request(client.debug_info.track_pixel_url);
      }
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client.debug_info.track_pixel_url.empty()),
        description +
          " Check impression absence#" + strof(i+1));
    }
  }
}
  
void
StatsHourlyNumShowingTest::num_shown_one_case_()
{
  std::string description ("One CC shown.");
  add_descr_phrase(description);

  //prepare expected data
  std::list<std::string> ccid_exp;
  ccid_exp.push_back(fetch_string("OneShown/CC/1"));

  //prepare table object
  ORM::HourlyStats stat;
  stat.key(
    ORM::HourlyStats::Key().
      cc_id(fetch_int("OneShown/CC/1")).
      num_shown(1).
      stimestamp(today_));
  stat.description(description);
  stat.select(conn_);
  stats_.push_back(stat);

  // pub amount for 1 impression = PUB_AMOUNT / 1000 / num_shown
  //  (approximate to 5 digits after point)
  // num_shown = 1
  double pub_amount =
    round_pub_amount(fetch_float("PUB_CPM")/1000) * REPEAT_COUNT;

  diffs_.push_back(
    ORM::HourlyStats::Diffs().
      imps(REPEAT_COUNT).
      clicks(0).
      actions(0).
      requests(REPEAT_COUNT).
      pub_amount(
        ORM::stats_diff_type(pub_amount, 0.00001)));

  make_requests_(
    description,
    NSLookupRequest().
      referer_kw(fetch_string("OneShown/KWD")).
      tid(fetch_string("TID")).
      debug_time(today_),
    ccid_exp);
   
}


void
StatsHourlyNumShowingTest::num_shown_two_case_()
{
  std::string description ("Two CCs shown.");
  add_descr_phrase(description);

  std::list<std::string> ccid_exp;
  const char* CC[] =
  {
    "TwoShown/CC/1",
    "TwoShown/CC/2"
  };

  double pub_amount =
    round_pub_amount(fetch_float("PUB_CPM") / (2 * 1000)) * REPEAT_COUNT;

  for (size_t i = 0; i < countof(CC); ++i)
  {
    ccid_exp.push_back(fetch_string(CC[i]));

    ORM::HourlyStats stat;
    stat.key(
      ORM::HourlyStats::Key().
      cc_id(fetch_int(CC[i])).
      num_shown(2).
      stimestamp(today_));
    stat.description(description + " #" + strof(i+1));
    stat.select(conn_);
    stats_.push_back(stat);

    diffs_.push_back(
      ORM::HourlyStats::Diffs().
        imps(REPEAT_COUNT).
        clicks(0).
        actions(0).
        requests(REPEAT_COUNT).
        pub_amount(
          ORM::stats_diff_type(pub_amount, 0.00001)));
  }

  make_requests_(
    description,
    NSLookupRequest().
      referer_kw(fetch_string("TwoShown/KWD")).
      tid(fetch_string("TID")).
      debug_time(today_),
    ccid_exp);
}


void
StatsHourlyNumShowingTest::num_shown_two_pub_specific_currency_case_()
{
  std::string description("Non-system currency case.");
  add_descr_phrase(description);
  
  const unsigned int exchange_rate = fetch_int("EXCHANGE_RATE");
  
  std::list<std::string> ccid_exp;
  const char* CC[] =
  {
    "NonSystemCurrency/CC/1",
    "NonSystemCurrency/CC/2"
  };

  double pub_amount =
    round_pub_amount(
      fetch_float("PUB_CPM") * exchange_rate / ( 2 * 1000)) * REPEAT_COUNT;

  for (size_t i = 0; i < countof(CC); ++i)
  {
    ccid_exp.push_back(fetch_string(CC[i]));

    ORM::HourlyStats stat;
    stat.key(
      ORM::HourlyStats::Key().
      cc_id(fetch_int(CC[i])).
      num_shown(2).
      stimestamp(today_));
    stat.description(description + " #" + strof(i+1));
    stat.select(conn_);
    stats_.push_back(stat);

    diffs_.push_back(
      ORM::HourlyStats::Diffs().
        imps(REPEAT_COUNT).
        clicks(0).
        actions(0).
        requests(REPEAT_COUNT).
        pub_amount(
          ORM::stats_diff_type(pub_amount, 0.00001)));
  }

  make_requests_(
    description,
    NSLookupRequest().
      referer_kw(fetch_string("NonSystemCurrency/KWD")).
      tid(fetch_string("TIDCURRENCY")).
      debug_time(today_),
    ccid_exp);
}

void
StatsHourlyNumShowingTest::num_shown_two_track_imp_case_()
{
  std::string description("Impression tracking case.");
  add_descr_phrase(description);
  
  //prepare expected data
  std::list<std::string> ccid_exp;
  const char* CC[] =
  {
    "ImprTrack/CC/1",
    "ImprTrack/CC/2"
  };

  double pub_amount =
    round_pub_amount(
      fetch_float("PUB_CPM") / (2 * 1000)) * REPEAT_COUNT / 2;
  
  for (size_t i = 0; i < countof(CC); ++i)
  {
    ccid_exp.push_back(fetch_string(CC[i]));

    ORM::HourlyStats stat;
    stat.key(
      ORM::HourlyStats::Key().
      cc_id(fetch_int(CC[i])).
      num_shown(2).
      stimestamp(today_));
    stat.description(description + " #" + strof(i+1));
    stat.select(conn_);
    stats_.push_back(stat);

    diffs_.push_back(
      ORM::HourlyStats::Diffs().
        imps(REPEAT_COUNT / 2).
        clicks(0).
        actions(0).
        requests(REPEAT_COUNT).
        pub_amount(
          ORM::stats_diff_type(pub_amount, 0.0000001)));
  }

  make_requests_(
    description,
    NSLookupRequest().
      referer_kw(fetch_string("ImprTrack/KWD")).
      tid(fetch_string("TID")).
      debug_time(today_).
      format("unit-test-imp"),
    ccid_exp, true);
}
