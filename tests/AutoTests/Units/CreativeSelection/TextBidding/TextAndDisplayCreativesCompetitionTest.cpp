#include "TextAndDisplayCreativesCompetitionTest.hpp"

REFLECT_UNIT(TextAndDisplayCreativesCompetitionTest) (
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
  typedef AutoTest::SelectedCreativesImpRevenue SelectedCreativesImpRevenue;
  typedef AutoTest::precisely_number precisely_number;
}

bool 
TextAndDisplayCreativesCompetitionTest::run_test()
{
  AUTOTEST_CASE(
    scenario1(),
    "Display wins Keyword");
  
  AUTOTEST_CASE(
    scenario2(),
    "Keyword wins display");

  AUTOTEST_CASE(
    scenario3(),
    "Channel wins display("
    "channel eCPM equal to display eCpm)");

  AUTOTEST_CASE(
    scenario4(),
    "Channel wins display("
    "channel eCPM more than display eCpm)");

  AUTOTEST_CASE(
    scenario5(),
    "Display wins channel("
    "channel eCPM less than display eCpm)");
  
  return true;
}

void
TextAndDisplayCreativesCompetitionTest::scenario1()
{
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw = fetch_string("KEYWORD_1");
  client.process_request(request);

  std::string exp_ccids[] = {
    fetch_string("DISPLAYCC_1")
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "display ccid check#1");
  
  client.repeat_request ();

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "display ccid check#2");


}

void 
TextAndDisplayCreativesCompetitionTest::scenario2()
{
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw = fetch_string("KEYWORD_2");
  client.process_request(request);

  {
    std::string exp_ccids[] = {
      fetch_string("DISPLAYCC_2")
    };
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "display ccid check#1");
  }

  client.repeat_request ();

  {
    std::string exp_ccids[] = {
      fetch_string("TEXTCC2_1"),
      fetch_string("TEXTCC2_2")
    };
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "text ccids check#2");
  }

}

void
TextAndDisplayCreativesCompetitionTest::scenario3()
{
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw = fetch_string("KEYWORD_3");
  client.process_request(request);

  std::string exp_ccids[] = {
    fetch_string("CHANNELTGTCC3_1")
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "display ccids check");

  precisely_number exp_imp_revenues[] = {
    AutoTest::precisely_number(fetch_float("IMPREV3_1"), 0.001),
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_imp_revenues,
      SelectedCreativesImpRevenue(client)).check(),
    "imp revenue check");
}

void
TextAndDisplayCreativesCompetitionTest::scenario4()
{
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw = fetch_string("KEYWORD_4");
  client.process_request(request);

  std::string exp_ccids[] = {
    fetch_string("CHANNELTGTCC4_1"),
    fetch_string("CHANNELTGTCC4_2"),
    fetch_string("CHANNELTGTCC4_3")
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "channel text ccids check");

  precisely_number exp_imp_revenues[] = {
    precisely_number(fetch_float("IMPREV4_1"), 0.001),
    precisely_number(fetch_float("IMPREV4_2"), 0.001),
    precisely_number(fetch_float("IMPREV4_3"), 0.001)
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_imp_revenues,
      SelectedCreativesImpRevenue(client)).check(),
    "imp revenue check");
}

void
TextAndDisplayCreativesCompetitionTest::scenario5()
{
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw = fetch_string("KEYWORD_5");
  client.process_request(request);

  std::string exp_ccids[] = {
    fetch_string("DISPLAYCC_5")
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "display ccids check");

  precisely_number exp_imp_revenues[] = {
    precisely_number(fetch_float("IMPREV5_1"), 0.001)
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_imp_revenues,
      SelectedCreativesImpRevenue(client)).check(),
    "imp revenue check");
}
