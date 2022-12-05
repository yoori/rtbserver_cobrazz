
#include "TempProfileMergingTest.hpp"
#include "UserProfilesExchangeCommon.hpp"
 
REFLECT_UNIT(TempProfileMergingTest) (
  "UserProfilesExchange",
  AUTO_TEST_SLOW
);


namespace
{
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::UserProfilesExchange::CheckWaitHistoryChannel CheckWaitHistoryChannel;
}
 
bool 
TempProfileMergingTest::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE1, STE_FRONTEND)),
    "Remote#1.AdFrontend must set in the XML configuration file");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE2, STE_FRONTEND)),
    "Remote#1.AdFrontend must set in the XML configuration file");

  colo_req_timeout = fetch_int("COLO_EXCHANGE_TIMEOUT") + 1;

  remote1 =
    get_config().get_service(CTE_REMOTE1, STE_FRONTEND).address;
  remote2 =
    get_config().get_service(CTE_REMOTE2, STE_FRONTEND).address;

  colo1_id = fetch_string("Colo/1");
  colo2_id = fetch_string("Colo/2");
 
  merge_on_colo_change();
  merge_before_get_profile();
  return true;
}

// Test 6.1. Merging with tuid on colo change request
// 1. Create temporary uid
// 2. Temporary user match S1, H1 -> Remote#1
// 3. Create Remote#2 User
// 4. User match S2, H2 -> Remote#2
// 5. Merge temporary and persistent user -> Remote#1
// 6. Wait for S2, H2 appear in Remote#1 history
void TempProfileMergingTest::merge_on_colo_change()
{
  add_descr_phrase("Test 6.1. Merging with tuid on "
                   "colo change request");

  // Create temporary user
  TemporaryAdClient
    tclient(TemporaryAdClient::create_user(this));
  {
    NSLookupRequest request;
    request.referer_kw = fetch_string("KeywordS1") + "," +
        fetch_string("KeywordHT1");
    tclient.process_request(request);

    std::string expected[] = {
      fetch_string("Channel/S1"),
      fetch_string("Channel/HT1")
    };


    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        tclient.debug_info.history_channels,
        AutoTest::SCE_ENTRY).check(),
      "history");
  }

  {
    AdClient client(AdClient::create_user(this, AutoTest::UF_FRONTEND_MINOR));

    NSLookupRequest request; 
    request.referer_kw = fetch_string("KeywordS2") + "," +
        fetch_string("KeywordHT2");
    
    client.process_request(request);

    {
      std::string expected[] = {
        fetch_string("Channel/S2"),
        fetch_string("Channel/HT2")
      };

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.history_channels,
          AutoTest::SCE_ENTRY).check(),
        "history");
      
    }

    client.change_base_url(remote1.c_str());

    client.merge(tclient);
    
    {
      std::string expected[] = {
        fetch_string("Channel/S1"),
        fetch_string("Channel/HT1")
      };

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.history_channels,
          AutoTest::SCE_ENTRY).check(),
        "history");
    }

    FAIL_CONTEXT(
      CheckWaitHistoryChannel(
        client,
        fetch_int("Channel/S2"),
        colo1_id.c_str(),
        colo_req_timeout).check(),
      "Check session channel in adjacent colo history");

    client.process_request(NSLookupRequest());

    {
      std::string expected[] = {
        fetch_string("Channel/S1"),
        fetch_string("Channel/HT1"),
        fetch_string("Channel/S2"),
        fetch_string("Channel/HT2")
      };

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.history_channels,
          AutoTest::SCE_ENTRY).check(),
        "history");

    }
  }
}

// Test 6.2. Merging with tuid before profile from second colo has been received
// 1. Create temporary uid
// 2. Temporary user match S1, H1 -> Remote#1
// 3. Create Remote#2 User
// 4. User match S2, H2 -> Remote#2
// 5. User send empty request for exhange -> Remote#1
// 5. Merge temporary and persistent user -> Remote#1
// 6. Wait for S2, H2 appear in Remote#1 history
void TempProfileMergingTest::merge_before_get_profile()
{
  add_descr_phrase("Test 6.2. Merging with tuid before "
                   "profile from second colo has been "
                   "received");

  // Create temporary user
  TemporaryAdClient
    tclient(TemporaryAdClient::create_user(this));

  {
    NSLookupRequest request;
    request.referer_kw = fetch_string("KeywordS1") + "," +
        fetch_string("KeywordHT1");
    tclient.process_request(request);

    std::string expected[] = {
      fetch_string("Channel/S1"),
      fetch_string("Channel/HT1")
    };

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        tclient.debug_info.history_channels,
        AutoTest::SCE_ENTRY).check(),
      "history");
  }

  {
    AdClient client(AdClient::create_user(this,AutoTest::UF_FRONTEND_MINOR));
    
    NSLookupRequest request; 
    request.referer_kw = fetch_string("KeywordS2") + "," +
        fetch_string("KeywordHT2");
    
    client.process_request(request);

    {
      std::string expected[] = {
        fetch_string("Channel/S2"),
        fetch_string("Channel/HT2")
      };

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.history_channels,
          AutoTest::SCE_ENTRY).check(),
        "history");
    }

    client.change_base_url(remote1.c_str());

    // Exchange request
    client.process_request(NSLookupRequest());
        
    client.merge(tclient);
    
    {
      std::string expected[] = {
        fetch_string("Channel/S1"),
        fetch_string("Channel/HT1")
      };

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.history_channels,
          AutoTest::SCE_ENTRY).check(),
        "history");
    }

    FAIL_CONTEXT(
      CheckWaitHistoryChannel(
        client,
        fetch_int("Channel/S2"),
        colo1_id.c_str(),
        colo_req_timeout).check(),
      "Check session channel in adjacent colo history");

    client.process_request(NSLookupRequest());

    {
      std::string expected[] = {
        fetch_string("Channel/S1"),
        fetch_string("Channel/HT1"),
        fetch_string("Channel/S2"),
        fetch_string("Channel/HT2")
      };

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.history_channels,
          AutoTest::SCE_ENTRY).check(),
        "history");

    }
  }
}

