
#include "OutOfScopeToken.hpp"
 
REFLECT_UNIT(OutOfScopeToken) (
  "CreativeInstantiation",
  AUTO_TEST_SLOW
);

namespace {
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CreativeChecker CreativeChecker;
  typedef AutoTest::ORM::PQ::CreativeOptionValue CreativeOptionValue;
}


void OutOfScopeToken::set_up()
{
  add_descr_phrase("Preconditions");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager must prisent in the configuration file");
  
  set_creative_option();
}

void
OutOfScopeToken::tear_down()
{
  CreativeOptionValue option(pq_conn_);
  option.select(fetch_int("CC"),
                fetch_int("OPTION"));
  option.value = "##KEYWORD##";
  option.update();
}

bool
OutOfScopeToken::run()
{
  add_descr_phrase("Test 2.1. Static test");
  NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD");
  request.tid = fetch_string("TAG");

  {
    AdClient client(AdClient::create_user(this));
  
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "mustn't return creative");
  }

  add_descr_phrase("Test 2.2. Dynamic test - changing Text "
                   "Creative with version updating");

  CreativeOptionValue option(pq_conn_);
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      option.select(
        fetch_int("CREATIVE"),
        fetch_int("OPTION"))),
    "Can't select creative option value");
  option.value.null();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      option.update()),
    "Can't update creative option value");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CreativeChecker(
        this,
        fetch_int("CC"),
        CreativeChecker::Expected().
          status("A"))).check(),
    "Check creative status");

  {
    AdClient client(AdClient::create_user(this));
    
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("CC"),
        client.debug_info.ccid).check(),
      "must return creative");
  }

  add_descr_phrase("Postcondition");
  set_creative_option();

  return true;

}

void OutOfScopeToken::set_creative_option()
{
  CreativeOptionValue option(pq_conn_);
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      option.select(
        fetch_int("CREATIVE"),
        fetch_int("OPTION"))),
    "Can't select creative option value");
  option.value = "##KEYWORD##";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      option.update()),
    "Can't update creative option value");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CreativeChecker(
        this,
        fetch_int("CC"),
        CreativeChecker::Expected().
          status("C"))).check(),
    "Check creative status");
}

