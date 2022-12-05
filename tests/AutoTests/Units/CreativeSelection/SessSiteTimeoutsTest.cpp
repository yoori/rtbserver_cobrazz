
#include "SessSiteTimeoutsTest.hpp"

REFLECT_UNIT(SessSiteTimeoutsTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

struct SessSiteTimeoutsTest::Request
{
  unsigned long time_ofset;
  const char* referer_kw;
  const char* tid;
  const char* expected_ccid;
  const char* expected_tag;
  unsigned long effects;
  unsigned long repeat_count;
};

namespace
{     
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::SpecialEffectsChecker SpecialEffectsChecker;

  // Defined in AdServer config
  // (Frontend config - requestModule/session.timeout)
  // Make sure that constant value correspond to the AdServer config
  const unsigned int SESSION_TIMEOUT = 30;

  enum SpecialEffectsEnum
  {
    SEE_NO_ADV = 1,    // Send <NO TRACK KWD> and check NO TRACK effect
    SEE_NO_TRACK = 2,  // Send <NO ADV KWD> and check NO ADV effect
    SEE_ADV = 4,       // Check TRACK effect
    SEE_TRACK = 8,     // Check ADV effect
    SEE_NO_CHECKS = 16 // Skip all checks (include ccid & tag)
  };

  // Site with noads_timeout.
  const SessSiteTimeoutsTest::Request NOADS_TIMEOUT[] =
  {
    {
      0, "Keyword/NOADS", "Tag/NOADS",
      0, 0 , 0, 1
    },
    {
      29, "Keyword/NOADS", "Tag/NOADS",
      0, 0, 0, 1
    },
    {
      30, "Keyword/NOADS", "Tag/NOADS",
      "CC/NOADS", "Tag/NOADS", 0, 1
    },
    {
      70, "Keyword/NOADS", "Tag/NOADS",
      0, 0, 0, 1
    },
    {
      90, "Keyword/NOADS", "Tag/NOADS",
      0, 0, 0, 1
    },
    {
      105, "Keyword/NOADS", "Tag/NOADS",
      "CC/NOADS", "Tag/NOADS", 0, 1
    },
  };

  const SessSiteTimeoutsTest::Request NOADS_TIMEOUT_TEXT[] =
  {
    {
      0, "Keyword/Text", "Tag/Text",
      0, 0 , 0, 1
    },
    {
      20, "Keyword/Text", "Tag/Text",
      0, 0, 0, 1
    },
    {
      39, "Keyword/Text", "Tag/Text",
      0, 0, 0, 1
    },
    {
      40, "Keyword/Text", "Tag/Text",
      "CC/Text", "Tag/Text", 0, 1
    },
    {
      71, "Keyword/Text", "Tag/Text",
      0, 0, 0, 1
    },
    {
      90, "Keyword/Text", "Tag/Text",
      0, 0, 0, 1
    },
    {
      115, "Keyword/Text", "Tag/Text",
      "CC/Text", "Tag/Text", 0, 1
    },
  };

  const SessSiteTimeoutsTest::Request ZERO_TIMEOUT[] =
  {
    {
      0, "Keyword/ZEROTIMEOUT", "Tag/ZEROTIMEOUT",
      "CC/ZEROTIMEOUT", "Tag/ZEROTIMEOUT", 0, 1
    },
    {
      35, "Keyword/ZEROTIMEOUT", "Tag/ZEROTIMEOUT",
      "CC/ZEROTIMEOUT", "Tag/ZEROTIMEOUT", 0, 1
    },
    {
      40, "Keyword/ZEROTIMEOUT", "Tag/ZEROTIMEOUT",
      "CC/ZEROTIMEOUT", "Tag/ZEROTIMEOUT", 0, 1
    }
  };

  const SessSiteTimeoutsTest::Request SPECIAL_EFFECTS[] =
  {
    {
      0, "Keyword/NOADS", "Tag/NOADS",
      0, 0, SEE_NO_ADV | SEE_TRACK, 1
    },
    {
      20, "Keyword/NOADS", "Tag/NOADS",
      0, 0, SEE_ADV | SEE_NO_TRACK, 1
    },
    {
      40, "Keyword/NOADS", "Tag/NOADS",
      0, 0, SEE_NO_ADV | SEE_NO_TRACK, 1
    },
    {
      60, "Keyword/NOADS", "Tag/NOADS",
      "CC/NOADS", "Tag/NOADS", SEE_ADV | SEE_TRACK, 9
    }
  };

  const SessSiteTimeoutsTest::Request RON[] =
  {
    {
      0, 0, "Tag/RON",
      0, 0, 0, 1
    },
    { //less then session timeout (defined in AdServer configuration)
      10, 0, "Tag/RON",
      0, 0, 0, 1
    },
    { //less then session timeout (defined in AdServer configuration)
      20, 0, "Tag/RON",
      0, 0, 0, 1
    },
    {
      40, 0, "Tag/RON",
      "CC/RON", "Tag/RON", 0, 1
    },
    { //more then session timeout (defined in AdServer configuration)
      71, 0, "Tag/RON",
      0, 0, 0, 1
    }
  };
  

}

bool 
SessSiteTimeoutsTest::run_test()
{
  no_track = fetch_string("no_track_words").c_str();
  no_adv = fetch_string("no_adv_words").c_str();
  
  AUTOTEST_CASE(
    testcase(NOADS_TIMEOUT),
    "Site with noads_timeout.");

  AUTOTEST_CASE(
    testcase(NOADS_TIMEOUT_TEXT),
    "Site with noads_timeout TA.");

  AUTOTEST_CASE(
    testcase(ZERO_TIMEOUT),
    "Site without noads_timeout.");

  AUTOTEST_CASE(
    testcase(SPECIAL_EFFECTS),
    "No adv & No track.");

  AUTOTEST_CASE(
    testcase(RON),
    "RON case.");

  AUTOTEST_CASE(
    merge_users(),
    "Merge users.");

  return true;
}


template <size_t COUNT>
void SessSiteTimeoutsTest::testcase(
  const Request (&requests)[COUNT])
{
  AdClient client(AdClient::create_user(this));
  for (size_t i = 0; i <  COUNT; ++i)
  {
    NSLookupRequest request;
    std::string referer_kw;
    if (requests[i].referer_kw)
      referer_kw = fetch_string(requests[i].referer_kw);
    if (requests[i].effects & SEE_NO_ADV)
      referer_kw += "," + no_adv;
    if (requests[i].effects & SEE_NO_TRACK)
      referer_kw += "," + no_track;
    request.tid = fetch_string(requests[i].tid);
    request.referer_kw = referer_kw;
    request.debug_time = now + requests[i].time_ofset;
    for (unsigned long j=0; j < requests[i].repeat_count; ++j)
    {
      if (requests[i].effects & SEE_NO_CHECKS)
      {
        client.process_request(request);
        continue;
      }
      std::string exp_ccid =
        requests[i].expected_ccid?
          fetch_string(requests[i].expected_ccid): "0";
      std::string exp_tag =
        requests[i].expected_tag?
          fetch_string(requests[i].expected_tag): "0";

      SpecialEffectsChecker checker(
        client,
        request,
        exp_ccid,
        requests[i].effects);

      FAIL_CONTEXT(
        checker.check(),
        "Ccid check#" + strof(i));
    
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          exp_tag,
          checker.client().debug_info.tag_id).check(),
        "Tag check#" + strof(i));
    }
  }
}

void SessSiteTimeoutsTest::merge_users()
{
  AdClient persistent1(AdClient::create_user(this));
  AdClient persistent2(AdClient::create_user(this));
  AdClient persistent3(AdClient::create_user(this));
  AdClient persistent4(AdClient::create_user(this));

  // Initial persistent requests
  NSLookupRequest request;
  request.tid = fetch_string("Tag/MERGE");
  request.referer_kw = fetch_string("Keyword/MERGE");
  request.debug_time = now;
  persistent1.process_request(request);
  persistent2.process_request(request);

  // Temporary user creation  
  TemporaryAdClient temporary1(
    TemporaryAdClient::create_user(this));
  TemporaryAdClient temporary2(
    TemporaryAdClient::create_user(this));
  TemporaryAdClient temporary3(
    TemporaryAdClient::create_user(this));

  request.debug_time = now - SESSION_TIMEOUT -1;
  temporary1.process_request(
    NSLookupRequest().
      debug_time(now + SESSION_TIMEOUT - 1));
  temporary2.process_request(
    NSLookupRequest().
      debug_time(now + SESSION_TIMEOUT - 1));
  temporary3.process_request(
    NSLookupRequest().debug_time(now));
  persistent4.process_request(request);

  request.debug_time = now + SESSION_TIMEOUT + 1;
  persistent4.merge(temporary3, request);
  
  request.debug_time = now + 2*SESSION_TIMEOUT - 1;
  persistent1.merge(temporary1, request);
  persistent3.merge(temporary2, request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC/MERGE"),
      persistent1.debug_info.ccid).check(),
    "Ccid check (temporary#1 keep session opened)");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC/MERGE"),
      persistent3.debug_info.ccid).check(),
    "Ccid check (temporary#2 keep session opened)");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      persistent4.debug_info.ccid).check(),
    "Ccid check (noads_timeout works "
    "still work for user#4)");

  persistent2.process_request(request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      persistent2.debug_info.ccid).check(),
    "Ccid check (noads_timeout works "
    "still work for user#2)");
}
