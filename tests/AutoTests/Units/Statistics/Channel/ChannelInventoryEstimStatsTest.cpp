
#include "ChannelInventoryEstimStatsTest.hpp"

REFLECT_UNIT(ChannelInventoryEstimStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace {
  
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef ORM::ChannelInventoryEstimStats::Diffs Diffs;
  typedef ORM::ChannelInventoryEstimStats Stat;
  typedef ORM::StatsList<Stat> Stats;

  const ChannelInventoryEstimStatsTest::UserRequestType requests[] =
  {
      // channel level regular from_now   
      //   #S1     0.2     0       +5
      {0, 0, 0, "KW0", "AdvBPP0"},
      {1, 0, 0, "KW0", "AdvBPP0"},
      {2, 0, 0, "KW0", "AdvBPP0"},
      {3, 0, 0, "KW0", "AdvBPP0"},

      //   S#2      1     +3       +3
      {4, "URL1", 0, 0, "AdvBPU1"},
      {5, "URL1", 0, 0, "AdvBPU1"},
      {6, "URL1", 0, 0, "AdvBPU1"},

      //   S#2     0.5     0       +2
      {7, 0, "KW1", 0, "AdvBPS1"},
      {8, 0, "KW1", 0, "AdvBPS1"},

      //   S#2     0.2     0       +1
      {9, 0, 0, "KW1", "AdvBPP1"},
      
      //   S#1     0.5     0       +4
      {10, 0, "KW0", 0, "AdvBPS0"},
      {11, 0, "KW0", 0, "AdvBPS0"},
      {12, 0, "KW0", 0, "AdvBPS0"},
      {13, 0, "KW0", 0, "AdvBPS0"},

      //   S#2    0.2    +5        +5
      {14, 0, 0, "KW1", "AdvBPP1"},
      {15, 0, 0, "KW1", "AdvBPP1"},
      {16, 0, 0, "KW1", "AdvBPP1"},
      {17, 0, 0, "KW1", "AdvBPP1"},
      {18, 0, 0, "KW1", "AdvBPP1"},

      // Mixed
      
      //  S#1     S#2     1      +1      +1
      //  S#2     S#2   0.2       0      +1
      {19, "URL0", 0, "KW1", "AdvBPU0,AdvBPP1"},
      //  S#2     S#1   0.5       0      +1
      //  S#1     S#1   0.2       0      +1
      {20, 0, "KW1", "KW0", "AdvBPS1,AdvBPP0"},
      // This request should increase row with maximum match level!
      //  S#2     S#2    1       +1      +1
      {21, "URL1", 0, "KW1", "AdvBPU1,AdvBPP1"},
      // This request should increase row with maximum match level!
      // S#1      S#2  0.5       0       +1 
      {22, 0, "KW0", "KW0", "AdvBPS0,AdvBPP0"},

      // History & history+today

      // H#1      1.0     0     +1
      // HT       0.5    +1     +1
      {23, "URLH1", 0, "KWHT", "AdvBPUH1,AdvBPPHT"},
      // H#2      1.0     0     +1
      {24, 0, 0, "KWH2", "AdvBPPH2"}
  };

  /**
   * @class TriggerChannelsCheck
   * @brief Check trigger channels on client (user)
   */
  class TriggerChannelsCheck : public AutoTest::Checker
  {
  public:
    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param test.
     * @param expected trigger channel names.
     */
    TriggerChannelsCheck(
      AutoTest::AdClient& user,
      BaseUnit* test,
      const char* exp_channels)
      : user_(user),
        test_(test),
        exp_channels_(exp_channels)
    {}

    /**
     * @brief Destructor.
     */
    virtual ~TriggerChannelsCheck() noexcept
    {}

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool = true) /*throw(eh::Exception)*/
    {
      std::list<std::string> exp_trigger_channels;
      String::StringManip::SplitComma tokenizer(exp_channels_);
      String::SubString token;
      while (tokenizer.get_token(token))
      {
        String::StringManip::trim(token);
        exp_trigger_channels.push_back(test_->fetch_string(token.str()));
      }
      
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          exp_trigger_channels,
          user_.debug_info.trigger_channels,
          AutoTest::SCE_ENTRY).check(),
        "trigger_channels");
      
      return true;
    }

  private:
    AutoTest::AdClient& user_;  // user
    BaseUnit* test_;            // test
    std::string exp_channels_;  // expected trigger channels names
  };
}
 
bool 
ChannelInventoryEstimStatsTest::run_test()
{
  users.initialize(this, USERS_COUNT);

  // To prevent sdate changing
  base_time = Generics::Time::get_time_of_day();
  if (base_time.get_gm_time().tm_hour >= 20)
  {
    base_time -= Generics::Time::ONE_HOUR * 2;
  }

  // Initialize stats for discover channels

  test_1st_request();
  test_2nd_request_70s_later();
  test_3d_request_130s_later();
  test_4th_request_tomorrow();
  test_5th_request_after_tomorrow();
  test_different_colo_();
 
  return true;
}

void
ChannelInventoryEstimStatsTest::test_1st_request()
{
  add_descr_phrase("1st request (initial)");

  ORM::StatsArray<ORM::ChannelInventoryEstimStats, 9> stats;

  // Today
  //   Session
  //     level = 0.2
  stats[0].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(0.2).
    sdate(base_time);
  stats[0].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 0.2");
  stats[1].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(0.2).
    sdate(base_time);
  stats[1].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 0.2");
  //     level = 0.5  
  stats[2].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(0.5).
    sdate(base_time);
  stats[2].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 0.5");
  stats[3].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(0.5).
    sdate(base_time);
  stats[3].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 0.5");
  //     level = 1.0
  stats[4].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time);
  stats[4].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 1.0");
  stats[5].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time);
  stats[5].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 1.0");
  //  History & History+today
  //     HT
  stats[6].key().
    channel_id(fetch_int("AdvChannelHT")).
    colo_id(1).
    match_level(0.5).
    sdate(base_time);
  stats[6].description(
    "ChannelInventoryEstimStats.History+today channel");
  //     H1 & H2
  stats[7].key().
    channel_id(fetch_int("AdvChannelH1")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time);
  stats[7].description(
    "ChannelInventoryEstimStats.History channel#1");
  stats[8].key().
    channel_id(fetch_int("AdvChannelH2")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time);
  stats[8].description(
    "ChannelInventoryEstimStats.History channel#2");

  stats.select(conn);

  for (unsigned int i = 0; i < countof(requests); ++i)
  {
    send_request(requests[i]);
    FAIL_CONTEXT(
      TriggerChannelsCheck(
        users[requests[i].user_index],
        this,
        requests[i].exp_trigger_channels).check(),
      "Check matching" + strof(i));
  }

  {
    const Diffs diff[9] =
    {
      // Today
      //   Session
      //     level = 0.2
      Diffs().
        users_regular(0).
        users_from_now(5),
      Diffs().
        users_regular(0).
        users_from_now(7),
      //     level = 0.5
      Diffs().
        users_regular(0).
        users_from_now(5),
      Diffs().
        users_regular(0).
        users_from_now(3),
      //     level = 1.0  
      Diffs(1),
      Diffs(4),
      //  History & History+today
      //     HT
      Diffs(1),
      //     H1 & H2
      Diffs().
        users_regular(0).
        users_from_now(1),
      Diffs().
        users_regular(0).
        users_from_now(1)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn, diff, stats)).check(),
      "initial");

  }
}

void
ChannelInventoryEstimStatsTest::test_2nd_request_70s_later()
{
  add_descr_phrase("2nd request (70 seconds later)");

  ORM::StatsArray<ORM::ChannelInventoryEstimStats, 6> stats;
  // Today
  //   Session
  //     level = 0.2
  stats[0].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(0.2).
    sdate(base_time);
  stats[0].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 0.2");
  stats[1].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(0.2).
    sdate(base_time);
  stats[1].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 0.2");
  //     level = 0.5  
  stats[2].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(0.5).
    sdate(base_time);
  stats[2].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 0.5");
  stats[3].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(0.5).
    sdate(base_time);
  stats[3].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 0.5");
  //     level = 1.0
  stats[4].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time);
  stats[4].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 1.0");
  stats[5].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time);
  stats[5].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 1.0");

  stats.select(conn);

  const UserRequestType requests_70[] =
  {
    // channel level regular from_now   
    //  S#1      0.2    +1       -1
    //  S#1      0.5     0       +1
    {1, 0, 0, "KW0", "AdvBPP0"},  // move user#1 from 0.2 to 0.5
    //  S#1      0.2     0       +2
    {23, 0, 0, "KW0", "AdvBPP0"}, // add user#23 to 0.2 
    {24, 0, 0, "KW0", "AdvBPP0"}, // add user#24 to 0.2

    //  S#1      0.2     0       -1
    //  S#1        1    +1       +1
    {2, "URL0", 0, 0, "AdvBPU0"}, // move user#2 to 1

    //  S#2      0.2    +1       -1
    //  S#2      0.5     0       +1
    {16, 0, 0, "KW1", "AdvBPP1"}, // move user#16 to 0.5

    //  S#2      0.2     0       -1
    //  S#2        1    +1       +1
    {17, "URL1", 0, 0, "AdvBPU1"}, // move user#17 to 0.5

    //  S#1      0.5     0       -2
    //  S#1        1     0       +2
    {10, 0, "KW0", 0, "AdvBPS0"}, // move user#10 to 1
    {11, 0, "KW0", 0, "AdvBPS0"}  // move user#10 to 1
    
  };
  
  for (unsigned int i=0; i < countof(requests_70); ++i)
  {
    send_request(requests_70[i], 70);
    FAIL_CONTEXT(
      TriggerChannelsCheck(
        users[requests_70[i].user_index],
        this,
        requests_70[i].exp_trigger_channels).check(),
      "Check matching#" + strof(i));
  }
    
  {
    const Diffs diff[6] =
    {
      // Today
      //   Session
      //     level = 0.2
      Diffs().
        users_regular(1).
        users_from_now(0),
      Diffs().
        users_regular(1).
        users_from_now(-2),
      //     level = 0.5
      Diffs().
        users_regular(0).
        users_from_now(-1),
      Diffs().
        users_regular(0).
        users_from_now(1),
      //     level = 1.0
      Diffs().
        users_regular(1).
        users_from_now(3),
      Diffs(1)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn, diff, stats)).check(),
      "70 seconds later");

  }
}

void
ChannelInventoryEstimStatsTest::test_3d_request_130s_later()
{
  add_descr_phrase("3d request (130 seconds later)");

  ORM::StatsArray<ORM::ChannelInventoryEstimStats, 6> stats;
  // Today
  //   Session
  //     level = 0.2
  stats[0].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(0.2).
    sdate(base_time);
  stats[0].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 0.2");
  stats[1].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(0.2).
    sdate(base_time);
  stats[1].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 0.2");
  //     level = 0.5  
  stats[2].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(0.5).
    sdate(base_time);
  stats[2].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 0.5");
  stats[3].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(0.5).
    sdate(base_time);
  stats[3].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 0.5");
  //     level = 1.0
  stats[4].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time);
  stats[4].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 1.0");
  stats[5].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time);
  stats[5].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 1.0");
  stats.select(conn);
  
  const UserRequestType requests_130[] =
   {
    // channel level regular from_now   
    //    S#2    0.5    +1      -1
    //    S#2      1     0      +1
    {7, 0, "KW1", 0, "AdvBPS1"},

    //    S#1    0.5    +1     -1
    //    S#1    0.5     0     +1
    {12, 0, "KW0", 0, "AdvBPS0"},

    //    S#1    0.5    0      -1
    //    S#1      1   +1      +1
    {13, "URL0", 0, 0, "AdvBPU0"}

   };
  
  for (unsigned int i=0; i < countof(requests_130); ++i)
  {
    send_request(requests_130[i], 130);
    FAIL_CONTEXT(
      TriggerChannelsCheck(
        users[requests_130[i].user_index],
        this,
        requests_130[i].exp_trigger_channels).check(),
      "Check matching" + strof(i));
  }
  
  {
    const Diffs diff[6] =
    {
      // Today
      //   Session
      //     level = 0.2
      Diffs(0),
      Diffs(0),
      //     level = 0.5
      Diffs().
        users_regular(1).
        users_from_now(-2),
      Diffs().
        users_regular(1).
        users_from_now(-1),
      //     level = 1.0
      Diffs().
        users_regular(1).
        users_from_now(2),
      Diffs().
        users_regular(0).
        users_from_now(1)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn, diff, stats)).check(),
      "130 seconds later");

  }
}


void
ChannelInventoryEstimStatsTest::test_4th_request_tomorrow()
{
  add_descr_phrase("4th request (tomorrow)");

  Generics::Time next_day = base_time + 25*60*60;
  ORM::StatsArray<ORM::ChannelInventoryEstimStats, 15> stats;

  // Next day
  //   Session
  //     level = 0.2
  stats[0].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(0.2).
    sdate(next_day);
  stats[0].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 0.2, tomorrow");
  stats[1].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(0.2).
    sdate(next_day);
  stats[1].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 0.2, tomorrow");
  //     level = 0.5
  stats[2].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(0.5).
    sdate(next_day);
  stats[2].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 0.5, tomorrow");
  stats[3].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(0.5).
    sdate(next_day);
  stats[3].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 0.5, tomorrow");
  //     level = 1.0
  stats[4].key().
    channel_id(fetch_int("AdvChannel0")).
    colo_id(1).
    match_level(1.0).
    sdate(next_day);
  stats[4].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "level 1.0, tomorrow");
  stats[5].key().
    channel_id(fetch_int("AdvChannel1")).
    colo_id(1).
    match_level(1.0).
    sdate(next_day);
  stats[5].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "level 1.0, tomorrow");
  //  History & History+today
  //   HT
  //     level = 1.0
  stats[6].key().
    channel_id(fetch_int("AdvChannelHT")).
    colo_id(1).
    match_level(1.0).
    sdate(next_day);
  stats[6].description(
    "ChannelInventoryEstimStats.History+today channel - "
    "level 1.0, tomorrow");
  //   H1
  //     level = 1.0
  stats[7].key().
    channel_id(fetch_int("AdvChannelH1")).
    colo_id(1).
    match_level(1.0).
    sdate(next_day);
  stats[7].description(
    "ChannelInventoryEstimStats.History channel#1 - "
    "level 1,0, tomorrow");
  //     level = 2.0
  stats[8].key().
    channel_id(fetch_int("AdvChannelH1")).
    colo_id(1).
    match_level(2.0).
    sdate(next_day);
  stats[8].description(
    "ChannelInventoryEstimStats.History channel#1 - "
    "level 2,0, tomorrow");
  //   H2
  //     level = 2.0
  stats[9].key().
    channel_id(fetch_int("AdvChannelH2")).
    colo_id(1).
    match_level(2.0).
    sdate(next_day);
  stats[9].description(
    "ChannelInventoryEstimStats.History channel#2 - "
    "level 2,0, tomorrow");
  // Today not changed
  //   Session
  stats[10].key().
    channel_id(fetch_int("AdvChannel0")).
    sdate(base_time);
  stats[10].description(
    "ChannelInventoryEstimStats.Session channel#1 - "
    "today not changed");
  stats[11].key().
    channel_id(fetch_int("AdvChannel1")).
    sdate(base_time);
  stats[11].description(
    "ChannelInventoryEstimStats.Session channel#2 - "
    "today not changed");
  //   HT
  stats[12].key().
    channel_id(fetch_int("AdvChannelHT")).
    sdate(base_time);
  stats[12].description(
    "ChannelInventoryEstimStats.History+today channel - "
    "today not changed");
  //   H1 & H2
  stats[13].key().
    channel_id(fetch_int("AdvChannelH1")).
    sdate(base_time);
  stats[13].description(
    "ChannelInventoryEstimStats.History channel#1 - "
    "today not changed");
  stats[14].key().
    channel_id(fetch_int("AdvChannelH2")).
    sdate(base_time);
  stats[14].description(
    "ChannelInventoryEstimStats.History channel#2 - "
    "today not changed");
  stats.select(conn);

  for (unsigned int i=0; i < countof(requests); ++i)
  {
    send_request(requests[i], 25*60*60);
    FAIL_CONTEXT(
      TriggerChannelsCheck(
        users[requests[i].user_index],
        this,
        requests[i].exp_trigger_channels).check(),
      "Check matching" + strof(i));
  }

  {
    const Diffs diff[15] =
    {
      // Next day
      //   Session
      //     level = 0.2
      Diffs().
        users_regular(0).
        users_from_now(5),
      Diffs().
        users_regular(0).
        users_from_now(7),
      //     level = 0.5
      Diffs().
        users_regular(0).
        users_from_now(5),
      Diffs().
        users_regular(0).
        users_from_now(3),
      //     level = 1.0
      Diffs(1),
      Diffs(4),
      //   HT, level = 0.5
      Diffs(1),
      //   H1
      //     level = 1.0
      Diffs().
        users_regular(1).
        users_from_now(0),
      //     level = 2.0
      Diffs().
        users_regular(0).
        users_from_now(1),
      //   H2
      Diffs().
        users_regular(0).
        users_from_now(1),
      // Today
      //   Session
      Diffs(0),
      Diffs(0),
      //   HT
      Diffs(0),
      //   H1 & H2
      Diffs(0),
      Diffs(0)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn, diff, stats)).check(),
      "tomorrow");

  }  
}

void ChannelInventoryEstimStatsTest::test_5th_request_after_tomorrow()
{
  add_descr_phrase("5th request (after tomorrow)");

  ORM::StatsArray<ORM::ChannelInventoryEstimStats, 4> stats;

  // After tomorrow
  //   H#2, level=1.0
  stats[0].key().
    channel_id(fetch_int("AdvChannelH2")).
    colo_id(1).
    match_level(1.0).
    sdate(base_time + 49*60*60);
  stats[0].description(
    "ChannelInventoryEstimStats.History channel#2 - "
    "level 1.0, after tomorrow");
  //   H#2, level=2.0
  stats[1].key().
    channel_id(fetch_int("AdvChannelH2")).
    colo_id(1).
    match_level(2.0).
    sdate(base_time + 49*60*60);
  stats[1].description(
    "ChannelInventoryEstimStats.History channel#2 - "
    "level 2.0, after tomorrow");
  // Tomorrow
  stats[2].key().
    channel_id(fetch_int("AdvChannelH2")).
    sdate(base_time + 25*60*60);
  stats[2].description(
    "ChannelInventoryEstimStats.History channel#2 - "
    "tomorrow not changed");
  // Today
  stats[3].key().
    channel_id(fetch_int("AdvChannelH2")).
    sdate(base_time);
  stats[3].description(
    "ChannelInventoryEstimStats.History channel#2 - "
    "today not changed");
  stats.select(conn);
  
  const UserRequestType requests_after_tomorrow[] =
   {
    // channel level regular from_now   
    //    H#2      1    +1       0     after tomorrow
    //    H#2      2     0      +1     after tomorrow
    {24, 0, 0, "KWH2", "AdvBPPH2"},
   };

  for (unsigned int i=0; i < countof(requests_after_tomorrow); ++i)
  {
    send_request(requests_after_tomorrow[i], 49*60*60);
    FAIL_CONTEXT(
      TriggerChannelsCheck(
        users[requests_after_tomorrow[i].user_index],
        this,
        requests_after_tomorrow[i].exp_trigger_channels).check(),
      "Check matching" + strof(i));
  }

  {
    const Diffs diff[4] =
    {
      // After tomorrow
      //   H#2, level=1.0
      Diffs().
        users_regular(1).
        users_from_now(0),
      //   H#2, level=2.0
      Diffs().
        users_regular(0).
        users_from_now(1),
      // Tomorrow
      Diffs(0),
      // Today
      Diffs(0)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn, diff, stats)).check(),
      "after tomorrow");
  }
}

void ChannelInventoryEstimStatsTest::test_different_colo_()
{
  std::string description("Different colo om matching (ADSC-7276)");
  add_descr_phrase(description);

  unsigned long colo1 = fetch_int("COLO1");
  unsigned long colo2 = fetch_int("COLO2");
  unsigned long channel = fetch_int("AdvChannelCOLO");

  ORM::StatsArray<ORM::ChannelInventoryEstimStats, 3> stats;
    
  stats[0].key().
    channel_id(channel).colo_id(colo1).match_level(0.2).sdate(base_time);
  stats[1].key().
    channel_id(channel).colo_id(colo2).match_level(0.2).sdate(base_time);
  stats[2].key().
    channel_id(channel).colo_id(colo2).match_level(0.5).sdate(base_time);
  for (size_t i = 0; i < 3; ++i)
  {
    stats[i].description(description);
  } 
  stats.select(conn);

  AutoTest::AdClient user(AutoTest::AdClient::create_user(this));

  NSLookupRequest request;
  request.referer_kw = fetch_string("KWCOLO");
  request.debug_time = base_time;
  request.colo = colo1;

  user.process_request(request, "requesting colo1");

  FAIL_CONTEXT(
    TriggerChannelsCheck(
      user,
      this,
      "AdvBPPCOLO").check(),
    "Check matching expected channel");

  request.colo = colo2;

  user.process_request(request, "requesting colo2");

  FAIL_CONTEXT(
    TriggerChannelsCheck(
      user,
      this,
      "AdvBPPCOLO").check(),
    "Check matching expected channel");

  const Diffs diffs[3] =
  {
    Diffs().users_regular(1).users_from_now(1),
    Diffs().users_regular(-1).users_from_now(-1),
    Diffs().users_regular(1).users_from_now(1)
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(conn, diffs, stats)).check(),
    "Checking expected stats in ChannelInventoryEstimStats table");
}

void
ChannelInventoryEstimStatsTest::send_request(const UserRequestType& request,
                                             unsigned int time_ofset)
{
  NSLookupRequest ns_request;
  if (request.referer) ns_request.referer = fetch_string(request.referer);
  if (request.referer_kw) ns_request.referer_kw = fetch_string(request.referer_kw);
  if (request.search_kw) ns_request.search = fetch_string(request.search_kw);
  ns_request.debug_time = base_time + time_ofset;
 
  users.process_request_i(request.user_index, ns_request);
}



