
#include "ExactKeywordMatching.hpp"

REFLECT_UNIT(ExactKeywordMatching) (
  "TriggerMatching",
  AUTO_TEST_FAST);

namespace
{
  enum TestFlags
  {
    TF_CHECK_TRIGGERS = 1 // Check Debug-Info triggers fiels
  };
  
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  const ExactKeywordMatching::TestCase TEST_1[]=
  {
    {"SEARCH1", "Channel2S", 0},            // 1
    {"SEARCH2", 0, "Channel2S"},            // 2
    {"SEARCH3", "Channel1S", "Channel2S"},  // 3
    {"SEARCH4", "Channel1S", "Channel2S"},  // 4
    {"SEARCH5", "Channel1S", "Channel2S"},  // 5
    {"SEARCH6", 0, "Channel2S"},            // 6
    {"SEARCH7", 0, "Channel2S"}             // 7
  };

  const ExactKeywordMatching::TestCase TEST_2[]=
  {
    {"SEARCH8", "ChannelTrigger/2/4", "ChannelTrigger/2/5"}, // 8
    {"SEARCH9", "ChannelTrigger/2/5", "ChannelTrigger/2/4"}, // 9
    {"SEARCH10", "ChannelTrigger/2/7", "ChannelTrigger/2/6"} // 10
  };
}

bool
ExactKeywordMatching::run_test()
{
  AdClient client(AdClient::create_user(this));

  NOSTOP_FAIL_CONTEXT(
    test_group_(client, TEST_1));

  NOSTOP_FAIL_CONTEXT(
    test_group_(client, TEST_2, TF_CHECK_TRIGGERS));
  
  return true;
}

template<size_t Count>
void
ExactKeywordMatching::test_group_(
  AdClient& client,
  const TestCase(&tests)[Count],
  unsigned long flags)
{
  for (size_t i = 0; i < Count; ++i)
  {
    NOSTOP_FAIL_CONTEXT(
      test_case_(client, tests[i], flags));
  }
}

void
ExactKeywordMatching::test_case_(
  AdClient& client,
  const TestCase& test,
  unsigned long flags)
{
    ++case_idx_;
    client.process_request(
      NSLookupRequest().
        search(fetch_string(test.search)));

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
      "Expected trigger_channels#"  + strof(case_idx_));
    
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        test.unmatched,
        got_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      "Unexpected trigger_channels#" + strof(case_idx_));
}


