#ifndef _UNITTEST__CHANNELINVENTORYTEST_
#define _UNITTEST__CHANNELINVENTORYTEST_

#include <tests/AutoTests/Commons/Common.hpp>

class ChannelInventoryTest:
  public BaseDBUnit
{

  typedef AutoTest::DBC::Conn DBConnection;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ORM::ChannelInventoryStats Stat;
  typedef AutoTest::ORM::StatsList<Stat> Stats;
  typedef AutoTest::ORM::ChannelInventoryStats::Diffs Diff;
  typedef std::list<Diff> Diffs;

public:
  
  struct UserRequest
  {
    int time_ofset;
    const char* referer_kws;
    const char* expected_triggers;
    const char* expected_history;
  };
  
public: 
  ChannelInventoryTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var);
 
  virtual ~ChannelInventoryTest() noexcept
  {};

private:

  struct ChannelInventoryStatsRow
  {
    const char* description;
    const char* colo_id;
    AutoTest::Time sdate;
    const char* channel_id;
    unsigned int active_users;
    unsigned int total_users;
  };

  template<size_t SIZE>
  void fill_expected_stats_(
    const ChannelInventoryStatsRow (&statistic)[SIZE],
    Stats& stats,
    Diffs& expected_diffs);

  void set_up();
  bool run();
  void tear_down();

  //  Test cases
  void base_scenario_1(AdClient& client);
  void base_scenario_2(AdClient& client);
  void active_users_1(AdClient& user1, AdClient& /*user2*/,
    Stats& today_stats, Diffs& today_diffs);
  void active_users_2(AdClient& user1, AdClient& user2,
    Stats& today_stats, Diffs& today_diffs);
  void active_users_3(AdClient& user1, AdClient& /*user2*/);

  void daily_processing();

  // Total/active user counters with delayed
  // logs delivery emulation
  void delayed_logs_1(AdClient& client);
  void delayed_logs_2(AdClient& client);

  // ADSC-4582
  // Unordered (by time) sequence
  // of requests 
  void late_request_1(AdClient& client);
  void late_request_2(AdClient& client);

  void merging_1(const AutoTest::Time& now,
    AdClient& persistent,
    TemporaryAdClient& temporary);
  void merging_2(const AutoTest::Time& now,
    AdClient& persistent,
    TemporaryAdClient& /*temporary*/);
  void merging_3(const AutoTest::Time& now,
    AdClient& persistent,
    TemporaryAdClient& temporary);

  // Utils
  template<size_t Count>
  void process_requests(
    AdClient& client,
    const std::string& description,
    const AutoTest::Time& base_time,
    const UserRequest(&requests)[Count]);

  void set_referer_kws(
    NSLookupRequest& request,
    const char* referer_kws);


private:
  AutoTest::Time today;
  AutoTest::Time yesterday;
};

#endif
