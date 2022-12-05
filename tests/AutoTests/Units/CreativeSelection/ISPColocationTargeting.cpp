
#include "ISPColocationTargeting.hpp"

REFLECT_UNIT(ISPColocationTargeting) (
  "CreativeSelection",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
}


template <size_t Count>
void
ISPColocationTargeting::process_case(
  const std::string& description,
  const TestCase(&testcases)[Count])
{
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  for (size_t i = 0; i < Count; ++i)
  {

    NSLookupRequest request;

    request.referer_kw = 
      map_objects(testcases[i].referer_kw);
    request.tid = fetch_int("TAG");
    if (testcases[i].colo)
    {
      request.colo =
        fetch_int(testcases[i].colo);
    }
  
    client.process_request(request);

    if (testcases[i].expected_history)
    {
      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          testcases[i].expected_history,
          client.debug_info.history_channels).check(),
        description + " Check history#" + strof(i+1));
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          0,
          client.debug_info.history_channels.size()).check(),
        description +
          " Check history#" + strof(i+1));
    }
  
    std::list<std::string> exp_ccids;

    if (testcases[i].expected_ccs)
    {
      fetch_objects(
        std::inserter(exp_ccids, exp_ccids.begin()),
        testcases[i].expected_ccs);
    }

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      description +  " Check CCs#"  + strof(i+1));
  }
}


bool
ISPColocationTargeting::run_test()
{

  // Display CCG
  {
    const TestCase CASES[] =
    {
      { "COLO/1", "KWD/DISPLAY", "CHANNEL/DISPLAY", "CC/DISPLAY" },
      { "COLO/2", "KWD/DISPLAY", "CHANNEL/DISPLAY", 0 },
    };

    NOSTOP_FAIL_CONTEXT(process_case("Display CCG.", CASES));    
  }

  // Text CCG
  {
    const TestCase CASES[] =
    {
      {
        "COLO/1", "KWD/TEXT/1,KWD/TEXT/2",
        "CHANNEL/TEXT/1,CHANNEL/TEXT/2",
        "CC/TEXT/1,CC/TEXT/2"
      },
      {
        "COLO/3", "KWD/TEXT/1,KWD/TEXT/2",
        "CHANNEL/TEXT/1,CHANNEL/TEXT/2", 0
      }
    };

    NOSTOP_FAIL_CONTEXT(process_case("Text CCG.", CASES));    
  }

  // Several colos targeting
  {
    const TestCase CASES[] =
    {
      { "COLO/1", "KWD/SEVERALCOLOS", "CHANNEL/SEVERALCOLOS", "CC/SEVERALCOLOS1" },
      { "COLO/2", "KWD/SEVERALCOLOS", "CHANNEL/SEVERALCOLOS", "CC/SEVERALCOLOS1" },
      { "COLO/3", "KWD/SEVERALCOLOS", "CHANNEL/SEVERALCOLOS", "CC/SEVERALCOLOS2" }
    };

    NOSTOP_FAIL_CONTEXT(process_case("Several colos targeting.", CASES));    
  }

  // Deleted colo targeting
  {
    const TestCase CASES[] =
    {
      { "COLO/DELETED", "KWD/DELETEDCOLO", 0, 0 },
      { 0, "KWD/DELETEDCOLO", "CHANNEL/DELETEDCOLO", 0 }
    };

    NOSTOP_FAIL_CONTEXT(process_case("Deleted colo targeting.", CASES));    
  }
  
  return true;
}

