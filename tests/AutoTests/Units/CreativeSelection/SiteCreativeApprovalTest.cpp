
#include "SiteCreativeApprovalTest.hpp"

REFLECT_UNIT(SiteCreativeApprovalTest) (
  "CreativeSelection",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;

  struct TestCase
  {
    const char* tag;
    bool ad_shown;
  };

  const TestCase CASES[] =
  {
    { "TAG/1", false },
    { "TAG/2", true },
    { "TAG/3", false },
    { "TAG/4", false },
    { "TAG/5", true },
    { "TAG/6", false },
    { "TAG/7", false }, 
    { "TAG/8", false },
    { "TAG/9", true },
    { "TAG/10", false }
  };
  
}

bool
SiteCreativeApprovalTest::run_test()
{
  AdClient client(AdClient::create_user(this));

  for (size_t i = 0; i < countof(CASES); ++i)
  {
    client.process_request(
      NSLookupRequest().
        referer_kw(fetch_string("KWD")).
        tid(fetch_string(CASES[i].tag)));

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        CASES[i].ad_shown? fetch_string("CC"): "0",
        client.debug_info.ccid).check(),
      "Check CC#" + strof(i+1));
  }
  return true;
}

