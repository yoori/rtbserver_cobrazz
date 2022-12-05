
#include "OutOfScopeRecursiveSub.hpp"
 
REFLECT_UNIT(OutOfScopeRecursiveSub) (
  "CreativeInstantiation",
  AUTO_TEST_FAST
);

namespace {
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CreativeChecker CreativeChecker;
  const unsigned short TRY_COUNT = 4;
}

 
bool 
OutOfScopeRecursiveSub::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager");
   
  add_descr_phrase("Ensure that Creative#2 status 'C'");

  FAIL_CONTEXT(
    CreativeChecker(
      this,
      fetch_int("CC2"),
      CreativeChecker::Expected().
        status("C")).check(),
    "Check creative#2 status");
      
  add_descr_phrase("Ensure that Creative#1 status 'A'");

  FAIL_CONTEXT(
    CreativeChecker(
      this,
      fetch_int("CC1"),
      CreativeChecker::Expected().
        status("A")).check(),
    "Check creative#1 status");

  add_descr_phrase("Check that Creative with high "
                   "weight doesn't match");
  NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD");
  request.tid = fetch_string("TAG");

  AdClient client(AdClient::create_user(this));

  for (unsigned short i=0; i < TRY_COUNT; ++i)
  {
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("CC1"),
        client.debug_info.ccid).check(),
      "must return creative#1");
  }
 
  return true;
}

