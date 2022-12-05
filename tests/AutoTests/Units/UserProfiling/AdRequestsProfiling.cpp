
#include "Common.hpp"
#include "AdRequestsProfiling.hpp"

REFLECT_UNIT(AdRequestsProfiling) (
  "UserProfiling",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
}

bool
AdRequestsProfiling::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE1, STE_FRONTEND)),
    "Remote#1.AdFrontend need for this test");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE2, STE_FRONTEND)),
    "Remote#2.AdFrontend need for this test");

  NOSTOP_FAIL_CONTEXT(basic_case());

  NOSTOP_FAIL_CONTEXT(history_optimization());

  NOSTOP_FAIL_CONTEXT(visit_on_adrequest());

  NOSTOP_FAIL_CONTEXT(no_visit_on_adrequest());

  NOSTOP_FAIL_CONTEXT(full_text_mode());

  NOSTOP_FAIL_CONTEXT(merging());
   
  return true;
}

void AdRequestsProfiling::basic_case()
{
  std::string description("Basic test.");
  add_descr_phrase(description);
  {
    AdClient client(AdClient::create_user(this));

    NSLookupRequest match_request;
    match_request.tid = fetch_string("Tag");
    match_request.referer_kw = fetch_string("Keyword1");
    
    client.process_request(match_request);
    client.process_request(match_request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel1",
        client.debug_info.history_channels).check(),
      description +
        " Expected history (ad_request_profiling=true)#1");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("CC"),
        client.debug_info.ccid).check(),
      description +
        " Expected ccid (ad_request_profiling=true)#1");


    NSLookupRequest ad_request;
    ad_request.tid = fetch_string("Tag");
    ad_request.referer_kw = fetch_string("Keyword3");

    client.process_request(ad_request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel1,Channel3",
        client.debug_info.history_channels).check(),
      description +
        " Expected history (ad_request_profiling=true)#2");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("CC"),
        client.debug_info.ccid).check(),
      description +
        " Expected ccid (ad_request_profiling=true)#2");
  }

  {
    AdClient client(
      AdClient::create_user(
        this, AutoTest::UF_FRONTEND_MINOR));

    NSLookupRequest match_request;
    match_request.tid = fetch_string("Tag");
    match_request.referer = fetch_string("Ref1");

    client.process_request(match_request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel1",
        client.debug_info.history_channels).check(),
      description +
        " Expected history (ad_request_profiling=false)#1");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("CC"),
        client.debug_info.ccid).check(),
      description +
        " Expected ccid (ad_request_profiling=false)#1");

    NSLookupRequest ad_request;
    ad_request.tid = fetch_string("Tag");
    
    client.process_request(ad_request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this, "Channel1",
        client.debug_info.history_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      description +
        " Expected history (ad_request_profiling=false)#2");
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      description +
        " Expected ccid (ad_request_profiling=false)#2");
  }
}

void AdRequestsProfiling::history_optimization()
{
  std::string description("History optimization.");
  add_descr_phrase(description);
  AdClient client(
    AdClient::create_user(
      this,
      AutoTest::UF_FRONTEND_MINOR));

  AutoTest::Time base_time;

  client.process_request(
    NSLookupRequest().
    referer_kw(fetch_string("Keyword2")).
    debug_time(base_time - 24*60*60));
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "BP2P",
      client.debug_info.trigger_channels).check(),
    description +
      " Expected trigger channels");


  client.process_request(
    NSLookupRequest().
    tid(fetch_string("Tag")).
    debug_time(base_time));
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Channel2",
      client.debug_info.history_channels).check(),
    description +
      " Expected history");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC"),
      client.debug_info.ccid).check(),
    description +
      " Expected ccid");
}

void AdRequestsProfiling::visit_on_adrequest()
{
  std::string description(
    "Channel visit exists in user info and request."
    "Channel in profile not fully matched, channel "
    "in request fully matches.");
  add_descr_phrase(description);
  AdClient client(AdClient::create_user(this,
      AutoTest::UF_FRONTEND_MINOR));

  client.process_request(
    NSLookupRequest().
    referer_kw(fetch_string("Keyword1")));
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "BP1P",
      client.debug_info.trigger_channels).check(),
    description +
      " Expected trigger channels");

  client.process_request(
    NSLookupRequest().
    tid(fetch_string("Tag")));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    description +
      " Expected ccid#1");
  
  client.process_request(
    NSLookupRequest().
    tid(fetch_string("Tag")).
    referer(fetch_string("Ref1")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Channel1",
      client.debug_info.history_channels).check(),
    description +
      " Expected history");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC"),
      client.debug_info.ccid).check(),
      description +
        " Expected ccid#2");
}

void AdRequestsProfiling::no_visit_on_adrequest()
{
  std::string description(
    "Channel visit exists in user info and request."
    "Channel in profile fully matches, channel in "
    "request not fully matched.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this,
      AutoTest::UF_FRONTEND_MINOR));

  client.process_request(
    NSLookupRequest().
    referer(fetch_string("Ref2")));
  client.repeat_request();
  client.repeat_request();

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Channel2",
      client.debug_info.history_channels).check(),
    description +
      " Expected history#1");

  client.process_request(
    NSLookupRequest().
    tid(fetch_string("Tag")).
    referer_kw(fetch_string("Keyword2")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Channel2",
      client.debug_info.history_channels).check(),
    description +
      " Expected history#2");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC"),
      client.debug_info.ccid).check(),
    description +
      " Expected ccid");
}

void AdRequestsProfiling::full_text_mode()
{
  std::string description(
    "Full text mode with disabled Tag profiling.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this,
      AutoTest::UF_FRONTEND_MINOR));

  client.process_request(
    NSLookupRequest().
    referer_kw(fetch_string("Keyword1")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "BP1P",
      client.debug_info.trigger_channels).check(),
    description +
      " Expected trigger channels");
  
  std::ostringstream ft;
  ft << fetch_string("Keyword1") << std::endl <<
    fetch_string("Keyword2")  << std::endl <<
    fetch_string("Keyword3");

  client.process_request(
    NSLookupRequest().
    ft(ft.str()).
    tid(fetch_string("Tag")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Channel1",
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Expected history#1");

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Channel3",
      client.debug_info.history_channels).check(),
    description +
      " Expected history#2");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC"),
      client.debug_info.ccid).check(),
    description +
      " Expected ccid");
}

void AdRequestsProfiling::merging()
{
  std::string description(
    "Merging on tad requests with enabled Tag profiling.");
  add_descr_phrase(description);

  {

    TemporaryAdClient tclient(TemporaryAdClient::create_user(this));
    
    tclient.process_request(
      NSLookupRequest().
      referer_kw(fetch_string("Keyword4")));  
    
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "BP4P,BP5P",
        tclient.debug_info.trigger_channels).check(),
      description +
        " Expected trigger channels#1")
    
      AdClient client(AdClient::create_user(this));

      client.process_request(
        NSLookupRequest().
        referer_kw(fetch_string("Keyword5")).
        tid(fetch_string("Tag")));

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "BP4P",
          client.debug_info.trigger_channels).check(),
        description +
          " Expected trigger channels#2");

      client.merge(tclient);

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel4,Channel5",
          client.debug_info.history_channels).check(),
        description +
          " Expected history#1");
  }

  {
    TemporaryAdClient tclient(TemporaryAdClient::create_user(this));
    
    tclient.process_request(
      NSLookupRequest().
      referer_kw(fetch_string("Keyword4")));  
    
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "BP4P,BP5P",
        tclient.debug_info.trigger_channels).check(),
      description +
        " Expected trigger channels#3");
    
      AdClient client(AdClient::create_user(this));

      client.merge(
        tclient,
        NSLookupRequest().
        referer_kw(fetch_string("Keyword5")).
        tid(fetch_string("Tag")));

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel4,Channel5",
          client.debug_info.history_channels).check(),
        description +
          " Expected history#2");
  }
  
  
}

