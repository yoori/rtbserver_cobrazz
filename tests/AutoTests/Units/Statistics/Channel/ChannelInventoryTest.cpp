#include "ChannelInventoryTest.hpp"

REFLECT_UNIT(ChannelInventoryTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::InventoryProfileChecker InventoryProfileChecker;

  const AutoTest::Time UNDEF_DATE = Generics::Time::ZERO - 1; 

  namespace ORM = AutoTest::ORM;

  /**
   * @class ProfileChecker
   * @brief Wait user profile appearance on ExpressionMatcher.
   */
  class ProfileChecker : public AutoTest::Checker
  {
  public:

    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param test.
     */
    ProfileChecker(
      AutoTest::AdClient& user,
      BaseUnit* test) :
      user_(user),
      test_(test)
    { }

    /**
     * @brief Destructor.
     */
     virtual ~ProfileChecker() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_on_error = true) /*throw(eh::Exception)*/
    {

      std::string uid(user_.get_uid());

      return
        InventoryProfileChecker(
          test_,
          AutoTest::prepare_uid(uid),
          InventoryProfileChecker::Expected().
            user_id(
              AutoTest::prepare_uid(
                uid,
                AutoTest::UUE_EXPECTED_VALUE))).check(throw_on_error);
    }

  private:
    AutoTest::AdClient& user_;  // user
    BaseUnit* test_;            // test
  };
}

ChannelInventoryTest::ChannelInventoryTest(
  UnitStat& stat_var,
  const char* task_name,
  XsdParams params_var) :
  BaseDBUnit(stat_var, task_name, params_var),
  today(),
  yesterday(today - Generics::Time::ONE_DAY)
{}

template<size_t SIZE>
void ChannelInventoryTest::fill_expected_stats_(
  const ChannelInventoryStatsRow (&statistic)[SIZE],
  Stats& stats,
  Diffs& expected_diffs)
{
  for (size_t i = 0; i < SIZE; ++i)
  {
    Stat::Key key;
    key.channel_id(fetch_int(statistic[i].channel_id));
    if (statistic[i].sdate != UNDEF_DATE)
    {
      key.sdate(statistic[i].sdate);
    }
    if (statistic[i].colo_id)
    {
      key.colo_id(fetch_int(statistic[i].colo_id));
    }

    Stat stat(key);

    stat.description(statistic[i].description);
    stat.select(pq_conn_);

    stats.push_back(stat);
    expected_diffs.push_back(Diff().
      active_users(statistic[i].active_users).
      total_users(statistic[i].total_users));
  };
}

void ChannelInventoryTest::set_up()
{}

bool
ChannelInventoryTest::run()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_EXPRESSION_MATCHER)),
    "ExpressionMatcher need for this test");

  const char BASE_SCENARIO[] = "Base scenario";
  const char ACTIVE_USERS[] = "Active users";
  const char DELAYED_LOGS[] = "Delayed logs";
  const char LATE_REQUEST[] = "Late request";
  const char MERGIN[] = "Mergin";

  // 1st step
  AdClient base_user(AdClient::create_user(this));
  AUTOTEST_CASE(base_scenario_1(base_user), BASE_SCENARIO);
  
  AdClient active_user1(AdClient::create_user(this, yesterday - 1)),
           active_user2(AdClient::create_user(this, yesterday - 1));
  Stats active_today_stats;
  Diffs active_today_diffs;
  AUTOTEST_CASE(
    active_users_1(
      active_user1,
      active_user2,
      active_today_stats,
      active_today_diffs),
    ACTIVE_USERS);

  
  AUTOTEST_CASE(daily_processing(), "Daily processing");

  AdClient delayed_user(AdClient::create_user(this, yesterday - 1));
  AUTOTEST_CASE(delayed_logs_1(delayed_user), DELAYED_LOGS);

  AdClient late_user(AdClient::create_user(this));
  AUTOTEST_CASE(late_request_1(late_user), LATE_REQUEST);

  TemporaryAdClient merge_temp_user(TemporaryAdClient::create_user(this));
  AdClient merge_user(AdClient::create_user(this));
  // Use current time in this case, 
  // to reduce the likelihood of temporary
  // profile removing by delete-old-profiles procedure.
  // By default, temp_profile_lifetime = 30 mins
  AutoTest::Time now;
  AUTOTEST_CASE(merging_1(now, merge_user, merge_temp_user), MERGIN);

  check();

  // 2nd step
  AUTOTEST_CASE(base_scenario_2(base_user), BASE_SCENARIO);
  AUTOTEST_CASE(
    active_users_2(
      active_user1,
      active_user2,
      active_today_stats,
      active_today_diffs),
    ACTIVE_USERS);
  AUTOTEST_CASE(delayed_logs_2(delayed_user), DELAYED_LOGS);
  AUTOTEST_CASE(late_request_2(late_user), LATE_REQUEST);
  AUTOTEST_CASE(merging_2(now, merge_user, merge_temp_user), MERGIN);

  check();

  // 3rd step
  AUTOTEST_CASE(active_users_3(active_user1, active_user2), ACTIVE_USERS);
  AUTOTEST_CASE(merging_3(now, merge_user, merge_temp_user), MERGIN);

  // check() will be called implicitly (in BaseDBUnit::run_test)
  // check();

  return true;
}

void ChannelInventoryTest::tear_down()
{}

void ChannelInventoryTest::base_scenario_1(AdClient& client)
{
  std::string description("Base scenario. Yesterday check. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description                             colo_id           sdate       channel_id                active_users  total_users
    { "'Context', yesterday",                 0,                yesterday,  "Base/Channel-Context",   1,            1 },
    { "'History', yesterday",                 0,                yesterday,  "Base/Channel-History",   0,            0 },
    { "'Expression#1', yesterday",            0,                yesterday,  "Base/Expression-1",      1,            1 },
    { "'Expression#2', yesterday",            0,                yesterday,  "Base/Expression-2",      0,            0 },
    { "'Targeting#1', yesterday",              0,                yesterday,  "Base/Targeting-A1",       1,            1 },
    { "'Targeting#2', yesterday",              0,                yesterday,  "Base/Targeting-A2",       1,            1 },
    { "'Targeting#3', yesterday",              0,                yesterday,  "Base/Targeting-A3",       0,            0 },
    { "'Context', today",                     0,                today,      "Base/Channel-Context",   0,            0 },
    { "'History', today",                     0,                today,      "Base/Channel-History",   0,            0 },
    { "'Expression#1', today",                0,                today,      "Base/Expression-1",      0,            0 },
    { "'Expression#2', today",                0,                today,      "Base/Expression-2",      0,            0 },
    { "'Targeting', today",                    0,                today,      "Base/Targeting-A1",       0,            0 },
    { "'Targeting#2', today",                  0,                today,      "Base/Targeting-A2",       0,            0 },
    { "'Targeting#3', today",                  0,                today,      "Base/Targeting-A3",       0,            0 },
    { "'Context' with default colo",          "DefaultColo",    UNDEF_DATE, "Base/Channel-Context",   1,            1 },
    { "'Expression#1' with default colo",     "DefaultColo",    UNDEF_DATE, "Base/Expression-1",      1,            1 },
    { "'Context' with non default colo",      "NonDefaultColo", UNDEF_DATE, "Base/Channel-Context",   0,            0 },
    { "'Expression#1' with non default colo", "NonDefaultColo", UNDEF_DATE, "Base/Expression-1",      0,            0 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  NSLookupRequest request;
  request.referer_kw = fetch_string("Base/Kwd");
  request.loc_name = fetch_string("Location");

  AdClient unknown(AdClient::create_nonoptin_user(this));
  AdClient optout(AdClient::create_optout_user(this, yesterday));

  // Yesterday request
  request.debug_time = yesterday;
  client.process_request(request);
  unknown.process_request(request);
  optout.process_request(request);

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Base/BP-Context,Base/BP-History",
      client.debug_info.trigger_channels).check(),
      description +
      " Expected trigger_channels#1");

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Base/Ext-GEO",
      client.debug_info.geo_channels).check(),
    description +
      " Expected geo_channels#1");

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Base/Ext-Device",
      client.debug_info.device_channels).check(),
    description +
      " Expected device_channels#1");

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
}

void ChannelInventoryTest::base_scenario_2(AdClient& client)
{
  std::string description("Base scenario. Today check. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description                             colo_id           sdate       channel_id                active_users  total_users
    { "'Context', yesterday",                 0,                yesterday,  "Base/Channel-Context",   0,            0 },
    { "'History', yesterday",                 0,                yesterday,  "Base/Channel-History",   0,            0 },
    { "'Expression#1', yesterday",            0,                yesterday,  "Base/Expression-1",      0,            0 },
    { "'Expression#2', yesterday",            0,                yesterday,  "Base/Expression-2",      0,            0 },
    { "'Targeting#1', yesterday",              0,                yesterday,  "Base/Targeting-A1",       0,            0 },
    { "'Targeting#2', yesterday",              0,                yesterday,  "Base/Targeting-A2",       0,            0 },
    { "'Targeting#3', yesterday",              0,                yesterday,  "Base/Targeting-A3",       0,            0 },
    { "'Context', today",                     0,                today,      "Base/Channel-Context",   1,            1 },
    { "'History', today",                     0,                today,      "Base/Channel-History",   0,            0 },
    { "'Expression#1', today",                0,                today,      "Base/Expression-1",      1,            1 },
    { "'Expression#2', today",                0,                today,      "Base/Expression-2",      0,            0 },
    { "'Targeting', today",                    0,                today,      "Base/Targeting-A1",       1,            1 },
    { "'Targeting#2', today",                  0,                today,      "Base/Targeting-A2",       1,            1 },
    { "'Targeting#3', today",                  0,                today,      "Base/Targeting-A3",       0,            0 },
    { "'Context' with default colo",          "DefaultColo",    UNDEF_DATE, "Base/Channel-Context",   1,            1 },
    { "'Expression#1' with default colo",     "DefaultColo",    UNDEF_DATE, "Base/Expression-1",      1,            1 },
    { "'Context' with non default colo",      "NonDefaultColo", UNDEF_DATE, "Base/Channel-Context",   0,            0 },
    { "'Expression#1' with non default colo", "NonDefaultColo", UNDEF_DATE, "Base/Expression-1",      0,            0 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  NSLookupRequest request;
  request.referer_kw = fetch_string("Base/Kwd");
  request.loc_name = fetch_string("Location");
  request.debug_time = today;
  request.colo = fetch_int("NonDefaultColo"); // colo request param is not applicable in stats

  AdClient unknown(AdClient::create_nonoptin_user(this));
  AdClient optout(AdClient::create_optout_user(this, yesterday));

  // Today request
  client.process_request(request);
  unknown.process_request(request);
  optout.process_request(request);

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Base/BP-Context,Base/BP-History",
      client.debug_info.trigger_channels).check(),
    description +
      " Expected trigger_channels#2");

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Base/Ext-GEO",
      client.debug_info.geo_channels).check(),
    description +
      " Expected geo_channels#2");

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Base/Ext-Device",
      client.debug_info.device_channels).check(),
    description +
      " Expected device_channels#2");

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
}

void
ChannelInventoryTest::active_users_1(AdClient& user1, AdClient& /*user2*/,
  Stats& today_stats, Diffs& today_diffs)
{
  std::string description("Active users. Yesterday check. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow YESTERDAY_STATISTIC[] = {
//    description   colo_id sdate       channel_id                  active_users  total_users
    { "Channel#1",  0,      yesterday,  "ActiveUsers/Channel-B1",   1,            1 },
    { "Channel#2",  0,      yesterday,  "ActiveUsers/Channel-B2",   1,            1 },
    { "Expression", 0,      yesterday,  "ActiveUsers/Expression-E", 1,            1 }
  };

  Stats yesterday_stats;
  Diffs yesterday_diffs;  
  fill_expected_stats_(YESTERDAY_STATISTIC, yesterday_stats, yesterday_diffs);

  // We must select today stats before any requests
  // (we can't expect that server will not call daily-process command)
  const ChannelInventoryStatsRow TODAY_STATISTIC[] = {
//    description   colo_id sdate   channel_id                  active_users  total_users
    { "Channel#1",  0,      today,  "ActiveUsers/Channel-B1",   1,            2 },
    { "Channel#2",  0,      today,  "ActiveUsers/Channel-B2",   1,            2 },
    { "Expression", 0,      today,  "ActiveUsers/Expression-E", 1,            2 }
  };
  fill_expected_stats_(TODAY_STATISTIC, today_stats, today_diffs);

  user1.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("ActiveUsers/Kwd")).
      debug_time(yesterday));

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, yesterday_diffs, yesterday_stats));
}

void
ChannelInventoryTest::active_users_2(AdClient& user1, AdClient& user2,
  Stats& today_stats, Diffs& today_diffs)
{
  std::string description("Active users. Today check #1. ");
  add_descr_phrase(description);

  user2.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("ActiveUsers/Kwd")).
      debug_time(today));

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ProfileChecker(user1, this)).check());
    
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ProfileChecker(user2, this)).check());

  FAIL_CONTEXT(AutoTest::DailyProcess::execute(this));

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, today_diffs, today_stats));
}

void
ChannelInventoryTest::active_users_3(AdClient& user1, AdClient& /*user2*/)
{
  std::string description("Active users. Today check #2. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description   colo_id sdate   channel_id                  active_users  total_users
    { "Channel#1",  0,      today,  "ActiveUsers/Channel-B1",   1,            0 },
    { "Channel#2",  0,      today,  "ActiveUsers/Channel-B2",   1,            0 },
    { "Expression", 0,      today,  "ActiveUsers/Expression-E", 1,            0 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  user1.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("ActiveUsers/Kwd")).
      debug_time(today));

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
}

void
ChannelInventoryTest::daily_processing()
{
  std::string description("Daily processing. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description             colo_id sdate       channel_id            active_users  total_users
    { "Context, yesterday",     0,    yesterday,  "DailyProc/Channel-C",    1,        1 },
    { "H+T, yesterday",         0,    yesterday,  "DailyProc/Channel-HT",   1,        1 },
    { "Session, yesterday",     0,    yesterday,  "DailyProc/Channel-S",    1,        1 },
    { "History#1, yesterday",   0,    yesterday,  "DailyProc/Channel-H1",   0,        0 },
    { "History#2, yesterday",   0,    yesterday,  "DailyProc/Channel-H2",   0,        0 },
    { "History#3, yesterday",   0,    yesterday,  "DailyProc/Channel-H3",   0,        0 },
    { "Expression, yesterday",  0,    yesterday,  "DailyProc/Expression-E", 1,        1 },
    { "Targeting#1, yesterday",  0,    yesterday,  "DailyProc/Targeting-A1",  1,        1 },
    { "Targeting#2, yesterday",  0,    yesterday,  "DailyProc/Targeting-A2",  1,        1 },
    { "Context, today",         0,    today,      "DailyProc/Channel-C",    0,        1 },
    { "H+T, today",             0,    today,      "DailyProc/Channel-HT",   0,        0 },
    { "Session, today",         0,    today,      "DailyProc/Channel-S",    0,        0 },
    { "History#1, today",       0,    today,      "DailyProc/Channel-H1",   0,        1 },
    { "History#2, today",       0,    today,      "DailyProc/Channel-H2",   1,        1 },
    { "History#3, today",       0,    today,      "DailyProc/Channel-H3",   0,        1 },
    { "Expression, today",      0,    today,      "DailyProc/Expression-E", 1,        3 },
    { "Targeting#1, today",      0,    today,      "DailyProc/Targeting-A1",  1,        3 },
    { "Targeting#2, today",      0,    today,      "DailyProc/Targeting-A2",  1,        1 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  AutoTest::Time create_time(today - Generics::Time::ONE_WEEK);

  // User#1
  {
    // Daily process runs before first channel match
    const ChannelInventoryTest::UserRequest BEFORE_MATCH[] = {{
      -1,                                                                   //time_offset
      "DailyProc/Kwd-C,DailyProc/Kwd-HT,DailyProc/Kwd-S,DailyProc/Kwd-H1",  //referer_kws
      "DailyProc/BP-C,DailyProc/BP-HT,DailyProc/BP-S,DailyProc/BP-H1",      //expected_triggers
      "DailyProc/Channel-C,DailyProc/Channel-S,DailyProc/Channel-HT" }};    //expected_history

    Generics::Time midnignt(today.get_gm_time().get_date());

    AdClient client(AdClient::create_user(this, create_time));
    process_requests(client,
      description + " Before first match.",
      midnignt, BEFORE_MATCH);

  }

  // User#2
  {
    // Daily process runs after first channel match on the current day
    const ChannelInventoryTest::UserRequest YESTERDAY_MATCH[] =
    {
      { -24*60*60, "DailyProc/Kwd-H2", "DailyProc/BP-H2", 0 },
      { 0,  0,  0, "DailyProc/Channel-H2" }
    };

    AdClient client(AdClient::create_user(this, create_time));
    process_requests(client,
      description + " After yesterday match.",
      today, YESTERDAY_MATCH);
  }

  // User#3
  {
    // Daily process shouldn't update data for the previous date
    const ChannelInventoryTest::UserRequest TWO_DAYS_LATER_MATCH[] =
    {
      { -2*24*60*60, "DailyProc/Kwd-H3", "DailyProc/BP-H3", 0 }
    };

    AdClient client(AdClient::create_user(this, create_time));

    process_requests(client,
      description + " After 2 days later match.",
      today, TWO_DAYS_LATER_MATCH);
  }

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));

}

void ChannelInventoryTest::delayed_logs_1(AdClient& client)
{
  std::string description("Delayed logs. After daily processing. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description         colo_id sdate       channel_id            active_users  total_users
    { "Context, yesterday", 0,    yesterday,  "DelayedLogs/Channel-C",  1,          1 },
    { "H+T, yesterday",     0,    yesterday,  "DelayedLogs/Channel-HT", 1,          1 },
    { "Context, today",     0,    today,      "DelayedLogs/Channel-C",  0,          0 },
    { "H+T, today",         0,    today,      "DelayedLogs/Channel-HT", 0,          1 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("DelayedLogs/Kwd")).
      debug_time(today - Generics::Time::ONE_DAY));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "DelayedLogs/BP-C,DelayedLogs/BP-HT",
      client.debug_info.trigger_channels).check(),
    description +
      " Expected trigger_channels#1");

  client.repeat_request();

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "DelayedLogs/BP-C,DelayedLogs/BP-HT",
      client.debug_info.trigger_channels).check(),
    description +
      " Expected trigger_channels#2");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ProfileChecker(client, this)).check());

  FAIL_CONTEXT(AutoTest::DailyProcess::execute(this));

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
};

void ChannelInventoryTest::delayed_logs_2(AdClient& client)
{
  std::string description("Delayed logs. After lated request. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description       colo_id   sdate       channel_id            active_users  total_users
    { "Session channel",    0,    yesterday,  "DelayedLogs/Channel-S",  1,          1 },
    { "Context, yesterday", 0,    yesterday,  "DelayedLogs/Channel-C",  0,          0 },
    { "H+T, yesterday",     0,    yesterday,  "DelayedLogs/Channel-HT", 0,          0 },
    { "Context, today",     0,    today,      "DelayedLogs/Channel-C",  0,          0 },
    { "H+T, today",         0,    today,      "DelayedLogs/Channel-HT", 0,          0 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  NSLookupRequest request;
  set_referer_kws(request, "DelayedLogs/Kwd,DelayedLogs/Kwd-S");
  request.debug_time = today - Generics::Time::ONE_DAY;
  client.process_request(request);

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "DelayedLogs/BP-C,DelayedLogs/BP-HT,DelayedLogs/BP-S",
      client.debug_info.trigger_channels).check(),
    description +
      " Expected trigger_channels#3");

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
}

void
ChannelInventoryTest::late_request_1(AdClient& client)
{
  std::string description("Late request. Yesterday check. ");
  add_descr_phrase(description);

  AutoTest::Time midnight(today.get_gm_time().get_date());

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description colo_id sdate       channel_id            active_users  total_users
    { "Yesterday",  0,    yesterday,  "LateRequest/Channel-B",  1,        1 },
    { "Today",      0,    today,      "LateRequest/Channel-B",  0,        0 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  client.process_request(
    NSLookupRequest().
    referer_kw(fetch_string("LateRequest/Kwd-B")).
    debug_time(
      midnight -
      Generics::Time::ONE_SECOND * 5));

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
}

void
ChannelInventoryTest::late_request_2(AdClient& client)
{
  std::string description("Late request. After today/yesterday switching. ");
  add_descr_phrase(description);

  AutoTest::Time midnight(today.get_gm_time().get_date());

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description colo_id sdate       channel_id            active_users  total_users
    { "Yesterday",  0,    yesterday,  "LateRequest/Channel-B",  0,        0 },
    { "Today",      0,    today,      "LateRequest/Channel-B",  1,        1 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  std::string keyword = fetch_string("LateRequest/Kwd-B");

  client.process_request(
    NSLookupRequest().
      referer_kw(keyword).
      debug_time(today));

  // unordered (by time) request
  client.process_request(
    NSLookupRequest().
      referer_kw(keyword).
      debug_time(midnight - Generics::Time::ONE_SECOND * 5));

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
}


void ChannelInventoryTest::merging_1(
  const AutoTest::Time& now,
  AdClient& persistent,
  TemporaryAdClient& temporary)
{
  std::string description("Merging. After temporary matches. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description colo_id sdate       channel_id                active_users  total_users
    { "ChannelInventory. Context",  0,  now,  "Merging/Channel-C",  0,        0 },
    { "ChannelInventory. H+T",      0,  now,  "Merging/Channel-HT", 0,        0 },
    { "ChannelInventory. Session",  0,  now,  "Merging/Channel-S",  1,        1 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  NSLookupRequest request;
  request.debug_time =  now;
  set_referer_kws(request, "Merging/Kwd-C,Merging/Kwd-HT");

  temporary.process_request(request);

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Merging/Channel-C,Merging/Channel-HT",
      temporary.debug_info.history_channels).check(),
    description +
      " History check#1");

  request.referer_kw = fetch_string("Merging/Kwd-S");
  persistent.process_request(request);

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Merging/Channel-S",
      persistent.debug_info.history_channels).check(),
    description +
      " History check#2");

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
};

void ChannelInventoryTest::merging_2(
  const AutoTest::Time& now,
  AdClient& persistent,
  TemporaryAdClient& /*temporary*/)
{
  std::string description("Merging. After persistent matches. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description colo_id sdate       channel_id                active_users  total_users
    { "ChannelInventory. Context",  0,  now,  "Merging/Channel-C",  1,        1 },
    { "ChannelInventory. H+T",      0,  now,  "Merging/Channel-HT", 0,        0 },
    { "ChannelInventory. Session",  0,  now,  "Merging/Channel-S",  0,        0 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  persistent.process_request(
    NSLookupRequest().
      debug_time(now).
      referer_kw(fetch_string("Merging/Kwd-C")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Merging/Channel-C",
      persistent.debug_info.history_channels).check(),
    description +
      " History check#3");

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
};

void ChannelInventoryTest::merging_3(
  const AutoTest::Time& now,
  AdClient& persistent,
  TemporaryAdClient& temporary)
{
  std::string description("Merging. After merging. ");
  add_descr_phrase(description);

  const ChannelInventoryStatsRow STATISTIC[] = {
//    description colo_id sdate       channel_id                active_users  total_users
    { "ChannelInventory. Context",  0,  now,  "Merging/Channel-C",  0,        0 },
    { "ChannelInventory. H+T",      0,  now,  "Merging/Channel-HT", 1,        1 },
    { "ChannelInventory. Session",  0,  now,  "Merging/Channel-S",  0,        0 }
  };

  Stats stats;
  Diffs expected_diffs;
  fill_expected_stats_(STATISTIC, stats, expected_diffs);

  persistent.merge(temporary,
    NSLookupRequest().
      debug_time(now).
      referer_kw(fetch_string("Merging/Kwd-C")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Merging/Channel-C,Merging/Channel-HT",
      persistent.debug_info.history_channels).check(),
    description +
      " History check#4");

  ADD_WAIT_CHECKER(description,
    AutoTest::stats_diff_checker(pq_conn_, expected_diffs, stats));
}

template<size_t Count>
void
ChannelInventoryTest::process_requests(
  AdClient& client,
  const std::string& description,
  const AutoTest::Time& base_time,
  const UserRequest(&requests)[Count])
{

  for (size_t i = 0; i < Count; ++i)
  {
    NSLookupRequest request;
    set_referer_kws(request, requests[i].referer_kws);
    request.debug_time = base_time + requests[i].time_ofset;
    request.loc_name = fetch_string("Location");

    client.process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        requests[i].expected_triggers,
        client.debug_info.trigger_channels).check(),
      description +
        " trigger_channels check#" + strof(i));

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        requests[i].expected_history,
        client.debug_info.history_channels).check(),
      description +
        " history_channels check#" + strof(i));
  }

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ProfileChecker(client, this)).check());

  // call inventory daily processing
  // daily processing make today channel matching for user,
  // it's leads to today total_users + 1
  FAIL_CONTEXT(AutoTest::DailyProcess::execute(this));
}

void
ChannelInventoryTest::set_referer_kws(
  NSLookupRequest& request,
  const char* referer_kws)
{
  if ( referer_kws )
  {
    std::string value;
    String::SubString s(referer_kws);
    String::StringManip::SplitComma tokenizer(s);
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      if (!value.empty())
      {
        value+=",";
      }
      value += fetch_string(token.str());
    }
    request.referer_kw = value;
  }
}
