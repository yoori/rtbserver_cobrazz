#include "OptOutFreqCapTest.hpp"

REFLECT_UNIT(OptOutFreqCapTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);
 
void
OptOutFreqCapTest::process_testcase(AdClient& test_client)
{
  // Scenario:
  // 1. opted-out user send advertizing request for campaign with
  //    disabled frequency caps (campaign have no link with the FreqCaps
  //    table (Freq_cap_id=null in tables Campaign, CampaignCreative and
  //    CampaignCreativeGroup)) & action tracking (no TRACK_ACTIONS flag
  //    and CPA == 0).
  // 2. test that server return creative.

  // 3. opted-out user send advertizing request for campaign with
  //    disabled frequency caps (campaign have reference to frequency
  //    caps with period=0) & enabled action tracking.
  // 4. test that server return creative.
  
  // 5. opted-out user send advertizing request for campaign with
  //    enabled frequency caps (campaign have reference to frequency
  //    caps with period != 0)
  // 6. test that server do not return creative.

  add_descr_phrase("FreqCaps is null");
  test_client.process_request(NSLookupRequest().
        tid(fetch_string("Tag-1")).
        referer("www.OptOutFreqCapTest.com").
        colo(fetch_string("Colo")));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !test_client.debug_info.ccid.empty()),
    "Server has returned empty ccid");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC-1"),
      test_client.debug_info.ccid).check(),
    "Server has returned not expected ccid");
  
  add_descr_phrase("FreqCaps with period = 0");
  test_client.process_request(NSLookupRequest().
        tid(fetch_string("Tag-2")).
        referer("www.OptOutFreqCapTest.com").
        colo(fetch_string("Colo")));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !test_client.debug_info.ccid.empty()),
    "Server has returned empty ccid");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC-2"),
      test_client.debug_info.ccid).check(),
    "Server has returned not expected ccid");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !test_client.debug_info.selected_creatives.empty()),
    "Server has returned selected creatives");

  add_descr_phrase("FreqCaps with period != 0");
  test_client.process_request(NSLookupRequest().
        tid(fetch_string("Tag-3")).
        referer("www.OptOutFreqCapTest.com").
        colo(fetch_string("Colo")));
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      test_client.debug_info.ccid).check(),
    "Server has returned not empty ccid");
}


bool 
OptOutFreqCapTest::run_test()
{
  add_descr_phrase(
                   "Starting https://confluence.ocslab.com/display/ADS/OptOutFreqCapTest");

  AdClient test_client(AdClient::create_nonoptin_user(this));

  test_client.process_request(OptOutRequest().op("out"));

  process_testcase(test_client);


  return true;
}
