

#ifndef _AUTOTEST__COLOUSERS_
#define _AUTOTEST__COLOUSERS_

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class ColoUsers
 * @brief Check ColoUsers logging
 */
namespace ORM = AutoTest::ORM;
class ColoUsers: public BaseDBUnit
{

  typedef ORM::ColoUserStats ColoUserStat;
  typedef ORM::StatsList<ColoUserStat> ColoUserStats;
  typedef ColoUserStat::Diffs ColoUserDiff;
  typedef std::list<ColoUserDiff> ColoUserDiffs;
  
  typedef ORM::GlobalColoUserStats GlobalColoUserStat;
  typedef ORM::StatsList<GlobalColoUserStat> GlobalColoUserStats;
  typedef GlobalColoUserStat::Diffs GlobalColoUserDiff;
  typedef std::list<GlobalColoUserDiff> GlobalColoUserDiffs;
  
  typedef ORM::CreatedUserStats CreatedUserStat;
  typedef ORM::StatsList<CreatedUserStat> CreatedUserStats;
  typedef CreatedUserStat::Diffs CreatedUserDiff;
  typedef std::list<CreatedUserDiff> CreatedUserDiffs;


  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef std::vector<AdClient> Clients;
  typedef std::vector<TemporaryAdClient> TmpClients;
  typedef std::vector<std::string> Hids;

  struct TestRequest
  {
    AdClient& client;
    int time_offset;
    const char* colo;
    const char* tid;
    const char* referer_kw;
    const char* hid;
    TemporaryAdClient* temporary;
    unsigned int flags;
  };

public:
 
  ColoUsers(UnitStat& stat_var, 
            const char* task_name, 
            XsdParams params_var): 
    BaseDBUnit(stat_var, task_name, params_var)
  { }
  
  virtual ~ColoUsers() noexcept
  { }

protected:

  virtual bool run();

  virtual void set_up();
  
  virtual void tear_down();
 
private:

  AutoTest::Time base_time_;

  template<class Stat, class Expected, size_t Count>
  void
  add_stats_(
    ORM::StatsList<Stat>& stats,
    std::list<typename Stat::Diffs>& diffs,
    const Expected (&expected)[Count]);

  template<class Expected>
  void
  init_stat_(
    ColoUserStat& stat,
    const Expected& expected);

  template<class Expected>
  void
  init_stat_(
    GlobalColoUserStat& stat,
    const Expected& expected);

  template<class Expected>
  void
  init_stat_(
    CreatedUserStat& stat,
    const Expected& expected);

  template<class Diffs, class Expected>
  void
  add_diff_(
    std::list<Diffs>& diffs,
    const Expected& expected);

  template<class Diffs, class Expected>
  void
  init_sum_diff_(
    Diffs& diff,
    const Expected& expected);
  
  template<class Expected>
  void
  init_sum_diff_(
    ColoUserDiff& diff,
    const Expected& expected);

  template<size_t Count>
  void
  process_requests_(
    const AutoTest::Time& base_time,
    const TestRequest(&requests)[Count]);

  void
  unique_users_stats_();

  void
  unique_hids_();
  
  void
  create_and_last_appearance_dates_();

  void
  non_gmt_timezone_();
  
  void
  basic_async_part_1_(
    AdClient& client,
    TemporaryAdClient& temporary,
    const std::string& hid);
  
  void
  basic_async_part_2_(
    AdClient& client,
    TemporaryAdClient& temporary,
    const std::string& hid);
  
  void
  big_date_difference_();
  
  void
  merge_on_adrequest_();

  void
  create_date_after_merge_();

  void
  invalid_merge_();

  void
  optout_();
  
  void
  non_serialized_();
  
  void
  pub_inventory_();

  void
  oo_service_();
};

#endif  // _AUTOTEST__COLOUSERS_
