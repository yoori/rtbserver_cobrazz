
#include "CombinedChannelsMatchingTest.hpp"
 
REFLECT_UNIT(CombinedChannelsMatchingTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::DebugInfoList    DebugInfoList;
}

 
bool 
CombinedChannelsMatchingTest::run_test()
{
  NSLookupRequest request;
  request.tid = fetch_string("Tag");
  {
    add_descr_phrase("Test 2.1. 1 AND 2");
    AdClient client(AdClient::create_user(this));
    request.referer = fetch_string("REF1");

    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel1")) ==
              std::string::npos),
      "mustn't have " +
      fetch_string("Channel1") +
      " in triggered_expression");

    request.referer = fetch_string("REF2");
    request.referer_kw = fetch_string("KWD2");
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel1")) !=
              std::string::npos),
      "must have " +
      fetch_string("Channel1") +
      " in triggered_expression");
    
    add_descr_phrase("Test 2.2. 1 OR 2");
    request.referer.clear();
    request.referer_kw = fetch_string("KWD3");
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel2")) !=
              std::string::npos),
      "must have " +
      fetch_string("Channel2") +
      " in triggered_expression");
      
    request.referer_kw = fetch_string("KWD4");
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel2")) !=
              std::string::npos),
      "must have " +
      fetch_string("Channel2") +
      " in triggered_expression");
    
    add_descr_phrase("Test 2.3. 1 AND NOT 2");
    request.referer = fetch_string("REF5");
    request.referer_kw = fetch_string("KWD5");
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel3")) ==
              std::string::npos),
      "mustn't have " +
      fetch_string("Channel3") +
      " in triggered_expression");

    request.referer.clear();
    request.referer_kw = fetch_string("KWD6");
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel3")) !=
              std::string::npos),
      "must have " +
      fetch_string("Channel3") +
      " in triggered_expression");
    
 
    add_descr_phrase("Test 2.4. 1 OR 2 AND NOT 3");
    request.referer_kw = fetch_string("KWD7");
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
             find(fetch_string("Channel4")) !=
               std::string::npos),
      "must have " +
      fetch_string("Channel4") +
      " in triggered_expression");
  }
  {
    AdClient client(AdClient::create_user(this));

    request.referer_kw = fetch_string("KWD8");
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel4")) !=
              std::string::npos),
        "must have " +
        fetch_string("Channel4") +
        " in triggered_expression");

    add_descr_phrase("Test 2.5. (1 OR 2) AND NOT 3");
    request.referer_kw = fetch_string("KWD9");
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel5")) ==
              std::string::npos),
      "mustn't have " +
      fetch_string("Channel5") +
      " in triggered_expression");

  }
  {
    AdClient client(AdClient::create_user(this));
    request.referer_kw = fetch_string("KWD10");
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel5")) !=
              std::string::npos),
      "must have " +
      fetch_string("Channel5") +
      " in triggered_expression");

    add_descr_phrase("Test 2.6. (1 OR 2) AND NOT (3 OR 4) AND (1 AND 2 OR 3)");
    request.referer_kw = fetch_string("KWD11");
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel6")) !=
              std::string::npos),
      "must have " +
      fetch_string("Channel6") +
      " in triggered_expression");
    
    request.referer_kw = fetch_string("KWD12");
    request.referer = fetch_string("REF12");
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.
          triggered_expression.value().
            find(fetch_string("Channel6")) ==
              std::string::npos),
      "mustn't have " +
      fetch_string("Channel6") +
      " in triggered_expressions");
  }
  return true;
}

