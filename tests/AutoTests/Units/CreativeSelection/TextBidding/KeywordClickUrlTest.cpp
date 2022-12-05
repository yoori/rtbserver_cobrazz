#include "KeywordClickUrlTest.hpp"

REFLECT_UNIT(KeywordClickUrlTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::RedirectChecker RedirectChecker;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::SelectedCreativesActualCPC SelectedCreativesActualCPC;
  typedef AutoTest::ClickResponseChecker ClickResponseChecker;
}
 
bool 
KeywordClickUrlTest::run_test()
{
  add_descr_phrase("Prepare data common");
  tid = fetch_string("Tag");
  
  add_descr_phrase("Start testing");
  NOSTOP_FAIL_CONTEXT(part1());
  NOSTOP_FAIL_CONTEXT(part2());
  NOSTOP_FAIL_CONTEXT(part3());
  NOSTOP_FAIL_CONTEXT(part4());
  add_descr_phrase("Done");

  return true;
}
 
void
KeywordClickUrlTest::part1()
{
  add_descr_phrase("Prepare data part1");
  std::string keyword1   = fetch_string("Keyword1");
  std::string click_url1 = fetch_string("click_url1");

  add_descr_phrase("Prepare server context part1");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid = tid;

  add_descr_phrase("Step 1 click_url on keyword");

  request.referer_kw = keyword1;
  client.process_request(request);
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      1,
      client.debug_info.selected_creatives.size()).check(),
    "must select creative");
  client.process_request(client.debug_info.selected_creatives.first().click_url, "click on keyword click_url");

  FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      "",
      click_url1).check(),
    "Click response check");
}

void
KeywordClickUrlTest::part2()
{
  add_descr_phrase("Prepare data part2");
  std::string keyword2   = fetch_string("Keyword2");
  std::string click_url2 = fetch_string("click_url2");

  add_descr_phrase("Prepare server context part2");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid = tid;

  add_descr_phrase("Step 2 click_url on creative option");

  request.referer_kw = keyword2;
  client.process_request(request);
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      1,
      client.debug_info.selected_creatives.size()).check(),
    "must select creative");
  client.process_request(client.debug_info.selected_creatives.first().click_url, "click on creative click_url");

  FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      "",
      click_url2).check(),
    "Click response check");
}

void
KeywordClickUrlTest::part3()
{
  add_descr_phrase("Prepare data part3");
  std::string keyword   = fetch_string("Keyword3");
  std::string click_url = fetch_string("click_url3");

  add_descr_phrase("Prepare server context part3");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid = tid;

  add_descr_phrase("Step 3 click_url on creative option with ##KEYWORD##");

  request.referer_kw = keyword;
  client.process_request(request);
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      1,
      client.debug_info.selected_creatives.size()).check(),
    "must select creative");
  client.process_request(client.debug_info.selected_creatives.first().click_url, "click on creative click_url");

  FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      "",
      click_url+keyword).check(),
    "Click response check");
}

void
KeywordClickUrlTest::part4()
{
  add_descr_phrase("Prepare data part4");
  std::string keyword   = fetch_string("Keyword4");
  std::string click_url = fetch_string("click_url4");

  add_descr_phrase("Prepare server context part4");
  AdClient client(AdClient::create_user(this));
  NSLookupRequest request;
  request.tid = tid;

  add_descr_phrase("Step 4 click_url on creative option with ##RANDOM##");

  request.referer_kw = keyword;
  client.process_request(request);
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(1,
      client.debug_info.selected_creatives.size()).check(), "must select creative");
  client.process_request(client.debug_info.selected_creatives.first().click_url, "click on creative click_url");

  FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      ClickResponseChecker::EMPTY_PUB_PRECLICK,
      String::RegEx("^CRCLICK=" + click_url + "(\\d)+$")).check(),
    "Click response check");
}
