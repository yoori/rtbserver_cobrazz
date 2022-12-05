
#include "ApplyDeliveryThresholdTest.hpp"
 
REFLECT_UNIT(ApplyDeliveryThresholdTest) (
  "NoDBUpdate",
  AUTO_TEST_SLOW
);

namespace {
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CampaignChecker CampaignChecker;

  const char ASPECT[] = "ApplyDeliveryThresholdTest";
  const unsigned long INFO = Logging::Logger::INFO;  
}
 
bool 
ApplyDeliveryThresholdTest::run_test()
{
  unsigned long ccg_id = fetch_int("ATR_CCGID");
  add_descr_phrase("Check initial state");

  Generics::Time today = Generics::Time::get_time_of_day();
  Generics::ExtendedTime gmt = today.get_gm_time();

  FAIL_CONTEXT(CampaignChecker(
      this,
      ccg_id,
      CampaignChecker::Expected().eval_status("A").status("A")).check(),
    "CCG initial state");

  add_descr_phrase("Make requests");

  // Make request
  unsigned int req_count = gmt.tm_min <= 30 ?
    // ceil((0,5 - [prev hour amount] * [30 - current min] / 60) / 0,1),
    // where [prev hour amount] = 0 (always clean run)
    static_cast<int>(ceil(0.5 / 0.1)) :
    // ceil((0,5 / ([current min] / 60) - [cur hour amount]) / 0,1),
    // where prev [cur hour amount] = 0 (always clean run)
    static_cast<int>(ceil((0.5 / (static_cast<double>(gmt.tm_min) / 60.0)) / 0.1));

  for(unsigned int i = 0; i < req_count; ++i)
  {
    AdClient client = AdClient::create_user(this);
    client.process_request(NSLookupRequest().
      referer_kw(fetch_string("KEYWORD")).
      tid(fetch_int("ATR_TID")).
      debug_time(today));
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("ATR_CCID"),
        client.debug_info.ccid).check(),
      "must return test CG ccid");
  }
  
  add_descr_phrase("Check changed state");

  FAIL_CONTEXT(AutoTest::wait_checker(
    CampaignChecker(
      this,
      ccg_id,
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
        exclude_tags("^\\[.*" + fetch_string("ATR_TID") + ".*\\]$")
      )).check(),
    "CCG excluded tag state");

  add_descr_phrase("Make check requests");
  unsigned int total_requests = 50;
  unsigned int creative_requests = 0;
  for(unsigned int i = 0; i < total_requests; ++i)
  {
    AdClient client = AdClient::create_user(this);
    client.process_request(NSLookupRequest().
      referer_kw(fetch_string("KEYWORD")).
      tid(fetch_int("ATR_TID")));
    if (client.debug_info.ccid == fetch_string("ATR_CCID"))
    {
      creative_requests++;
    }
  }

  double creative_prc =
    (static_cast<double>(creative_requests) / total_requests) * 100;

  AutoTest::Logger::thlog().stream(INFO, ASPECT) << "Creative cases: " <<
    creative_prc << "%";
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      creative_prc > 1 && creative_prc < 99),
    "Must return creative in ~50% cases");
  
  return true;
}

