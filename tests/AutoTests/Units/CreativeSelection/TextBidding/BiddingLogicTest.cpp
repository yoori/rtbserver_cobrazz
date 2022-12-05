#include "BiddingLogicTest.hpp"

REFLECT_UNIT(BiddingLogicTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::SelectedCreativesActualCPC SelectedCreativesActualCPC;
}

bool 
BiddingLogicTest::run_test()
{ 
  NOSTOP_FAIL_CONTEXT(scenario1 ());
  NOSTOP_FAIL_CONTEXT(scenario2 ());
  NOSTOP_FAIL_CONTEXT(scenario3 ());
  NOSTOP_FAIL_CONTEXT(scenario4 ());
  NOSTOP_FAIL_CONTEXT(scenario5 ());
  NOSTOP_FAIL_CONTEXT(scenario6 ());
  NOSTOP_FAIL_CONTEXT(scenario7 ());
  NOSTOP_FAIL_CONTEXT(scenario8 ());
  NOSTOP_FAIL_CONTEXT(scenario9 ());
  return true;
}

void 
BiddingLogicTest::scenario1 ()
{
  add_descr_phrase("'BiddingLogicTest' Filtering by tag eCPM (one winner)");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("Tag01");
  request.referer_kw = fetch_string("Key02");
  client.process_request(request, "for making keyword#1 context");
  client.repeat_request ("text advertising");
  std::string exp_ccids[] = {
    fetch_string("CC01")
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives ccid");
}

void 
BiddingLogicTest::scenario2 ()
{
  add_descr_phrase("'BiddingLogicTest' Filtering by tag eCPM (two winners)");
  AdClient client(AdClient::create_user(this));
 
  NSLookupRequest request;
  request.tid         = fetch_string("Tag01");
  request.referer_kw = fetch_string("Key03");
  client.process_request(request,  "for making keyword#2 context");
  client.repeat_request ("text advertising");
  std::string exp_ccids[] = {
    fetch_string("CC01"),
    fetch_string("CC04")
  };
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives ccid");
}

void 
BiddingLogicTest::scenario3 ()
{
  add_descr_phrase("'BiddingLogicTest' Filtering by account");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("Tag02");
  request.referer_kw = fetch_string("Key01") + "," + fetch_string("Key01");
  client.process_request(request, "for making keyword#1, keyword#2 context");
  client.repeat_request ("text advertising");
  std::string exp_ccids[] = {
    fetch_string("CC03")
  };
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives ccid");
}

void 
BiddingLogicTest::scenario4 ()
{
  add_descr_phrase("'BiddingLogicTest' eCPM group");
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("Tag01");
  request.referer_kw = fetch_string("Key01") + "," + fetch_string("Key04");
  client.process_request(request, "for making keyword#1, keyword#3 context");
  client.repeat_request ("text advertising");

  // 'selected_creatives' may contain creatives in any order,
  // because creatives have same ecpmBid for this case.
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      2,
      SelectedCreativesCCID(client).size()).check(),
    "unexpected selected_creatives size");

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("CC03"),
      SelectedCreativesCCID(client)).check(),
    "ccid must entry to selected_creatives");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("CC05"),
      SelectedCreativesCCID(client)).check(),
    "ccid must entry to selected_creatives");
}

void 
BiddingLogicTest::scenario5 ()
{
  add_descr_phrase("'BiddingLogicTest' MIN ECPM from display campain CPM");
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("Tag01");
  request.referer_kw = fetch_string("Key05");
  client.process_request(request, "for making keyword#4 context");
  client.repeat_request ("text advertising");
  std::string exp_ccids[] = {
    fetch_string("CC04"),
    fetch_string("CC05")
  };
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives ccid");
}


void 
BiddingLogicTest::scenario6 ()
{
  add_descr_phrase("'BiddingLogicTest' Filtering by campaign (M)");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("Tag03");
  request.referer_kw = fetch_string("Key06");
  client.process_request(request, "for making keyword context");
  client.repeat_request ("text advertising");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      2,
      client.debug_info.selected_creatives.size()).check(),
    "must got 2 creatives");
  std::string exp_ccids_1[] =
  {
    fetch_string("CC05_1"),
    fetch_string("CC05_3")
  };

  std::string exp_ccids_2[] =
  {
    fetch_string("CC05_2"),
    fetch_string("CC05_3")
  };

  FAIL_CONTEXT(
    AutoTest::or_checker(
      AutoTest::sequence_checker(
        exp_ccids_1,
        SelectedCreativesCCID(client)),
      AutoTest::sequence_checker(
        exp_ccids_2,
        SelectedCreativesCCID(client))).check(),
    "selected_creatives ccid first variant");
}

void 
BiddingLogicTest::scenario7 ()
{
  add_descr_phrase("'BiddingLogicTest' Filtering by advertiser (A)");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("Tag03");
  request.referer_kw = fetch_string("Key07");
  client.process_request(request, "for making keyword context");
  client.repeat_request ("text advertising");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      2,
      client.debug_info.selected_creatives.size()).check(),
    "must got 2 creatives");
  std::string exp_ccids[] = {
    fetch_string("CC06_2"),
    fetch_string("CC06_1")
  };
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives ccid");
}

void 
BiddingLogicTest::scenario8 ()
{
  add_descr_phrase("'BiddingLogicTest' Filtering by account (O)");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("Tag03");
  request.referer_kw = fetch_string("Key08");
  client.process_request(request, "for making keyword context");
  client.repeat_request ("text advertising");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      2,
      client.debug_info.selected_creatives.size()).check(),
    "must got 2 creatives");
  std::string exp_ccids[] = {
    fetch_string("CC07_1"),
    fetch_string("CC07_2")
  };
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives ccid");
}

void 
BiddingLogicTest::scenario9 ()
{
  add_descr_phrase("'BiddingLogicTest' One winner "
                   "(tag with cpm = 0 and margin = 0");
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("TagNoMargin");
  request.referer_kw = fetch_string("Key02");
  request.colo        = fetch_string("NoMarginColo");
  client.process_request(request, "for making keyword#1 context");
  client.repeat_request ("text advertising");
  std::string exp_ccids[] = {
    fetch_string("CC01")
  };
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives ccid");
}
