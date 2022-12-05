
#ifndef _AUTOTEST__ADVERTISERUSERSTATSTEST_
#define _AUTOTEST__ADVERTISERUSERSTATSTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;

class AdvertiserUserStatsTest : public BaseUnit
{

  typedef AutoTest::AdClient AdClient;
  typedef ORM::AdvertiserUserStats AdvStats;
  typedef AdvStats::Diffs AdvDiff;
  typedef ORM::CampaignUserStats CmpStats;
  typedef CmpStats::Diffs CmpDiff;
  typedef ORM::CCGUserStats CCGStats;
  typedef CCGStats::Diffs CCGDiff;
  typedef ORM::CCUserStats CCStats;
  typedef CCStats::Diffs CCDiff;
  
  struct Expected
  {
    const char* advertisers;
    const char* campaigns;
    const char* ccgs;
    const char* ccs;
    AutoTest::Time last_appearance;
    AutoTest::Time sdate;
    int unique_users;
    int display_unique_users;
    int text_unique_users;
  };

  struct Request
  {
    const char* kwd;
    const char* tid;
    int time_ofset;
    const char* format;
    const char* expected_ccs;
  };

  struct SumExpected
  {
    unsigned int users;
    long int control_sum;
  };

  typedef const char* Expected::* Entity;

  typedef std::list<std::string> RequestList;
  
public:
  AdvertiserUserStatsTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var),
    conn_(open_pq()),
    today_()
  {};

  virtual ~AdvertiserUserStatsTest() noexcept
  {};

private:

  AutoTest::DBC::Conn conn_;
  AutoTest::Time today_;
  RequestList requests_;
  
  // Stats
  ORM::StatsList<AdvStats> adv_stats_;
  ORM::StatsList<AdvStats> adv_sum_stats_;
  std::list<AdvDiff> adv_diffs_;
  std::list<AdvDiff> adv_sum_diffs_;
  ORM::StatsList<CmpStats> cmp_stats_;
  ORM::StatsList<CmpStats> cmp_sum_stats_;
  std::list<CmpDiff> cmp_diffs_;
  std::list<CmpDiff> cmp_sum_diffs_;
  ORM::StatsList<CCGStats> ccg_stats_;
  ORM::StatsList<CCGStats> ccg_sum_stats_;
  std::list<CCGDiff> ccg_diffs_;
  std::list<CCGDiff> ccg_sum_diffs_;
  ORM::StatsList<CCStats> cc_stats_;
  ORM::StatsList<CCStats> cc_sum_stats_;
  std::list<CCDiff> cc_diffs_;
  std::list<CCDiff> cc_sum_diffs_;

  // Utils
  void
  log_request_(std::string request_id);

  template<size_t ReqCount>
  void
  process_requests_(
    AdClient& client,
    const std::string& description,
    const Request(&requests)[ReqCount],
    const AutoTest::Time& base_time,
    unsigned long colo = 0);

  template <class Stats>
  void
  initialize_stat_(
    Stats& stat,
    typename Stats::Diffs& diff,
    const Expected& expected,
    unsigned long entity,
    unsigned long colo);

  void
  initialize_stat_(
    AdvStats& stat,
    AdvDiff& diff,
    const Expected& expected,
    unsigned long entity,
    unsigned long colo);

  template <class Stats, size_t Count>
  void
  initialize_stats_(
    const std::string& description,
    ORM::StatsList<Stats>& stats,
    std::list<typename Stats::Diffs>& diffs,
    const Expected (&expected) [Count],
    Entity entity,
    unsigned long colo);

  template <size_t Count>
  void
  initialize_all_stats_(
    const std::string& description,
    const Expected (&expected) [Count],
    unsigned long colo = 0);

  template <class Stats, typename Key, size_t Count>
  void
  initialize_sum_stats_(
    const std::string& description,
    ORM::StatsList<Stats>& stats,
    std::list<typename Stats::Diffs>& diffs,
    const std::string& entity_name,
    Key& (Key::*Func)(const int&),
    const SumExpected (&expected) [Count]);

  void check_stats_();

  virtual
  bool
  run_test();

  // Cases
  void
  base_();

  void
  unique_users_();

  void
  last_usage_();

  void
  timezones_();

  AdClient
  async_part_1_();
  
  void
  async_part_2_(
    AdClient& client);

  void
  async_part_3_(
    AdClient& client);


  AdClient
  big_date_diff_part_1_();

  void
  big_date_diff_part_2_(
    AdClient& client);

  void
  colo_();

  void
  temporary_user_();

  void
  optout_users_();
};

#endif // _AUTOTEST__ADVERTISERUSERSTATSTEST_
