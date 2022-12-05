#ifndef AUTOTESTS_UNITS_CHANNELTRIGGERIMPSTATSTEST_HPP
#define AUTOTESTS_UNITS_CHANNELTRIGGERIMPSTATSTEST_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;
namespace DBC = ::AutoTest::DBC;

typedef AutoTest::ORM::ChannelTriggerStats ChannelTriggerStats;
typedef ORM::StatsList<ChannelTriggerStats> ChannelTriggerStatsArray;
typedef std::map<unsigned long, unsigned long> TriggerCounterMap;
typedef ORM::StatsArray<ChannelTriggerStats, 1> ChannelTriggerSumStats;
 
class ChannelTriggerImpStatsTest: public BaseDBUnit
{
public:
  ChannelTriggerImpStatsTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~ChannelTriggerImpStatsTest() noexcept
  { }


protected:

  virtual bool run();

  virtual void set_up();
  
  virtual void tear_down();

private:
  AutoTest::Time time_;

  void fetch_stats_(
    ChannelTriggerStatsArray& triggers_stats,
    TriggerCounterMap& imps_done,
    ChannelTriggerSumStats& sum_stats,
    const char* entity_prefix,
    unsigned long imps_count);

  void do_requests_for_one_trigger_case_(
    const char* entity_prefix,
    unsigned long imps_count,
    unsigned long clicks_count);

  void do_requests_for_one_trigger_tempo_case_(
    const char* entity_prefix,
    unsigned long imps_count,
    unsigned long clicks_count);

  void channel_types();
};

#endif /*AUTOTESTS_UNITS_CHANNELTRIGGERIMPSTATSTEST_HPP*/
