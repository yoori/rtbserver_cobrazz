
#include "ChannelInventoryEstimMergeUsers.hpp"
 
REFLECT_UNIT(ChannelInventoryEstimMergeUsers) (
  "Statistics",
  AUTO_TEST_SLOW
);


namespace {
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef ORM::ChannelInventoryEstimStats::Diffs Diffs;
}

bool 
ChannelInventoryEstimMergeUsers::run_test()
{

  base_time = Generics::Time::get_time_of_day();
  
  ORM::StatsArray<ORM::ChannelInventoryEstimStats, 9> stats;
  // Today
  //   H1, level=0.3
  stats[0].key().
    channel_id(fetch_int("AdvChannelH1")).
    colo_id(1).
    match_level(0.3).
    sdate(base_time);
   stats[0].description(
     "ChannelInventoryEstimStats.H#1 - "
     "level 0.3, today");
  //   H1, level=0.2
  stats[1].key().
    channel_id(fetch_int("AdvChannelH1")).
    colo_id(1).
    match_level(0.2).
    sdate(base_time);
  stats[1].description(
    "ChannelInventoryEstimStats.H#1 - "
    "level 0.2, today");
  //   HT1, level=0.1
  stats[2].key().
    channel_id(fetch_int("AdvChannelHT1")).
    colo_id(1).
    match_level(0.1).
    sdate(base_time);
  stats[2].description(
    "ChannelInventoryEstimStats.HT#1 - "
    "level 0.1, today");
  //   S1, level=0.2
  stats[3].key().
    channel_id(fetch_int("AdvChannelS1")).
    colo_id(1).
    match_level(0.2).
    sdate(base_time);
  stats[3].description(
    "ChannelInventoryEstimStats.S#1 - "
    "level 0.2, today");
  //   S2, level=2.0
  stats[4].key().
    channel_id(fetch_int("AdvChannelS2")).
    colo_id(1).
    match_level(2.0).
    sdate(base_time);
  stats[4].description(
    "ChannelInventoryEstimStats.S#2 - "
    "level 2.0, today");
  //   HT2, level=2.0
  stats[5].key().
    channel_id(fetch_int("AdvChannelHT2")).
    colo_id(1).
    match_level(2.0).
    sdate(base_time);
  stats[5].description(
    "ChannelInventoryEstimStats.HT#2 - "
    "level 2.0, today");
  // Tomorrow
  //   H1, level=0.3
  stats[6].key().
    channel_id(fetch_int("AdvChannelH1")).
    colo_id(1).
    match_level(0.3).
    sdate(base_time+24*60*60);
  stats[6].description(
    "ChannelInventoryEstimStats.H#1 - "
    "level 0.3, tomorrow");
  //   H1, level=0.4
  stats[7].key().
    channel_id(fetch_int("AdvChannelH1")).
    colo_id(1).
    match_level(0.4).
    sdate(base_time+24*60*60);
  stats[7].description(
    "ChannelInventoryEstimStats.H#1 - "
    "level 0.4, tomorrow");
  //   HT1, level=0.4
  stats[8].key().
    channel_id(fetch_int("AdvChannelHT1")).
    colo_id(1).
    match_level(0.4).
    sdate(base_time+24*60*60);
  stats[8].description(
    "ChannelInventoryEstimStats.HT#1 - "
    "level 0.4, tomorrow");
 
  stats.select(conn);

  // Test cases
  simple_merging_1();
  temp_user_lost_history();
  rotten_channel_on_merging();
  exceed_match_level_on_merging();

  // first check
  {
    const Diffs diff[9] =
    {
      // Today
      //   H1, level=0.3
      Diffs().
        users_regular(0).
        users_from_now(1),
      //   H1, level=0.2
      Diffs(0),
      //   HT1, level=0.1
      Diffs(1),
      //   S1, level=0.2
      Diffs(1),
      //   S2, level=2.0
      Diffs(1),
      //   HT2, level=2.0
      Diffs(1),
      // Tomorrow
      //   H1, level=0.3
      Diffs(1),
      //   H1, level=0.4
      Diffs(0),
      //   HT1, level=0.4
      Diffs(1)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn,
          diff,
          stats)).check(),
      "verification");

  }
  
  stats.select(conn);
  
  simple_merging_2();

  // second check
  {
    const Diffs diff[9] =
    {
      // Today
      //   H1, level=0.3
      Diffs(0),
      //   H1, level=0.2
      Diffs().
        users_regular(0).
        users_from_now(1),
      //   HT1, level=0.1
      Diffs(0),
      //   S1, level=0.2
      Diffs(0),
      //   S2, level=2.0
      Diffs(0),
      //   HT2, level=2.0
      Diffs(0),
      // Tomorrow
      //   H1, level=0.3
      Diffs().
        users_regular(1).
        users_from_now(0),
      //   H2, level=0.4
      Diffs().
        users_regular(0).
        users_from_now(1),
      //   HT1, level=0.4
      Diffs(0)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn, diff, stats)).check(),
                         "verification");

  }
 
  return true;
}

// Test 6. Merging simple (part #1)
void
ChannelInventoryEstimMergeUsers::simple_merging_1()
{
  add_descr_phrase("Simple merging 1");

  TemporaryAdClient user_temp(
    TemporaryAdClient::create_user(this));
  
  NSLookupRequest merge;
  merge.debug_time = base_time;

  NSLookupRequest request;
  request.referer_kw = fetch_string("KWH1");
  request.debug_time = base_time;

  user_temp.process_request(request);

  std::string expected[] = {
    fetch_string("AdvBPPH1")
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
  
  AdClient user(AdClient::create_user(this));
  
  user.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
  
  user.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
  
  user.merge(user_temp, merge);
  user.process_request(NSLookupRequest().
                       debug_time(base_time + 24*60*60));
}

// Test 6. Merging simple (part #2 - with history optimization)
void
ChannelInventoryEstimMergeUsers::simple_merging_2()
{
  add_descr_phrase("Simple merging 1 (with history optimization)");

  TemporaryAdClient user_temp(
    TemporaryAdClient::create_user(this));
  
  NSLookupRequest merge;
  merge.referer_kw = fetch_string("KWH1");
  merge.debug_time = base_time + 24*60*60;

  NSLookupRequest request;
  request.referer_kw = fetch_string("KWH1");
  request.debug_time = base_time;

  std::string expected[] = {
        fetch_string("AdvBPPH1")
  };

  user_temp.process_request(request);

  AdClient user(AdClient::create_user(this));
  
  user.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
  
  user.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
  
  user.merge(user_temp, merge);
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
}

// Test 7. Temporary user's history isn't considered for the previous day
void ChannelInventoryEstimMergeUsers::temp_user_lost_history()
{
  add_descr_phrase("Temporary user's history isn't "
                   "considered for the previous day");

  TemporaryAdClient user_temp(
    TemporaryAdClient::create_user(this));

  NSLookupRequest request;
  request.referer_kw = fetch_string("KWHT1");
  request.debug_time = base_time;

  std::string expected[] = {
        fetch_string("AdvBPPHT1")
    };

  user_temp.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");

  user_temp.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");

  request.debug_time = base_time+24*60*60;

  user_temp.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");

  AdClient user(AdClient::create_user(this));

  request.debug_time = base_time;
  
  user.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");

  user.merge(user_temp,
             NSLookupRequest().
             debug_time(base_time + 24*60*60));   
   
}

// Test 8. Records expiration on merging (session)
void ChannelInventoryEstimMergeUsers::rotten_channel_on_merging()
{
  add_descr_phrase("Records expiration on merging (session)");

  TemporaryAdClient user_temp(
    TemporaryAdClient::create_user(this));
  
  {
    NSLookupRequest request;
    request.referer_kw = fetch_string("KWS1");
    request.debug_time = base_time;
    
    std::string expected[] = {
        fetch_string("AdvBPPS1")
    };

    user_temp.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
  }

  {
    NSLookupRequest request;
    request.referer = fetch_string("URLS1");
    request.debug_time = base_time;
    
    std::string expected[] = {
        fetch_string("AdvBPUS1")
    };

    user_temp.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");

    user_temp.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
    
  }


  AdClient user(AdClient::create_user(this));

  user.merge(user_temp,
             NSLookupRequest().
             debug_time(base_time + 3*60*60));   
  
}

// Test 9-10. Match_level exceeding on the merging
void ChannelInventoryEstimMergeUsers::exceed_match_level_on_merging()
{
  add_descr_phrase("Match_level exceeding on the merging");

  TemporaryAdClient user_temp(
      TemporaryAdClient::create_user(this));
  
  NSLookupRequest request;
  request.debug_time = base_time;
  request.referer = fetch_string("URLS2");
  request.referer_kw = fetch_string("KWHT2");

  std::string expected[] = {
      fetch_string("AdvBPUS2"),
      fetch_string("AdvBPPHT2")
  };

  for (unsigned long i=0; i < 3; ++i)
  {
    user_temp.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      user_temp.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels#" + strof(i+1));    
  }

  AdClient user(AdClient::create_user(this));

  user.merge(user_temp, NSLookupRequest().debug_time(base_time));
}
