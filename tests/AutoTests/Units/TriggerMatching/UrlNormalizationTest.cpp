
#include "UrlNormalizationTest.hpp"
 
REFLECT_UNIT(UrlNormalizationTest) (
  "TriggerMatching",
  AUTO_TEST_FAST | AUTO_TEST_SLOW
);

namespace {
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  
  struct TestCase
  {
    const char* referer;
    const char* expected_channels;
    const char* unexpected_channels;
  };

  const TestCase TEST_CASES[] = 
  {
    { "REF1", "Channel1,Channel2,Channel3,Channel4", 0 },
    { "REF2", 0, "Channel1" },
    { "REF3", "Channel1", 0 },
    { "REF4", "Channel1", 0 },
    { "REF5", 0, "Channel1" },
    { "REF6", 0, "Channel1" },
    { "REF7", "Channel1", 0 },
    { "REF8", "Channel1", 0 },
    { "REF9", "Channel1,Channel2,Channel3", 0 },
    { "REF10", "Channel1,Channel2,Channel3", 0 },
    { "REF11", "Channel1", 0 },
    { "REF12", "Channel1", 0 },    
    { "REF13", "Channel1", 0 },
    { "REF14", "Channel1", 0 },
    { "REF15", "Channel1", 0 },
    { "REF16", "Channel1", 0 },
    { "REF17", "Channel1", 0 },
    { "REF18", "Channel1", 0 },
    { "REF19", 0, "Channel1" }
  };
}

void UrlNormalizationTest::set_up() {}

void UrlNormalizationTest::pre_condition()
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
      trigger_type("U"));
    stat.description(trigger->Name());
    stat.select(conn_);
    trigger_stats_.push_back(stat);

    trigger_diffs_.push_back(TriggerDiff().hits(trigger->Description()));
  }

  std::map<std::string, ChannelDiff> channels_diff;

  for (size_t i = 0; i < countof(TEST_CASES); ++i)
  {
    if (!TEST_CASES[i].expected_channels)
    { continue; }

    String::StringManip::SplitComma tokenizer(
      String::SubString(TEST_CASES[i].expected_channels));
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      String::StringManip::trim(token);
      std::string channel_id = fetch_string(token.str());
      if (channels_diff.find(channel_id) == channels_diff.end())
      {
        channels_diff[channel_id].hits_kws(0);
        channels_diff[channel_id].hits_search_kws(0);
        channels_diff[channel_id].hits_urls(0);
        channels_diff[channel_id].hits(0);
      }
        channels_diff[channel_id].hits_urls(
          channels_diff[channel_id].hits_urls() + 1);
        channels_diff[channel_id].hits(
          channels_diff[channel_id].hits() + 1);
    }
  }

  DataElemObjectPtr channel;
  while (next_list_item(channel, "ChannelList"))
  {
    std::string channel_id(channel->Value());

    ChannelStat stat(ChannelStat::Key().
      channel_id(::atoi(channel_id.c_str())));
    stat.description(channel->Name());
    stat.select(conn_);
    channel_stats_.push_back(stat);

    channel_diffs_.push_back(
      channels_diff.find(channel_id) == channels_diff.end()
        ? ChannelDiff(0)
        : channels_diff[channel_id]);
  }
}
 
bool 
UrlNormalizationTest::run()
{
  AdClient client(AdClient::create_user(this));
  
  for (unsigned int i = 0;
       i < sizeof(TEST_CASES)/sizeof(*TEST_CASES);  ++i)
  {
    add_descr_phrase("Request " + strof(i+1));
    NSLookupRequest request;
    request.referer = fetch_string(TEST_CASES[i].referer);
    client.process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        TEST_CASES[i].expected_channels,
        client.debug_info.trigger_channels).check(),
      "Expected trigger_channels#" + strof(i+1));

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        TEST_CASES[i].unexpected_channels,
        client.debug_info.trigger_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      "Unexpected trigger_channels#" + strof(i+1));

  }

  return true;
}

void UrlNormalizationTest::post_condition()
{
  add_descr_phrase("Check ChannelTriggerStats table");
  
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
}

void UrlNormalizationTest::tear_down() {}

