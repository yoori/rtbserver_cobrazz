
#include "TextChannelAndKeywordCreativesCompetition.hpp"

REFLECT_UNIT(TextChannelAndKeywordCreativesCompetition) (
  "CreativeSelection",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::SelectedCreativesActualCPC SelectedCreativesActualCPC;
  typedef AutoTest::SelectedCreativesImpRevenue SelectedCreativesImpRevenue;
  typedef AutoTest::DebugInfo::SelectedCreativesList SelectedCreativesList;
  typedef AutoTest::precisely_number precisely_number;
}

bool
TextChannelAndKeywordCreativesCompetition::run_test()
{
  NOSTOP_FAIL_CONTEXT(scenario1());
  NOSTOP_FAIL_CONTEXT(scenario2());
  NOSTOP_FAIL_CONTEXT(scenario3());
  NOSTOP_FAIL_CONTEXT(scenario4());
  NOSTOP_FAIL_CONTEXT(scenario5());
  NOSTOP_FAIL_CONTEXT(scenario6());
  return true;
}

void
TextChannelAndKeywordCreativesCompetition::scenario1()
{
  add_descr_phrase("2 Text and one channel.");
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw =
    fetch_string("TEXTKEYWORD1") + "," +
    fetch_string("TEXTKEYWORD2") + "," +
    fetch_string("CHANNELKEYWORD1");
  client.process_request(request);
  client.repeat_request ();

  std::string exp_ccids[] = {
    fetch_string("TEXTCC1"),
    fetch_string("CHANNELCC1"),
    fetch_string("TEXTCC2")
  };

  precisely_number exp_click_revenues[] = {
    precisely_number(fetch_float("CLICKREV1_1"), 0.001),
    precisely_number(fetch_float("CLICKREV1_2"), 0.001),
    precisely_number(fetch_float("CLICKREV1_3"), 0.001)
  };

  precisely_number exp_imp_revenues[] = {
    precisely_number(fetch_float("IMPREV1_1"), 0.001),
    precisely_number(fetch_float("IMPREV1_2"), 0.001),
    precisely_number(fetch_float("IMPREV1_3"), 0.001)
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "ccids check");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_click_revenues,
      SelectedCreativesActualCPC(client)).check(),
    "click revenue check");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_imp_revenues,
      SelectedCreativesImpRevenue(client)).check(),
    "imp revenue check");
}

void
TextChannelAndKeywordCreativesCompetition::scenario2()
{
  add_descr_phrase("2 Text on top and one channel.");
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw =
    fetch_string("TEXTKEYWORD1a") + "," +
    fetch_string("TEXTKEYWORD2a") + "," +
    fetch_string("CHANNELKEYWORD2");
  client.process_request(request);
  client.repeat_request ();

  std::string exp_ccids[] = {
    fetch_string("TEXTCC1"),
    fetch_string("TEXTCC2"),
    fetch_string("CHANNELCC2")
  };

  precisely_number exp_click_revenues[] = {
    precisely_number(fetch_float("CLICKREV2_1"), 0.001),
    precisely_number(fetch_float("CLICKREV2_2"), 0.001),
    precisely_number(fetch_float("CLICKREV2_3"), 0.001)
  };

  precisely_number exp_imp_revenues[] = {
    precisely_number(fetch_float("IMPREV2_1"), 0.001),
    precisely_number(fetch_float("IMPREV2_2"), 0.001),
    precisely_number(fetch_float("IMPREV2_3"), 0.001)
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "ccids check");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_click_revenues,
      SelectedCreativesActualCPC(client)).check(),
    "click revenue check");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_imp_revenues,
      SelectedCreativesImpRevenue(client)).check(),
    "imp revenue check");
}

void
TextChannelAndKeywordCreativesCompetition::scenario3()
{
  add_descr_phrase("Text(C) CCG ecpm > ccg keyword ecpm.");
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw =
    fetch_string("TEXTKEYWORD2b") + "," +
    fetch_string("CHANNELKEYWORD1");
  client.process_request(request);
  client.repeat_request ();

  std::string exp_ccids[] = {
    fetch_string("CHANNELCC1"),
    fetch_string("TEXTCC2")
  };

  precisely_number exp_click_revenues[] = {
    precisely_number(fetch_float("CLICKREV3_1"), 0.001),
    precisely_number(fetch_float("CLICKREV3_2"), 0.001)
  };

  precisely_number exp_imp_revenues[] = {
    precisely_number(fetch_float("IMPREV3_1"), 0.001),
    precisely_number(fetch_float("IMPREV3_2"), 0.001)
  };
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "ccids check");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_click_revenues,
      SelectedCreativesActualCPC(client)).check(),
    "click revenue check");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_imp_revenues,
      SelectedCreativesImpRevenue(client)).check(),
    "imp revenue check");
}

void
TextChannelAndKeywordCreativesCompetition::scenario4()
{
  add_descr_phrase("Text(C) CCG ecpm = ccg keyword ecpm.");
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw =
    fetch_string("TEXTKEYWORD1b") + "," +
    fetch_string("CHANNELKEYWORD3");
  client.process_request(request);
  client.repeat_request ();

  std::list<std::string> exp_ccids;
  exp_ccids.push_back(fetch_string("TEXTCC1"));
  exp_ccids.push_back(fetch_string("CHANNELCC3"));

  std::list<std::string> exp_ccids_reverse(exp_ccids);
  exp_ccids_reverse.reverse();

  std::list<precisely_number> exp_click_revenues;
  exp_click_revenues.push_back(
    precisely_number(fetch_float("CLICKREV4_1"), 0.001));
  exp_click_revenues.push_back(
    precisely_number(fetch_float("CLICKREV4_2"), 0.001));

  std::list<precisely_number>
    exp_click_revenues_reverse(exp_click_revenues);
  exp_click_revenues_reverse.reverse();

  std::list<precisely_number> exp_imp_revenues;
  exp_imp_revenues.push_back(
    precisely_number(fetch_float("IMPREV4_1"), 0.001));
  exp_imp_revenues.push_back(
    precisely_number(fetch_float("IMPREV4_2"), 0.001));

  std::list<precisely_number>
    exp_imp_revenues_reverse(exp_imp_revenues);
  exp_imp_revenues_reverse.reverse();
 
  FAIL_CONTEXT(
    AutoTest::or_checker(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)),
      AutoTest::sequence_checker(
        exp_ccids_reverse,
        SelectedCreativesCCID(client))).check(),
    "ccids check");

   FAIL_CONTEXT(
     AutoTest::or_checker(
       AutoTest::sequence_checker(
         exp_click_revenues,
         SelectedCreativesActualCPC(client)),
       AutoTest::sequence_checker(
         exp_click_revenues_reverse,
         SelectedCreativesActualCPC(client))).check(),
     "click revenue check");

   FAIL_CONTEXT(
     AutoTest::or_checker(
       AutoTest::sequence_checker(
         exp_imp_revenues,
         SelectedCreativesImpRevenue(client)),
     AutoTest::sequence_checker(
       exp_imp_revenues_reverse,
       SelectedCreativesImpRevenue(client))).check(),
     "imp revenue check");
}

void
TextChannelAndKeywordCreativesCompetition::scenario5()
{
  add_descr_phrase("Actual CPC persistence for Text(C) CCGs "
                   "(Text(C) CCG ecpm < ccg keyword ecpm).");
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw =
    fetch_string("TEXTKEYWORD1c") + "," +
    fetch_string("CHANNELKEYWORD4");
  client.process_request(request);
  client.repeat_request ();

  std::string exp_ccids[] = {
    fetch_string("TEXTCC1"),
    fetch_string("CHANNELCC4"),
  };

  precisely_number exp_click_revenues[] = {
    precisely_number(fetch_float("CLICKREV5_1"), 0.001),
    precisely_number(fetch_float("CLICKREV5_2"), 0.001)
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "ccids check");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_click_revenues,
      SelectedCreativesActualCPC(client)).check(),
    "click revenue check");
}

void
TextChannelAndKeywordCreativesCompetition::scenario6()
{
  add_descr_phrase("Actual CPC persistence for Text(C) CCGs "
                   "(Text(C) CCG ecpm > ccg keyword ecpm).");
  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("TAG");
  request.referer_kw =
    fetch_string("TEXTKEYWORD1c") + "," +
    fetch_string("CHANNELKEYWORD5");
  client.process_request(request);
  client.repeat_request ();

  std::string exp_ccids[] = {
    fetch_string("CHANNELCC5"),
    fetch_string("TEXTCC1")
  };

  precisely_number exp_click_revenues[] = {
    precisely_number(fetch_float("CLICKREV6_1"), 0.001),
    precisely_number(fetch_float("CLICKREV6_2"), 0.001)
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client)).check(),
    "ccids check");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_click_revenues,
      SelectedCreativesActualCPC(client)).check(),
    "click revenue check");
}
