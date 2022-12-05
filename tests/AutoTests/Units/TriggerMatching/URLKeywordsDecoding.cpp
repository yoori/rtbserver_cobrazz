
#include "URLKeywordsDecoding.hpp"

REFLECT_UNIT(URLKeywordsDecoding) (
  "TriggerMatching",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  struct TestData
  {
    std::string description;
    const char* referer;
    const char* expected;
    const char* unexpected;
    const char* triggers;
  };

  const TestData TEST[] =
  {
    {
      "Domain name with port.",
      "REF1", "Channel/2",
      0, "Trigger/2/1", 
    },
    {
      "All parts of URL.",
      "REF2",
      "Channel/2,Channel/3,"
      "Channel/4,Channel/5",
      "Channel/6,Channel/7",
      "Trigger/2/1,Trigger/3/1,"
      "Trigger/4/1,Trigger/5/1"
    },
    {
      "Invalid utf-8 after url decoding.",
      "REF3",
      "Channel/2,Channel/3,"
      "Channel/4,Channel/5",
      "Channel/6,Channel/7",
      "Trigger/2/1,Trigger/3/1,"
      "Trigger/4/1,Trigger/5/1"
    },
    {
      "IDN encoded cyrillic domain and percent encoded part of URL.",
      "REF4", "Channel/8,Channel/9", 0,
      "Trigger/8/1,Trigger/9/1"
    },
    {
      "IDN encoded hangul line.",
      "REF5", "Channel/1", 0,
      "Trigger/1/1"
    },
    {
      "Percent encdoded value.",
      "REF6", "Channel/10", 0,
      "Trigger/10/1"
    },
    {
      "Percent encoded values with comma.",
      "REF7", "Channel/10,Channel/11", 0,
      "Trigger/10/1,Trigger/11/1"
    },
    {
      "Search engine with decoding depth=2.",
      "REF8", "Channel/1", "Channel/12",
      "Trigger/1/2"
    }
  };
}

bool
URLKeywordsDecoding::run_test()
{
  AdClient client(AdClient::create_user(this));

  for (size_t i = 0; i < countof(TEST); ++i)
  {
    client.process_request(
      NSLookupRequest().
      referer(fetch_string(TEST[i].referer)));

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        TEST[i].expected,
        client.debug_info.trigger_channels).check(),
      TEST[i].description +
        " Expected trigger_channels check");


    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        TEST[i].triggers,
        client.debug_info.triggers.list()).check(),
      TEST[i].description +
        " Expected triggers check");

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        TEST[i].unexpected,
        client.debug_info.trigger_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      TEST[i].description +
        " Unexpected trigger_channels check");
  }
  
  return true;
}

