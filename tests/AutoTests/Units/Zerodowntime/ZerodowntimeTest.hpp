
#ifndef _AUTOTEST__ZERODOWNTIMETEST_
#define _AUTOTEST__ZERODOWNTIMETEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;

class ZerodowntimeTest : public BaseDBUnit
{
  typedef AutoTest::ORM::ORMRestorer<AutoTest::ORM::PQ::CCGSite> CCGSite;

  enum ClusterGroup
  {
    CG_TR_1,
    CG_TR_2,
    CG_FE_1,
    CG_FE_2,
    CG_BE,
    CG_LP,
    CG_ALL
  };

  enum ClusterCommand
  {
    CC_START,
    CC_STOP,
    CC_RESTART
  };


public:
  ZerodowntimeTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var),
    campaign_managers_(
      get_config().get_services(
        CTE_CENTRAL, STE_CAMPAIGN_MANAGER)),
    channel_controllers_(
      get_config().get_services(
        CTE_CENTRAL, STE_CHANNEL_CONTROLLER)),
    channel_search_servers_(
      get_config().get_services(
        CTE_CENTRAL, STE_CHANNEL_SEARCH_SERVER)),
    user_info_managers_(
      get_config().get_services(
        CTE_CENTRAL, STE_USER_INFO_MANAGER)),
    expression_matchers_(
      get_config().get_services(
        CTE_CENTRAL, STE_EXPRESSION_MATCHER)),
    no_db_(!stat_var.db_active())
  { }

  virtual ~ZerodowntimeTest() noexcept
  { }

protected:

  virtual
  void
  set_up();

  virtual
  void
  tear_down();

  virtual
  bool
  run();

//   virtual
private:

  GlobalConfig::ServiceList campaign_managers_;
  GlobalConfig::ServiceList channel_controllers_;
  GlobalConfig::ServiceList channel_search_servers_;
  GlobalConfig::ServiceList user_info_managers_;
  GlobalConfig::ServiceList expression_matchers_;

  AutoTest::Time time_;
  bool no_db_;

  // Statistics
  ORM::StatsList<ORM::HourlyStats> request_stats_;
  ORM::ColoUserStats isp_stats_;
  ORM::StatsList<ORM::ChannelTriggerStats> trigger_stats_;
  ORM::StatsList<ORM::SiteChannelStats> site_channel_stats_;
  ORM::StatsList<ORM::ChannelInventoryStats> channel_inventory_stats_;
  ORM::StatsList<ORM::ChannelInventoryStats> zero_channel_inventory_stats_;
  ORM::StatsList<ORM::ChannelInventoryByCPMStats> channel_by_cpm_;
  ORM::StatsList<ORM::CCGUserStats> ccg_reach_;
  ORM::StatsList<ORM::ActionStats> action_stats_;
  ORM::StatsList<ORM::ActionRequests> action_requests_;

  // Utils
  void exec_cluster_command_(
    ClusterGroup group,
    ClusterCommand command);

  void check_click_and_actions_(
    unsigned long index,
    ClusterGroup group);

  void
  check_balancing_(
    ClusterGroup group,
    const char* cc_name);

  void
  check_user_profiling_(
    ClusterGroup group,
    const char* cc_name);

  void
  check_channels_(
    ClusterGroup group,
    const char* cc_name,
    bool no_db = false);

  CCGSite*
  campaign_update_(
    ClusterGroup group,
    const char* ccg_name,
    const char* cc_name,
    bool no_db = false);

  void
  check_channel_search_(
    ClusterGroup group);

  void
  init_stats_();

  // Test cases
  void
  precondition_(
    std::string& uid,
    std::ostream& user_profile,
    std::ostream& inventory_profile);

  void
  fe1_stop_();

  void
  lp_stop_();

  void
  be_stop_();

  void
  be_start_();

  void
  fe1_start_();

  void
  fe2_stop_();

  void
  lp_start_(
    const std::string& uid,
    const std::string& user_profile,
    const std::string& inventory_profile);

  void
  fe2_start_();

  void
  check_stats_();

};

#endif // _AUTOTEST__ZERODOWNTIMETEST_
