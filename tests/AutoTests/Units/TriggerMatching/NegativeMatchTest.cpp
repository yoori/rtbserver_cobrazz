
#include "NegativeMatchTest.hpp"

REFLECT_UNIT(NegativeMatchTest) (
  "TriggerMatching",
  AUTO_TEST_FAST | AUTO_TEST_SLOW);

namespace {
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  const NegativeMatchTest::TestCase TEST_CASES_1[] =
  {
    {
      "REF1", "KWD1",
      "Channel1_1,Channel2_1,Channel3_1,Channel4_1",
      0, "CC"
    },

    {
      0, "FT2",
      "Channel2_1",
      "Channel1_1,Channel1_2,Channel1_3", "CC"
    },

    {
      "REF3", "KWD3",
      "Channel2_1",
      "Channel1_1,Channel1_2,Channel1_3", "CC"
    },

    {
      "REF4", "KWD4",
      "Channel2_1,Channel2_2",
      "Channel1_1,Channel1_2,Channel1_3", "CC"
    },

    {
      "REF5", "KWD5",
      "Channel2_1,Channel2_2",
      "Channel1_1,Channel1_2,Channel1_3", "CC"
    },

    {
      0, "KWD6",
      "Channel2_1",
      "Channel1_1,Channel1_2,Channel1_3", "CC"
    }
  };

  const NegativeMatchTest::TestCase TEST_CASES_2[] =
  {
    {
      0, "FT7",
      0, "Channel3_1", 0
    },
    {
      "REF8", 0,
      0, "Channel4_1", 0
    },
    {
      0, "KWD9",
      0, "Channel5_1", 0
    },
  };

  const NegativeMatchTest::TestCase EXACT_TRIGGERS_1[] =
  {
    {
      "SEARCH10", "KWD10",
      0, "Channel6_1,Channel6_2", 0
    },
    {
      "SEARCH11", 0,
      "Channel6_2", 0, "CC"
    }
  };

  const NegativeMatchTest::TestCase EXACT_TRIGGERS_2[] =
  {
    {
      "SEARCH12", "KWD12",
      0, "Channel6_1,Channel6_2", 0
    },
    {
      "SEARCH13", "KWD13",
      "Channel6_1", 0, "CC"
    }
  };

  const NegativeMatchTest::TestCase SPLIT_1[] =
  {
    {
      "SEARCH14", "KWD14",
      0, "Channel7_1,Channel7_2,Channel7_3", 0
    },
    {
      "SEARCH15", "KWD15",
      0, "Channel7_1,Channel7_2,Channel7_3", 0
    },
    {
      "SEARCH17", "KWD15",
      0, "Channel7_1,Channel7_2,Channel7_3", 0
    },
    {
      "SEARCH16", 0,
      "Channel7_2,Channel7_3",
      "Channel7_1",
      "CC"
    }
  };

  const NegativeMatchTest::TestCase SPLIT_2[] =
  {
    {
      0, "KWD17",
      "Channel7_1",
      "Channel7_2,Channel7_3",
      "CC"
    }
  };

  const NegativeMatchTest::TestCase SPLIT_3[] =
  {
    {
      0, "KWD18",
      "Channel7_1",
      "Channel7_2,Channel7_3",
      "CC"
    }
  };

  
}

void NegativeMatchTest::set_up()
{

}

void NegativeMatchTest::pre_condition()
{
  add_descr_phrase("Select initial stats");

  DataElemObjectPtr trigger;
  while (next_list_item(trigger, "NegativeTriggersList"))
  {
    unsigned long channel_trigger_id;
    Stream::Parser istr(trigger->Value());
    istr >> channel_trigger_id;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !istr.bad() && !istr.fail()),
      "Can't fetch channel_trigger_id to int variable from LocalParams.xml");

    Stat stat(Stat::Key().channel_trigger_id(channel_trigger_id));
    stat.description(trigger->Name());
    stat.select(conn_);
    stats_.push_back(stat);

    // hits for negative trigers are not calculated
    diffs_.push_back(Diff().hits(0));
  }

  Stat stat(Stat::Key().channel_trigger_id(fetch_int("MarkerTrigger")));
  stat.description("marker trigger check");
  stat.select(conn_);
  stats_.push_back(stat);

  diffs_.push_back(Diff().hits(2));
}

bool 
NegativeMatchTest::run()
{

  AUTOTEST_CASE(
    test_group(
      "Simple case#1.",
      TEST_CASES_1),
    "Simple cases");
  
  AUTOTEST_CASE(
    test_group(
      "Simple case#2.",
      TEST_CASES_2),
    "Simple cases");

  AUTOTEST_CASE(
    test_group(
      "Exact triggers#1.",
      EXACT_TRIGGERS_1),
    "Exact triggers");
  
  AUTOTEST_CASE(
    test_group(
      "Exact triggers#2.",
      EXACT_TRIGGERS_2),
    "Exact triggers");
  
  AUTOTEST_CASE(
    test_group(
      "Split page & search#1.",
      SPLIT_1),
    "Separate page and search negative lists");
  
  AUTOTEST_CASE(
    test_group(
      "Split page & search#2.",
      SPLIT_2),
    "Separate page and search negative lists");

  AUTOTEST_CASE(
    test_group(
      "Split page & search#3.",
      SPLIT_3),
    "Separate page and search negative lists");

  return true;
}

void NegativeMatchTest::post_condition()
{
  add_descr_phrase("Check ChannelTriggerStats table");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_, diffs_, stats_)).check(),
    "ChannelTriggerStats check");
}

void NegativeMatchTest::tear_down()
{

}

template<size_t Count>
void
NegativeMatchTest::test_group(
  const std::string& description,
  const TestCase(&tests)[Count])
{
  add_descr_phrase(description);
  AdClient client(AdClient::create_user(this));
  for (size_t i = 0; i < Count; ++i)
  {
    NOSTOP_FAIL_CONTEXT(
      test_case(description + "#" + strof(i+1),
        client, tests[i]));
  }
}

void
NegativeMatchTest::test_case(
  const std::string& description,
  AdClient& client,
  const TestCase& test)
{
  NSLookupRequest request;
  if (!client.uses_profiling_cluster())
  {
    request.tid = fetch_string("Tag");
  }
  if (test.referer)
  {
    if (strstr(test.referer, "SEARCH"))
    {
      request.search = fetch_string(test.referer);
    }
    else
    {
      request.referer = fetch_string(test.referer);
    }
  }
  if (test.referer_kw)
  {
    if (strstr(test.referer_kw, "FT"))
    {
      request.ft = fetch_string(test.referer_kw);
    }
    else
    {
      request.referer_kw = fetch_string(test.referer_kw);
    }
  }
  client.process_request(request);
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this, test.matched,
      client.debug_info.trigger_channels).check(),
    description +
      " Expected trigger_channels");
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this, test.unmatched,
      client.debug_info.trigger_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Unexpected trigger_channels");

  if (!client.uses_profiling_cluster())
  {
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        test.ccid ? fetch_string(test.ccid) : "0",
        client.debug_info.ccid).check(),
      description +
        " Expected ccid");
  }
}



