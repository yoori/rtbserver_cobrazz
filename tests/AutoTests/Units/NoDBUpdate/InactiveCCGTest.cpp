
#include "InactiveCCGTest.hpp"
 
REFLECT_UNIT(InactiveCCGTest) (
  "NoDBUpdate",
  AUTO_TEST_QUIET
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CampaignChecker CampaignChecker;
}
 
bool 
InactiveCCGTest::run_test()
{
  unsigned long ccgid = fetch_int("IC_CCGID");

  FAIL_CONTEXT(CampaignChecker(
      this,
      ccgid,
      CampaignChecker::Expected().status("I").eval_status("A")).check(),
    "Inactive CCG");

  AdClient client = AdClient::create_user(this);

  client.process_request(NSLookupRequest().
    referer_kw(fetch_string("KEYWORD")).
    tid(fetch_string("IC_TID")).
    debug_time(Generics::Time::get_time_of_day()));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "mustn't return ccid for inactive CCG");

  return true;
}

