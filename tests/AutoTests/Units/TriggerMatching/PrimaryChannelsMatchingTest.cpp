
#include "PrimaryChannelsMatchingTest.hpp"
 
REFLECT_UNIT(PrimaryChannelsMatchingTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace
{
  enum TestFlags
  {
    TF_CHECK_TRIGGERS = 1 // Check Debug-Info triggers fiels
  };
  
  
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  

  const PrimaryChannelsMatchingTest::TestCase SIMPLE_CASES[] =
  {
    {
      "Search trigger type.",
      "SEARCH1", 0, "BP2", 0
    },
    {
      "Page trigger type.",
      0, "KW2", "BP1", 0
    },
    {
      "Multiple trigger types#1.",
      "SEARCH3", "KW3", 0, "BP3,BP4,BP5"
    },
    {
      "Multiple trigger types#2.",
      "SEARCH4", "KW4", "BP3,BP4,BP5", 0
    },
    {
      "Hard triggers matching on "
      "request phrases#1.",
      "KW5", "KW5",  "BP4,BP5", "BP3"
    },
    {
      "Hard triggers matching on "
      "request phrases#2.",
      "KW6", "KW6", "BP3,BP4,BP5", 0
    },
    {
      "Trigger type/request parameters match#1.",
      0, "KW7","BP3", "BP4,BP5"
    },
    {
      "Trigger type/request parameters match#2.",
      "SEARCH7", 0, "BP4,BP5", "BP3"
    },
    {
      "Trigger type/request parameters match#3.",
      "SEARCH8", "KW8", 0, "BP4,BP3,BP5"
    }
  };

  const PrimaryChannelsMatchingTest::TestCase SPLIT_PAGE[] =
  {
    {
      "Page&Search trigger type.",
      "SEARCH9", "KW9",
      "SearchTrigger/5/1,SearchTrigger/5/2,"
      "SearchTrigger/5/3,UrlKwdTrigger/5/1,"
      "UrlKwdTrigger/5/2,UrlKwdTrigger/5/3,"
      "PageTrigger/5/1,PageTrigger/5/2,"
      "PageTrigger/5/3", 0
    }
  };

  const PrimaryChannelsMatchingTest::TestCase FT_CASES[] =
  {
    {
      "Full text match#1.",
      "FT10", "KW10","BP3", "BP5"
    },
    {
      "Full text match#2.",
      "FT11", 0,"BP3", "BP5"
    },
    {
      "Full text match#3.",
      "FT12", 0, 0, "BP3,BP5"
    },
    {
      "Full text match#4.",
      "FT13", 0, "BP3", "BP5"
    }
  };

  const PrimaryChannelsMatchingTest::TestCase URL_KWD_CASES[] =
  {
    {
      "Url kwd match.",
      "REF14", 0, "BP6", 0,
      
    },
    {
      "Hard triggers matching on "
      "request phrases#3.",
      "REF15", 0, "BP5", 0
    }
  };
}
 
bool 
PrimaryChannelsMatchingTest::run_test()
{
  AUTOTEST_CASE(
    test_group(
      SIMPLE_CASES,
      &NSLookupRequest::search,
      &NSLookupRequest::referer_kw),
    "Simple cases");

 AUTOTEST_CASE(
    test_group(
      SPLIT_PAGE,
      &NSLookupRequest::search,
      &NSLookupRequest::referer_kw,
      TF_CHECK_TRIGGERS),
    "Split page & search");

  AUTOTEST_CASE(
    test_group(
      FT_CASES,
      &NSLookupRequest::ft,
      &NSLookupRequest::referer_kw),
    "Full text cases");

  AUTOTEST_CASE(
    test_group(
      URL_KWD_CASES,
      &NSLookupRequest::referer,
      &NSLookupRequest::referer_kw),
    "Url keyword case");
  
  return true;
}

template<size_t Count>
void
PrimaryChannelsMatchingTest::test_group(
  const TestCase(&tests)[Count],
  NSLookupRequest::Member member1,
  NSLookupRequest::Member member2,
  unsigned long flags)
{
  AdClient client(AdClient::create_user(this));
  for (size_t i = 0; i < Count; ++i)
  {
    NOSTOP_FAIL_CONTEXT(
      test_case(client, tests[i], member1, member2, flags));
  }
}

void
PrimaryChannelsMatchingTest::test_case(
  AdClient& client,
  const TestCase& test,
  NSLookupRequest::Member member1,
  NSLookupRequest::Member member2,
  unsigned long flags)
{
  add_descr_phrase(test.description);
  NSLookupRequest request;
  if (test.param1)
  {
    member1(request, fetch_string(test.param1));
  }
  if (test.param2)
  {
    member2(request, fetch_string(test.param2));
  }
  client.process_request(request);

  std::list<std::string> got_channels;
  if (flags & TF_CHECK_TRIGGERS)
  {
    got_channels = client.debug_info.triggers.list();
  }
  else
  {
    got_channels.assign(
      client.debug_info.trigger_channels.begin(),
      client.debug_info.trigger_channels.end());
  }
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      test.matched,
      got_channels).check(),
    test.description +
      " Expected trigger_channels");
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      test.unmatched,
      got_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    test.description +
      " Unexpected trigger_channels");

}


