
#include "KeywordsNormalizationTest.hpp"
 
REFLECT_UNIT(KeywordsNormalizationTest) (
  "TriggerMatching",
  AUTO_TEST_FAST | AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef std::set<unsigned long> IntSet;

  enum TestFlags
  {
    TF_USE_KN = 1
  };

  struct TestCase
  {
    const char* matched;
    const char* unmatched;
    unsigned long flags;
  };

  struct RequestParameter
  {
    std::string name;
    NSLookupRequest::Member member;
  };

  const RequestParameter PARAMS[] = 
  {
    { "KW", &NSLookupRequest::referer_kw },
    { "KWW", &NSLookupRequest::referer_kw },
    { "SEARCH", &NSLookupRequest::search },
    { "FT", &NSLookupRequest::ft }
  };

 
  const TestCase TEST_CASES[] =
  {
    { "Page1", "Search1", 0}, // 1
    { "Page1", "Search1", 0}, // 2
    { "Page1", "Search1", 0}, // 3
    { "Page1,Search1", 0, 0}, // 4
    { 0, "Page1,Search1", 0}, // 5
    { "Page1", "Search1", 0}, // 6
    { "Page1", "Search1", 0}, // 7
    { "Page1", "Search1", 0}, // 8
    { "Page1", "Search1", 0}, // 9
    { "Page1", "Search1", 0}, // 10
    { 0, "Page1,Search1", 0}, // 11
    { "Page1", "Search1", 0}, // 12
    { "Page1", "Search1", 0}, // 13
    { "Page1", "Search1", 0}, // 14
    { "Page1", "Search1", 0}, // 15
    { "Page1", "Search1", 0}, // 16
    { "Page1", "Search1", 0}, // 17
    { "Page1", "Search1", 0}, // 18
    { "Page1", "Search1", 0}, // 19
    { "Page1", "Search1", 0}, // 20
    { "Page1", "Search1", 0}, // 21
    { "Page1", "Search1", 0}, // 22
    { 0, "Page1,Search1", 0}, // 23
    { "Page1", "Search1", 0}, // 24

    //   ADSC-7051
    { "Search1", 0, 0 }, // 25
    { "Search1", 0, 0 }, // 26

    // 'kn' cases
    { "Page3", "Page1", TF_USE_KN}, // 27
    { "Page3", "Page1", TF_USE_KN}, // 28
    { "Search1,Search3", 0, TF_USE_KN}, // 29
    // 'kn' cases ADSC-7051
    { "Page3", "Page1", TF_USE_KN}, // 30

    // ADSC-6723
    { "Page1,Search1,Page2,Search2", 0, 0}    // 31
  };
 
}

void KeywordsNormalizationTest::set_up()
{}

void KeywordsNormalizationTest::pre_condition()
{
  add_descr_phrase("Select initial stats");

  DataElemObjectPtr trigger;
  while (next_list_item(trigger, "ChannelTriggerList"))
  {
    unsigned long channel_trigger_id;
    Stream::Parser istr(trigger->Value());
    istr >> channel_trigger_id;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !istr.bad() && !istr.fail()),
      "Can't fetch channel_trigger_id to int variable from LocalParams.xml");

    TriggerStat stat(TriggerStat::Key().
      channel_trigger_id(channel_trigger_id).
      trigger_type(trigger->Name()));
    stat.description(
      trigger->Value() + " on " + (trigger->Name() == "S" ? "search" : "page"));
    stat.select(conn_);
    trigger_stats_.push_back(stat);

    trigger_diffs_.push_back(TriggerDiff().hits(trigger->Description()));

    // Add unexp stats - page trigger can't be logged as search and inside out
    TriggerStat unexp_stat(TriggerStat::Key().
      channel_trigger_id(channel_trigger_id).
      trigger_type(trigger->Name() == "S" ? "P" : "S"));
    unexp_stat.description(
      trigger->Value() + " on " + (trigger->Name() == "P" ? "search" : "page"));
    unexp_stat.select(conn_);
    unexpected_trigger_stats_.push_back(unexp_stat);
  }

  std::map<unsigned long, ChannelDiff> channels_diff;

  for (size_t i = 0; i < sizeof(TEST_CASES)/sizeof(*TEST_CASES); ++i)
  {
    if (!TEST_CASES[i].matched)
    { continue; }

    IntSet channel_hits;

    String::StringManip::SplitComma tokenizer(String::SubString(TEST_CASES[i].matched));
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      String::StringManip::trim(token);
      std::string trigger_channel = fetch_string(token.str());
      unsigned long channel_id = ::atoi(trigger_channel.c_str());
      char trigger_type = *trigger_channel.rbegin();
      if (channels_diff.find(channel_id) == channels_diff.end())
      {
        channels_diff[channel_id].hits_kws(0);
        channels_diff[channel_id].hits_search_kws(0);
        channels_diff[channel_id].hits_urls(0);
        channels_diff[channel_id].hits(0);
      }
      if (trigger_type == 'P')
      {
        channels_diff[channel_id].hits_kws(
          channels_diff[channel_id].hits_kws() + 1);
      }
      else if (trigger_type == 'S')
      {
        channels_diff[channel_id].hits_search_kws(
          channels_diff[channel_id].hits_search_kws() + 1);
      }
      channel_hits.insert(channel_id);
    }

    for (IntSet::const_iterator it = channel_hits.begin();
         it != channel_hits.end();
         ++ it)
    {
      channels_diff[*it].hits(
          channels_diff[*it].hits() + 1);
    }
  }

  // Select ChannelInventoryStats
  DataElemObjectPtr channel;
  while (next_list_item(channel, "ChannelList"))
  {
    unsigned long channel_id;
    Stream::Parser istr(channel->Value());
    istr >> channel_id;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !istr.bad() && !istr.fail()),
      "Can't fetch channel_id to int variable from LocalParams.xml");

    ChannelStat stat(ChannelStat::Key().
      channel_id(channel_id));
    stat.description(channel->Name());
    stat.select(conn_);
    channel_stats_.push_back(stat);

    channel_diffs_.push_back(channels_diff[channel_id].hits_urls(0));
  }
}

bool 
KeywordsNormalizationTest::run()
{

  AdClient client(AdClient::create_user(this));
 
  for (unsigned int i = 0;
       i < sizeof(TEST_CASES) /
           sizeof(*TEST_CASES); ++i)
  {
    std::string description("Request " + strof(i+1));
    add_descr_phrase(description);

    // Make request
    NSLookupRequest request;

    for (size_t p = 0; p < countof(PARAMS); p++)
    {
      try
      {
        PARAMS[p].member(
          request,
          fetch_string(PARAMS[p].name + strof(i+1)));
          
      }
      catch (const InvalidArgument&)
      {
        // Error mean no keyword for this case, skip it
      }
    }
    
    request.kn = TEST_CASES[i].flags & TF_USE_KN;
    client.process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        TEST_CASES[i].matched,
        client.debug_info.trigger_channels).check(),
      description +
        ". Expected trigger_channels");
  
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        TEST_CASES[i].unmatched,
        client.debug_info.trigger_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      description +
        ". Unexpected trigger_channels");
  }

  return true;
}

void KeywordsNormalizationTest::post_condition()
{
  add_descr_phrase("Check generated statistic");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_, trigger_diffs_, trigger_stats_)).check(),
    "ChannelTriggerStats check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_, channel_diffs_, channel_stats_)).check(),
    "ChannelInventory check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_each_diff_checker(
        conn_, 0, unexpected_trigger_stats_)).check(),
    "ChannelTriggerStats unexpected check");
}

void KeywordsNormalizationTest::tear_down()
{

}
