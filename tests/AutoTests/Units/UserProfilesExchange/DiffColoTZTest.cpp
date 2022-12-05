
#include "DiffColoTZTest.hpp"
#include "UserProfilesExchangeCommon.hpp"
 
REFLECT_UNIT(DiffColoTZTest) (
  "UserProfilesExchange",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::UserProfilesExchange::CheckWaitHistoryChannel CheckWaitHistoryChannel;

  enum TestTimeCases {
      // Same current day in both test colocations
      TTC_SAME_DAY,
      // Different days in test colocations (current fo one, nex for over)
      TTC_DIFF_DAY,
      // Same next day in both test colocations
      TTC_NEXT_DAY    
  };

  Generics::ExtendedTime get_test_date(TestTimeCases tcase,
                                       Generics::Time base_date,
                                       double tzofset)
  {
    double ofset = tzofset > 0? tzofset: (-1)*tzofset;
    int hour_ofset = tcase == TTC_DIFF_DAY? 1: -1;
    int hour = 24 - int(ofset) + hour_ofset;
    Generics::ExtendedTime result =
        tcase == TTC_NEXT_DAY? (base_date+24*60*60).get_gm_time():
          base_date.get_gm_time();
    result.tm_hour = hour;
    return result;
  }
}

bool 
DiffColoTZTest::run_test()
{
  tz_ofset = fetch_float("TZOfset");
  colo_req_timeout = fetch_int("COLO_EXCHANGE_TIMEOUT") + 1;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      tz_ofset != 0.0),
    "GMT isn't valid timezone for Colo#2");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      tz_ofset <= 24.0 && tz_ofset >= -24.0),
    "Invalid timezone ofset for Colo#2");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE1, STE_FRONTEND)),
    "Remote#1.AdFrontend must set in the XML configuration file");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE2, STE_FRONTEND)),
    "Remote#1.AdFrontend must set in the XML configuration file");
  if (tz_ofset > 0)
  {
    remote1 =
      get_config().get_service(CTE_REMOTE1, STE_FRONTEND).address;
    remote2 =
      get_config().get_service(CTE_REMOTE2, STE_FRONTEND).address;
    colo1_id = fetch_string("Colo/1");
    colo2_id = fetch_string("Colo/2");
  }
  else
  {
    remote1 =
      get_config().get_service(CTE_REMOTE2, STE_FRONTEND).address;
    remote2 =
      get_config().get_service(CTE_REMOTE2, STE_FRONTEND).address;
    colo1_id = fetch_string("Colo/2");
    colo2_id = fetch_string("Colo/1");
  }
  today = Generics::Time::get_time_of_day();
  local_day_switch();
  gmt_day_switch();
  return true;
}

// Based on Test 5.1. Date change in the first time zone
// Remote#2.timezone offset > Remote#1.timezone offset
// HT - 2 visits, 2 days (history+today)
// S - 1 visits, 8 hours (session)
// 1. Remote#2 create user
// 2. 1st HT (also match S) -> Remote#2
// 3. 2nd HT next day in Remote#2 -> Remote#1
// 5. Wait S appear in Remote#1 history
// 6. Check that HT present in history
void
DiffColoTZTest::local_day_switch()
{
  add_descr_phrase("Test 5.1. Different days in test colocations");

  AdClient client(AdClient::create_user(this, AutoTest::UF_FRONTEND_MINOR));

  NSLookupRequest request;
  request.referer_kw = fetch_string("KeywordS") + "," +
      fetch_string("KeywordHT1");
  request.debug_time = get_test_date(TTC_SAME_DAY, today, tz_ofset);
  client.process_request(request);
  {
    std::string expected[] = { fetch_string("BP/S"),
                               fetch_string("BP/HT1") };

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        colo2_id,
        client.debug_info.colo_id).check(),
      "must receive Colo#2");

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "must have expected channel in trigger_channels");
    
  }
  client.change_base_url(remote1.c_str());
  request.referer_kw = fetch_string("KeywordHT1");
  request.debug_time = get_test_date(TTC_DIFF_DAY, today, tz_ofset);;
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("BP/HT1"),
      client.debug_info.trigger_channels).check(),
    "must have expected channel in trigger_channels");

  request.referer_kw.clear();
  
  FAIL_CONTEXT(
    CheckWaitHistoryChannel(
      client,
      fetch_int("Channel/S"),
      colo1_id.c_str(),
      colo_req_timeout,
      request).check(),
    "Check session channel in adjacent colo history");


  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("Channel/HT1"),
      client.debug_info.history_channels).check(),
    "must have expected channel in trigger_channels");
}

// Based on Test 5.2. Date change in both time zones
// Remote#2.timezone offset > Remote#1.timezone offset
// HT - 3 visits, 2 days (history+today)
// C - 1 visits, 3 days (history+today)
// 1. Remote#2 create user
// 2. 1st HT (also match C) -> Remote#2
// 3. 2nd HT next day in Remote#2 -> Remote#2
// 4. 3d HT next day in all clolocations -> Remote#1
// 5. Wait C appear in Remote#1 history
// 6. Check that HT isn't present in history
// 7. 4th HT next day in all clolocations -> Remote#1 (HT appearance)
void
DiffColoTZTest::gmt_day_switch()
{
  add_descr_phrase("Test 5.2. Switch day in test colocations");
  
  AdClient client(AdClient::create_user(this, AutoTest::UF_FRONTEND_MINOR));

  NSLookupRequest request;
  request.referer_kw = fetch_string("KeywordC") + "," +
      fetch_string("KeywordHT2");
  request.debug_time = get_test_date(TTC_SAME_DAY, today, tz_ofset);
  client.process_request(request);
  {
    std::string expected[] = { fetch_string("BP/C"),
                               fetch_string("BP/HT2") };

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        colo2_id,
        client.debug_info.colo_id).check(),
      "must receive Colo#2");
    
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "must have expected channel in trigger_channels");
  }

  request.referer_kw = fetch_string("KeywordHT2");
  request.debug_time = get_test_date(TTC_DIFF_DAY, today, tz_ofset);
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("BP/HT2"),
      client.debug_info.trigger_channels).check(),
    "must have expected channel in trigger_channels");

  client.change_base_url(remote1.c_str());
  request.debug_time = get_test_date(TTC_NEXT_DAY, today, tz_ofset);

  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("BP/HT2"),
      client.debug_info.trigger_channels).check(),
    "must have expected channel in trigger_channels");

  request.referer_kw.clear();
  
  FAIL_CONTEXT(
    CheckWaitHistoryChannel(
      client,
      fetch_int("Channel/C"),
      colo1_id.c_str(),
      colo_req_timeout,
      request).check(),
    "Check session channel in adjacent colo history");
  
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("Channel/HT2"),
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "must have expected channel in trigger_channels");

  {
    request.referer_kw = fetch_string("KeywordHT2");
    client.process_request(request);
    
    std::string expected[] = { fetch_string("Channel/C"),
                               fetch_string("Channel/HT2") };
    
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        client.debug_info.history_channels,
        AutoTest::SCE_ENTRY).check(),
      "must have expected channel in history");
  }
  
}

