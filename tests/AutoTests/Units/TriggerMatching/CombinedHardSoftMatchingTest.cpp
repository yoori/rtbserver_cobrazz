#include "CombinedHardSoftMatchingTest.hpp"

REFLECT_UNIT(CombinedHardSoftMatchingTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

typedef AutoTest::NSLookupRequest  NSLookupRequest;
typedef AutoTest::AdClient AdClient;
 
bool 
CombinedHardSoftMatchingTest::run_test()
{
  add_descr_phrase("XML data initializing");
  
  std::string required_channel = fetch_string("CombinedHardSoftMatchingTest/01");
 
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid = tid;
  request.referer = "google.com";

  add_descr_phrase("Sending requests for server validation");

  client.process_request(request.referer_kw("Test5 Test3 Test4"));
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      required_channel, 
      client.debug_info.trigger_channels +
        client.debug_info.trigger_channels).check(),
    "must have required channel");

  client.process_request(request.referer_kw("Test5 Test4 Test3"));
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      required_channel,
      client.debug_info.trigger_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "must not have required channel in trigger_channels");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      required_channel,
      client.debug_info.trigger_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "must not have required channel in trigger_channels");

  return true;
}
 
