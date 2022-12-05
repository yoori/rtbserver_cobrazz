#ifndef __TARGETINGCHANNELSTATS_HPP
#define __TARGETINGCHANNELSTATS_HPP
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;

class TargetingChannelStats : public BaseDBUnit
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::DBC::Conn Connection;

  struct ExpectedStats
  {
    int imps;
    int clicks;
    int actions;
    double revenue;
    int no_imp_other;
    int no_imp_no_imp;
    int no_imp_users;
    int imp_imp_other;
    ORM::base_stats_diff_type imp_imp_other_value;
    int imp_no_imp;
    int imp_no_imp_users;
  };

  struct StatCollection
  {
    ORM::StatsList<ORM::ChannelInventoryStats> inv;
    ORM::StatsList<ORM::ChannelImpInventory> imp_inv_disp;
    std::list<ORM::ChannelImpInventory::Diffs> imp_inv_disp_diffs;
    ORM::StatsList<ORM::ChannelImpInventory> imp_inv_text;
    std::list<ORM::ChannelImpInventory::Diffs> imp_inv_text_diffs;
    ORM::StatsList<ORM::ChannelInventoryByCPMStats> inv_by_cpm;
    ORM::StatsList<ORM::ChannelPerformance> perf;
    std::list<ORM::ChannelPerformance::Diffs> perf_diffs;
    ORM::StatsList<ORM::ChannelUsageStats> usage;
    std::list<ORM::ChannelUsageStats::Diffs> usage_diffs;
    ORM::StatsList<ORM::ExpressionPerformanceStats> expr_perf;
    std::list<ORM::ExpressionPerformanceStats::Diffs> expr_perf_diffs;
    ORM::StatsList<ORM::ExpressionPerformanceStats> expr_perf_zero;
    ORM::StatsList<ORM::SiteChannelStats> site_channel;
    std::list<ORM::SiteChannelStats::Diffs> site_channel_diffs;
    std::list<std::string> users;
  };
  
public:
  
  TargetingChannelStats(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~TargetingChannelStats() noexcept
  { }

  void
  log_profile(
    std::string uid);

protected:

  virtual bool run();
  
  virtual void set_up();
  
  virtual void tear_down();

private:

  bool
  check_channels_(
    const std::string& expected,
    AutoTest::DebugInfo::StringListValue& got);

  void
  set_req_param_(
    NSLookupRequest& request,
    NSLookupRequest::Member member,
    const std::string& name);

  void process_case_(
    const std::string& prefix);

  template <typename Stat>
  void
  add_stats_(
    AutoTest::DBC::IConn& connection,
    const typename Stat::Key& stat,
    ORM::StatsList<Stat>& stats);

  void
  initialize_stats_(
    StatCollection& stats,
    unsigned long channel_id,
    const std::string& expression,
    unsigned long cc_id,
    const ExpectedStats& expected,
    int text_size = 0);

  void
  add_checkers_(
    StatCollection& stats);

  void
  case_all_();
  
  AutoTest::Time now_;

};

#endif  // __TARGETINGCHANNELSTATS_HPP
