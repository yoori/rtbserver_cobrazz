
#include "TagsAndCreativesSizeMatching.hpp"

REFLECT_UNIT(TagsAndCreativesSizeMatching) (
  "CreativeSelection",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::OrChecker OrChecker;
  typedef AutoTest::CountChecker CountChecker;
  typedef AutoTest::OpenRTBRequest OpenRTBRequest;
  typedef AutoTest::OpenRTBResponseChecker OpenRTBResponseChecker;

  struct DisplayVsTextCase
  {
    const char* keyword;
    const char* tag;
    const char* cc;
  };

  struct RTBCase
  {
    size_t count;
    const char* src;
    const char* sizes;
    const char* ccs;
    bool nurl;
  };
}

void TagsAndCreativesSizeMatching::same_type_diff_sizes()
{
  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("SAMETYPE/KWD")).
      tid(fetch_int("SAMETYPE/Tag")));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check CC");  
}


void TagsAndCreativesSizeMatching::actual_size()
{
  AdClient client(AdClient::create_user(this));

  const size_t requests = 50;

  CountChecker counter(2, requests);
  
  for (size_t i = 0; i < requests; ++i)
  {
    client.process_request(
      NSLookupRequest().
        referer_kw(fetch_string("ACTUALSIZE/KWD")).
        tid(fetch_int("ACTUALSIZE/Tag")));

    FAIL_CONTEXT(
      AutoTest::and_checker(
        AutoTest::equal_checker(
          fetch_string("ACTUALSIZE/CC"),
          client.debug_info.ccid),
        AutoTest::or_count_checker(
          &counter,
          AutoTest::equal_checker(
            fetch_string("ACTUALSIZE/Size/1"),
            client.debug_info.creative_size),
          AutoTest::equal_checker(
            fetch_string("ACTUALSIZE/Size/2"),
            client.debug_info.creative_size))).check(),
      "Check#" + strof(i+1));
  }

  FAIL_CONTEXT(
    counter.check(),
    "Events count checker");

}

void TagsAndCreativesSizeMatching::diff_sizes()
{
  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("DIFFSIZES/KWD")).
      tid(fetch_int("DIFFSIZES/Tag")));

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      {
        fetch_string("DIFFSIZES/CC/2"),
        fetch_string("DIFFSIZES/CC/3")
      },
      SelectedCreativesCCID(client)).check(),
    "Check CCs");
}

void TagsAndCreativesSizeMatching::max_ads()
{

  AdClient client(AdClient::create_user(this));

  const size_t requests = 50;
  
  CountChecker counter(3, requests);
  
  for (size_t i = 0; i < requests; ++i)
  {
    client.process_request(
      NSLookupRequest().
        referer_kw(fetch_string("MAXADS/KWD")).
        tid(fetch_int("MAXADS/Tag")));

    FAIL_CONTEXT(
      AutoTest::or_count_checker(
        &counter,
        AutoTest::equal_checker(
          fetch_string("MAXADS/CC/1"),
          client.debug_info.ccid),
        AutoTest::equal_checker(
          fetch_string("MAXADS/CC/2"),
          client.debug_info.ccid),
        AutoTest::or_checker(
          AutoTest::sequence_checker(
            {
              fetch_string("MAXADS/CC/3"),
              fetch_string("MAXADS/CC/4")
            },
            SelectedCreativesCCID(client)),
          AutoTest::sequence_checker(
            {
              fetch_string("MAXADS/CC/4"),
              fetch_string("MAXADS/CC/3")
            },
            SelectedCreativesCCID(client)))).check(),
      "Check CC#" + strof(i+1));
  }

  FAIL_CONTEXT(
    counter.check(),
    "Events count checker");
}

void
TagsAndCreativesSizeMatching::zero_max_ads()
{
  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("ZEROMAXADS/KWD")).
      tid(fetch_int("ZEROMAXADS/Tag")));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("ZEROMAXADS/CC/2"),
      client.debug_info.ccid).check(),
    "Check CC");  
}

void
TagsAndCreativesSizeMatching::multi_creatives_ccg()
{
  AdClient client(AdClient::create_user(this));

  const size_t requests = 100;
  
  CountChecker counter(2, requests);
  
  for (size_t i = 0; i < requests; ++i)
  {
    client.process_request(
      NSLookupRequest().
        referer_kw(fetch_string("MULTICC/KWD")).
        tid(fetch_int("MULTICC/Tag")));

    FAIL_CONTEXT(
      AutoTest::or_count_checker(
        &counter,
        AutoTest::equal_checker(
          fetch_string("MULTICC/CC/1/2"),
          client.debug_info.ccid),
        AutoTest::equal_checker(
          fetch_string("MULTICC/CC/1/3"),
          client.debug_info.ccid)).check(),
      "Check#" + strof(i+1));
  }

  FAIL_CONTEXT(
    counter.check(),
    "Events count checker");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      counter.counts()[0] > 3 * counter.counts()[1]),
    "Frequency of the condition#1 should be "
    "higher tnen the frequency of the condition#2");
}

void
TagsAndCreativesSizeMatching::display_vs_text()
{
  const DisplayVsTextCase CASES[] =
  {
    {"DISPLAYTEXT/KWD/1", "DISPLAYTEXT/Tag/1", "DISPLAYTEXT/CC/Display/1" },
    {"DISPLAYTEXT/KWD/2", "DISPLAYTEXT/Tag/2", "DISPLAYTEXT/CC/Text/2" },
    {"DISPLAYTEXT/KWD/3", "DISPLAYTEXT/Tag/3", "DISPLAYTEXT/CC/Text/3" }
  };

  for (size_t i=0; i < countof(CASES); ++i)
  {

    AdClient client(AdClient::create_user(this));
    
    client.process_request(
      NSLookupRequest().
        referer_kw(fetch_string(CASES[i].keyword)).
      tid(fetch_int(CASES[i].tag)));

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string(CASES[i].cc),
        client.debug_info.ccid).check(),
      "Check CC#" + strof(i+1));
  }
}

void
TagsAndCreativesSizeMatching::size_type_level()
{
  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("SIZETYPE/KWD")).
      tid(fetch_int("SIZETYPE/Tag/1")));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      // https://jira.corp.foros.com/browse/TDOC-1636
      // appropriate test-plan expect no creative
      fetch_string("SIZETYPE/CC/1"),
      client.debug_info.ccid).check(),
    "Check CC#1");

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("SIZETYPE/KWD")).
      tid(fetch_int("SIZETYPE/Tag/2")));


  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      {
        fetch_string("SIZETYPE/CC/1"),
        fetch_string("SIZETYPE/CC/2")
      },
      SelectedCreativesCCID(client)).check(),
    "Check CC#2");
}

void
TagsAndCreativesSizeMatching::rtb()
{
  AdClient client(
    AdClient::create_nonoptin_user(this));

  const RTBCase CASES[] =
  {
    {5, "body", "RTB/Size/0,RTB/Size/1", "RTB/CC/1,RTB/CC/2", false },
    {5, "url", "RTB/Size/0,RTB/Size/1", "RTB/CC/1,RTB/CC/2", true }
  };

  for (size_t i = 0; i < countof(CASES); i++)
  {
    for (size_t j = 0; j < CASES[i].count; j++)
    {
      client.
        process_post(
          OpenRTBRequest().
            referer(fetch_string("RTB/SEARCH")).
            min_cpm_price(0).
            debug_ccg(fetch_string("RTB/CCG/1")).
            src(CASES[i].src).
            aid(fetch_string("RTB/ACCOUNT")).
            type(20));
      
      OpenRTBResponseChecker ad_checker(
        client,
        OpenRTBResponseChecker::Expected().
          adid(fetch_int("RTB/CREATIVE/1")));

      FAIL_CONTEXT(
        ad_checker.check(),
        "Check bid.adid");

      std::string body;

      if (!CASES[i].nurl)
      {
        body = ad_checker.bids().adm;
      }

      std::list<std::string> expected_sizes;
      fetch_objects(
        std::inserter(expected_sizes, expected_sizes.begin()),
        CASES[i].sizes);

      if (CASES[i].nurl)
      {
        OpenRTBResponseChecker cc_checker(
          client,
          OpenRTBResponseChecker::Expected().
             nurl(".*&ad=" + map_objects(CASES[i].ccs) + ".*"));
          
        FAIL_CONTEXT(
          cc_checker.check(),
          "Check bid.nurl.ccids");

        OrChecker tsid_checker;

        for (
          auto it = expected_sizes.begin(); it != expected_sizes.end(); ++it)
        {
          tsid_checker.or_if(
            OpenRTBResponseChecker(
              client,
              OpenRTBResponseChecker::Expected().
                nurl(".*&tsid=" + *it + "&.*")));
        }

       
        FAIL_CONTEXT(
          tsid_checker.check(),
          "Check bid.nurl.tsid");

        client.process_request(cc_checker.bids().nurl);

        body = client.req_response_data();
      }

      FAIL_CONTEXT(
        AutoTest::and_checker(
          equal_checker(
            AutoTest::ComparableRegExp(
              ".*\"CCID\":\"" + fetch_string("RTB/CC/1") + "\".*"),
            body),
          equal_checker(
            AutoTest::ComparableRegExp(
              ".*\"CCID\":\"" + fetch_string("RTB/CC/2") + "\".*"),
            body)).check(),
        "CCIDs check");

      FAIL_CONTEXT(
        AutoTest::or_checker(
          equal_checker(
            AutoTest::ComparableRegExp(
              ".*TAGSIZE\\s=\\s" + fetch_string("RTB/Size/0/Protocol") + "$"),
            body),
          equal_checker(
            AutoTest::ComparableRegExp(
              ".*TAGSIZE\\s=\\s" + fetch_string("RTB/Size/1/Protocol") + "$"),
            body)).check(),
        "TAGSIZEs check");
    }
  }
}

bool
TagsAndCreativesSizeMatching::run_test()
{

  AUTOTEST_CASE(
    same_type_diff_sizes(),
    "Same type and different sizes");
  
  AUTOTEST_CASE(
    actual_size(),
    "Actual tag size");

  AUTOTEST_CASE(
    diff_sizes(),
    "Creatives with different sizes");

  AUTOTEST_CASE(
    max_ads(),
    "Different max ads for creative sizes");

  AUTOTEST_CASE(
    zero_max_ads(),
    "Zero max text ads");

  AUTOTEST_CASE(
    multi_creatives_ccg(),
    "Several creatives in one CCG");

  AUTOTEST_CASE(
    display_vs_text(),
    "Display and Text Creative with same size");

  AUTOTEST_CASE(
    size_type_level(),
    "Size Type Level");

  AUTOTEST_CASE(
    rtb(),
    "RTB request");
  
  return true;
}

