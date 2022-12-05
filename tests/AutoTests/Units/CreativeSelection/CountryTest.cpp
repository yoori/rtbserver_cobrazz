#include "CountryTest.hpp"

REFLECT_UNIT(CountryTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

void
CountryTest::process_testcase(AdClient& test_client, const char *cc_id)
{
  // Scenario:
  //
  // 1. Create a campaign creative group, and link it with some country code.
  // 2. Send several requests with different &country= parameter
  //    settings. The only matching request should be the one with the
  //    country used in step 1.
  // 3. Send the request without &country= parameter. It should not match.

  struct CountryTestResult
  {
    const char *country_code;
    bool matched;
  };

  const CountryTestResult requests[] =
    {
      { "RU", false },
      { "LU", true },
      { 0, false },
      { "JP", false },
      { "LU", true },
    };

  for (unsigned int i = 0; i < sizeof(requests) / sizeof(*requests); ++i)
  {
    add_descr_phrase("Sending advertizing request");
    if (requests[i].country_code)
    {
      request.loc_name= requests[i].country_code;
    }
    else
    {
      request.loc_name = "";
    }
    test_client.process_request(request);

    add_descr_phrase("Testing ccid");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        (!test_client.debug_info.ccid.empty() &&
          (test_client.debug_info.ccid == cc_id)) == requests[i].matched),
      "Server return wrong creative");
  }
}

 
bool 
CountryTest::run_test()
{
  add_descr_phrase("Starting"
                   " https://confluence.ocslab.com/display/ADS/CountryTest");

  AdClient test_client(AdClient::create_user(this));

  // Test that server returns creative only when country code matches.
  request.tid = fetch_string("TAG");
  request.referer_kw = fetch_string("KEYWORD");
  process_testcase(
                   test_client,
                   fetch_string("CC").c_str());


  return true;
}
