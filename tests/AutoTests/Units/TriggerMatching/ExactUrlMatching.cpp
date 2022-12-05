
#include "ExactUrlMatching.hpp"
 
REFLECT_UNIT(ExactUrlMatching) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace {
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  const AutoTest::SequenceCheckerEnum REQUESTS[] =
  {
    AutoTest::SCE_ENTRY,     // 1
    AutoTest::SCE_ENTRY,     // 2
    AutoTest::SCE_ENTRY,     // 3
    AutoTest::SCE_ENTRY,     // 4
    AutoTest::SCE_ENTRY,     // 5
    AutoTest::SCE_ENTRY,     // 6
    AutoTest::SCE_ENTRY,     // 7
    AutoTest::SCE_NOT_ENTRY, // 8
    AutoTest::SCE_NOT_ENTRY, // 9
    AutoTest::SCE_ENTRY,     // 10
    AutoTest::SCE_NOT_ENTRY, // 11
    AutoTest::SCE_ENTRY,     // 12
    AutoTest::SCE_ENTRY,     // 13
    AutoTest::SCE_ENTRY,     // 14
    AutoTest::SCE_ENTRY,     // 15
    AutoTest::SCE_ENTRY      // 16
  };
}

 
bool 
ExactUrlMatching::run_test()
{
  AdClient client(AdClient::create_user(this));

  for (unsigned int i = 0; i < sizeof(REQUESTS) / sizeof(*REQUESTS); ++i)
  {
    add_descr_phrase(("Request " + strof(i+1)).c_str());
    std::string ref_name = "REF" + strof(i+1);
    NSLookupRequest request;
    request.referer = fetch_string(ref_name);
    client.process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel",
        client.debug_info.trigger_channels,
        REQUESTS[i]).check(),
      "Check trigger_channels#" +  strof(i+1));
    
  }
  return true;
}

