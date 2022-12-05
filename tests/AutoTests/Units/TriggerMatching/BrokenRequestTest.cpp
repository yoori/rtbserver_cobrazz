
#include "BrokenRequestTest.hpp"
 
REFLECT_UNIT(BrokenRequestTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::ChannelsCheck ChannelsCheck;
}

void
BrokenRequestTest::test_case(
  const std::string& description,
  AdClient& client,
  const NSLookupRequest& request,
  const char* expected,
  const char* unexpected)
{
  add_descr_phrase(description);

  client.process_request(request);

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      expected,
      client.debug_info.trigger_channels).check(),
    description +
        ". Expected trigger_channels check");

  if (unexpected)
  {
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        unexpected,
        client.debug_info.trigger_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      description +
        ". Expected trigger_channels check");
  }
}
 
bool 
BrokenRequestTest::run_test()
{
  AdClient client(AdClient::create_user(this));

  NOSTOP_FAIL_CONTEXT(
    test_case(
      "Broken referrer keywords",
      client,
      NSLookupRequest().
        referer_kw(fetch_string("KWD1")).
        referer(fetch_string("REF1")),
      "Channel1,Channel2"));
  
  NOSTOP_FAIL_CONTEXT(
    test_case(
      "Broken referrer",
      client,
      NSLookupRequest().
        referer_kw(fetch_string("KWD2")).
        referer(fetch_string("REF2")),
      "Channel1",
      "Channel2"));

  NOSTOP_FAIL_CONTEXT(
    test_case(
      "Broken referrer and referrer keywords",
      client,
      NSLookupRequest().
        referer_kw(fetch_string("KWD1")).
        referer(fetch_string("REF2")),
      "Channel1",
      "Channel2"));

  NOSTOP_FAIL_CONTEXT(
    test_case(
      "Broken encoding in referrer",
      client,
      NSLookupRequest().
        search(fetch_string("SEARCH3")),
      "Channel3_1,Channel3_2"));

  return true;
}

