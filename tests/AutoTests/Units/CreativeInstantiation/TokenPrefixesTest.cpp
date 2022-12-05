#include "TokenPrefixesTest.hpp"
#include <String/UTF8Case.hpp>

REFLECT_UNIT(TokenPrefixesTest) (
  "CreativeInstantiation",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
}

void TokenPrefixesTest::checkBody(
  const AdClient& client,
  const char* exp_body_tag)
{
 std::string expBody;
 std::string gotBody;
 String::case_change<String::Lower>(
   fetch_string(exp_body_tag),
   expBody);
 String::case_change<String::Lower>(
   client.req_response_data(),
   gotBody);
 unsigned long expLen = expBody.length();
 unsigned long gotLen = gotBody.length();
 FAIL_CONTEXT(
   AutoTest::equal_checker(
     expLen,
     gotLen).check(),
   "len of body must be expected");
 FAIL_CONTEXT(
   AutoTest::equal_checker(
     expBody,
     gotBody).check(),
   "must got expected body");
}
 
bool 
TokenPrefixesTest::run_test()

{
  tokenPrefixesSimpleFileCase();
  return true;
}

void TokenPrefixesTest::tokenPrefixesSimpleFileCase()
{
  add_descr_phrase("'TokenPrefixes.SimpleFile' test running");
  AdClient client(AdClient::create_user(this));
  NSLookupRequest request;
  request.tid = fetch_string("Tag/01");
  request.referer_kw = fetch_string("Keyword/01");
  client.process_request(request);
  checkBody(client, "Expected Body/01");
}


 
