
#include "ChangingCreativeWeightsTest.hpp"
 
REFLECT_UNIT(ChangingCreativeWeightsTest) (
  "NoDBUpdate",
  AUTO_TEST_QUIET
);

namespace {
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::CreativeChecker CreativeChecker;

  const char ASPECT[] = "ChangingCreativeWeightsTest";
  const unsigned long WARNING = Logging::Logger::WARNING;
  const unsigned long INFO = Logging::Logger::INFO;
  const unsigned long IMPS_COUNT = 2010;
}
 
bool 
ChangingCreativeWeightsTest::run()
{
  unsigned long ccid1 = fetch_int("CCW_CCID1");
  unsigned long ccid2 = fetch_int("CCW_CCID2");
  unsigned long initial_weight = 10000;

  const unsigned long ccids[] = {ccid1, ccid2};
  for (size_t i = 0; i < sizeof(ccids)/sizeof(*ccids); ++i)
  {
    FAIL_CONTEXT(
      CreativeChecker(this, ccids[i],
        CreativeChecker::Expected().
          ccid(ccids[i]).
          weight(strof(initial_weight)).
          status("A")).check(),
      "Creative#" + strof(i+1) + " initial check");
  }

  NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD");
  request.tid = fetch_string("CCW_TID");

  unsigned long ccid1_imps = 0;
  unsigned long ccid2_imps = 0;
  unsigned long ccid1_clicks = 0;
  unsigned long ccid2_clicks = 0;
  StrVector ccid1_seq;
  ccid1_seq.push_back(strof(ccid1));
  StrVector ccid2_seq;
  ccid2_seq.push_back(strof(ccid2));

  for (unsigned long i = 0; i < 2*IMPS_COUNT; i++)
  {
    AdClient client = AdClient::create_user(this);
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::or_checker(
        AutoTest::sequence_checker(
          ccid1_seq,
          SelectedCreativesCCID(client)),
      AutoTest::sequence_checker(
        ccid2_seq,
        SelectedCreativesCCID(client))).check(),
      "unexpected creatives");

    bool ccid1_shown = ( strof(ccid1) == client.debug_info.ccid );

    if(ccid1_shown)
    {
      ++ccid1_imps;
    }
    else
    {
      ++ccid2_imps;
    }
   
    if (i % 2 == 0)
    {
      std::string click_url =
        client.debug_info.selected_creatives.first().click_url;

      client.process_request(click_url, "Click on creative");

      if(ccid1_shown)
      {
        ++ccid1_clicks;
      }
      else
      {
        ++ccid2_clicks;
      }
    }
  }

  AutoTest::Logger::thlog().stream(INFO, ASPECT) <<
    "Creative1 (clicks/imps): " <<  ccid1_clicks << "/" <<  ccid1_imps;
  AutoTest::Logger::thlog().stream(INFO, ASPECT) <<
    "Creative2 (clicks/imps): " <<  ccid2_clicks << "/" <<  ccid2_imps;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccid1_imps >= 2000 || ccid2_imps >= 2000),
    "one or two creatives must have more "
    "than 2000 impression");
  

  const unsigned long weights[] =
  {
    // New weight#1
    ccid1_imps && ccid1_imps >= 2000 ?
      ( ccid1_clicks * 10000 / ccid1_imps) : initial_weight,
    // New weight#2
    ccid2_imps && ccid2_imps >= 2000 ?
    ( ccid2_clicks * 10000 / ccid2_imps) : initial_weight
  };
  
  for (size_t i = 0; i < sizeof(ccids)/sizeof(*ccids); ++i)
  {
    add_checker("Creative#" + strof(i+1) + " new weight check",
      AutoTest::wait_checker(
        CreativeChecker(this, ccids[i],
          CreativeChecker::Expected().
            ccid(ccids[i]).
            weight(strof(weights[i])).
            status("A"))));
  }
 
  return true;
}

void
ChangingCreativeWeightsTest::tear_down()
{}
