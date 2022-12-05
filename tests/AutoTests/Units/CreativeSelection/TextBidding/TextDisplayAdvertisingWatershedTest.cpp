#include "TextDisplayAdvertisingWatershedTest.hpp"

REFLECT_UNIT(TextDisplayAdvertisingWatershedTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

namespace ORM = AutoTest::ORM;

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::SelectedCreativesActualCPC SelectedCreativesActualCPC;
  typedef AutoTest::DebugInfo::SelectedCreativesList SelectedCreativesList;
  typedef AutoTest::Money Money;
}
 
bool 
TextDisplayAdvertisingWatershedTest::run_test()
{
  AUTOTEST_CASE(
    display_wins_(),
    "Display ads wins");

  AUTOTEST_CASE(
    text_wins_(),
    "TA ads win");

  return true;
}

void
TextDisplayAdvertisingWatershedTest::display_wins_()
{
  AdClient client(AdClient::create_user(this));
  NSLookupRequest request;
  request.tid = fetch_string("Tag");
  request.referer_kw = fetch_string("Keyword_1") + "," +
    fetch_string("Keyword_3"); 
  client.process_request(request);
    
  std::string exp_ccids[] = {
    fetch_string("DisplayCC")
  };

  Money exp_acpc[] = {
    fetch_string("DisplayCPC")
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "Display ccid check");
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_acpc,
      SelectedCreativesActualCPC(client)).check(),
    "Display revenue check");  
}

void
TextDisplayAdvertisingWatershedTest::text_wins_()
{
  AdClient client(AdClient::create_user(this));
  NSLookupRequest request;
  request.tid = fetch_string("Tag");
  request.referer_kw = fetch_string("Keyword_1") + "," +
    fetch_string("Keyword_2");
  client.process_request(request);
  
  std::string exp_ccids[] = {
    fetch_string("TACC")
  };
  
  Money exp_acpc[] = {
    fetch_string("TACPC")
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "TA ccid check");
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_acpc,
      SelectedCreativesActualCPC(client)).check(),
    "TA revenue check");  
}
  
 
