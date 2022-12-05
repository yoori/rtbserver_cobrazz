#include "RefererMatchingTest.hpp"
#include <String/InterConvertion.hpp>

REFLECT_UNIT(RefererMatchingTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace {

  // TODO. Think about more easy initialization
  struct RefererToChannels {
    const char* referer;
    bool channel1_appearence;
    bool channel2_appearence;
    bool channel3_appearence;
    bool channel4_appearence;
  };

  typedef bool RefererToChannels::* ChannelAppearence;

  struct Channel {
    const char* name;
    ChannelAppearence appearance;
  };

  const Channel channels[] = {
    { "RefererMatchingTest/01", &RefererToChannels::channel1_appearence},
    { "RefererMatchingTest/02", &RefererToChannels::channel2_appearence},
    { "RefererMatchingTest/03", &RefererToChannels::channel3_appearence},
    { "RefererMatchingTest/04", &RefererToChannels::channel4_appearence}
  };

  // Table. referer -> channel appearence
  const RefererToChannels appearances[] = {
    // only channel1 must appear
    // trigger list1=http://dev.ocslab.com/services/
    {"http://dev.ocslab.com:80/services/", true, false, false, false},
    {"http://dev.ocslab.com:80/services/nslookup", true, false, false, false},
    {"http://dev.ocslab.com:80/services/nslookup?site-id=100&app=PS", true, false, false, false},

    // channel2 must appear
    // trigger list2=http://andrey.gusev.com:80/
    {"http://andrey.gusev.com:80", false, true, false, false},
    {"http://andrey.gusev.com:80/", false, true, false, false},
    {"http://andrey.gusev.com:80?", false, true, false, false},
    {"http://andrey.gusev.com:80/?", false, true, false, false},
    {"http://andrey.gusev.com:80/good/specialist?", false, true, false, false},
    {"http://andrey.gusev.com:80?wants=1&cool=1", false, true, false, false},
    {"http://andrey.gusev.com:80/?wants=1&cool=1", false, true, false, false},

    // channel2 & channel3 must appear
    // trigger list2=http://andrey.gusev.com:80/
    // trigger list3=http://andrey.gusev.com:80/good/specialist/
    {"http://andrey.gusev.com:80/good/specialist/", false, true, true, false},
    {"http://andrey.gusev.com:80/good/specialist/?", false, true, true, false},
    {"http://andrey.gusev.com:80/good/specialist/?wants=1&cool=1", false, true, true, false },

    // channel2 & channel4 appear
    // trigger list2=http://andrey.gusev.com:80/
    // trigger list4=http://andrey.gusev.com:80/good/specialist?wants=1&cool=1
    {"http://andrey.gusev.com:80/good/specialist?wants=1&cool=1", false, true, false, true},

    // invalid port
    // channels isn't appear
    {"http://dev.ocslab.com:12345/services/nslookup", false, false, false, false},
    {"http://andrey.gusev.com:12345/good/specialist/?", false, false, false, false},
    {"http://andrey.gusev.com:12345/good/specialist?wants=1&cool=1", false, false, false, false}
  };

  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::DebugInfoList    DebugInfoList;
  typedef AutoTest::AdClient AdClient;
  
}

bool 
RefererMatchingTest::run_test()
{

  add_descr_phrase("Initialization");
  AdClient client(AdClient::create_user(this));
  NSLookupRequest request;
  for (unsigned int idx = 0;
       idx < sizeof(appearances) / sizeof(*appearances);
       idx++)
  
  {
      
    DebugInfoList in_channels;
    DebugInfoList out_channels;
    Stream::Stack<128> log_msg;
    log_msg << "Test reaction on referer " << appearances[idx].referer;
    add_descr_phrase(log_msg.str());
    client.process_request(request.referer(appearances[idx].referer));
    for (unsigned int channel_idx = 0;
         channel_idx < sizeof(channels) / sizeof(*channels);
         channel_idx++)
    {
          
      if (appearances[idx].*(channels[channel_idx].appearance))
      {
        // Channel must appear
        in_channels.push_back(fetch_string(channels[channel_idx].name));
      }
      else
      {
        // Channel must disappear
        out_channels.push_back(fetch_string(channels[channel_idx].name));
      }
    }
        
    Stream::Stack<128> in_msg;
    in_msg << "must got trigger_channels for " << appearances[idx].referer;

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        in_channels,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      in_msg.str().str());
      
    Stream::Stack<128> out_msg;
    in_msg << "must not got trigger_channels for " << appearances[idx].referer;

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        out_channels,
        client.debug_info.trigger_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      out_msg.str().str());
  }
  return true;
}
 
