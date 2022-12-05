
#ifndef _AUTOTEST__CHANNELINVENTORYESTIMMERGEUSERS_
#define _AUTOTEST__CHANNELINVENTORYESTIMMERGEUSERS_
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;
 
class ChannelInventoryEstimMergeUsers: public BaseUnit
{
  typedef AutoTest::DBC::Conn DBConnection;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;    
public:
  ChannelInventoryEstimMergeUsers(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var),
    conn(open_pq())
  {};
 
  virtual ~ChannelInventoryEstimMergeUsers() noexcept
  {};
 
private:
 
  virtual bool run_test();

  // Test cases

  /**
   * @brief Simple merging (part#1).
   *
   * Test ChannelInventoryEstimStats after
   * simple merge persistent & temporary clients.
   */
  void simple_merging_1();

  /**
   * @brief Simple merging (part#2).
   *
   * Test ChannelInventoryEstimStats after
   * simple merge persistent & temporary clients
   * (with history optimization).
   */
  void simple_merging_2();

  /**
   * @brief Temporary user history lost on merge.
   *
   * Test ChannelInventoryEstimStats after
   * merge persistent & temporary clients on
   * next day of temporary client history
   * profile creation.
   */
  void temp_user_lost_history();

  /**
   * @brief Temporary user "rotten" session channel profile on merge.
   *
   * Test ChannelInventoryEstimStats after
   * merge persistent & temporary clients after
   * session channel "become rotten" in temporary client
   * profile creation.
   */
  void rotten_channel_on_merging();

  /**
   * @brief Match level exceed on merge.
   *
   * Test ChannelInventoryEstimStats that
   * merge persistent & temporary clients
   * cann't generate records with
   * match level > 2.0.
   */
  void exceed_match_level_on_merging();

  Generics::Time base_time;
  
  DBConnection conn;
};

#endif //_AUTOTEST__CHANNELINVENTORYESTIMMERGEUSERS_

