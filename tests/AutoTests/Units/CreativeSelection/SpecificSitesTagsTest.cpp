#include "SpecificSitesTagsTest.hpp"

REFLECT_UNIT(SpecificSitesTagsTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;

bool 
SpecificSitesTagsTest::run_test()
{
  NOSTOP_FAIL_CONTEXT(case1());
  NOSTOP_FAIL_CONTEXT(case2());
  NOSTOP_FAIL_CONTEXT(case3());
  NOSTOP_FAIL_CONTEXT(case4());
  return true;
}

void
SpecificSitesTagsTest::case1()
{
  std::string cc_id1 = get_object_by_name("CC Id/1").Value();
  NSLookupRequest request;
  request.referer = "act.com";
  request.referer_kw = fetch_string("KEYWORD1");  
  request.tid = fetch_string("Tag Id/1");

  AdClient test_client = AdClient::create_user(this);  
  test_client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      cc_id1,
      SelectedCreativesCCID(test_client)).check(),
    "must got expected ccid 1 - KEYWORD1 - Tag 1");
}

void
SpecificSitesTagsTest::case2()
{  
  std::string cc_id1 = get_object_by_name("CC Id/1").Value();
  NSLookupRequest request;
  request.referer = "act.com";
  request.referer_kw = fetch_string("KEYWORD1");
  request.tid = fetch_string("Tag Id/2");

  AdClient test_client = AdClient::create_user(this);
  test_client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      cc_id1,
      SelectedCreativesCCID(test_client),
      AutoTest::SCE_NOT_ENTRY).check(),
    "must not got expected ccid 1 - KEYWORD 1 - Tag 2");
}
 
void
SpecificSitesTagsTest::case3()
{
  std::string cc_id2 = get_object_by_name("CC Id/2").Value();
  NSLookupRequest request;
  request.referer = "act.com";
  request.referer_kw = fetch_string("KEYWORD2");
  request.tid = fetch_string("Tag Id/1");

  AdClient test_client = AdClient::create_user(this);
  test_client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      cc_id2,
      SelectedCreativesCCID(test_client)).check(),
    "must got expected ccid 2 - KEYWORD 2 - Tag 1");
}

void
SpecificSitesTagsTest::case4()
{
  std::string cc_id2 = get_object_by_name("CC Id/2").Value();
  NSLookupRequest request;
  request.referer = "act.com";
  request.referer_kw = fetch_string("KEYWORD2");
  request.tid = fetch_string("Tag Id/2");

  AdClient test_client = AdClient::create_user(this);
  test_client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      cc_id2,
      SelectedCreativesCCID(test_client)).check(),
    "must got expected ccid 2 - KEYWORD 2 - Tag 2");
}
