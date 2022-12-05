#ifndef AUTOTESTS_UNITS_CHANNELTRIGGERPERFORMANCETEST_HPP
#define AUTOTESTS_UNITS_CHANNELTRIGGERPERFORMANCETEST_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;
namespace DBC = ::AutoTest::DBC;

class ChannelTriggerPerformanceTest: public BaseUnit
{
  typedef ORM::ChannelTriggerStats Stat;
  typedef ORM::StatsList<Stat> Stats;
  typedef Stat::Diffs Diff;
  typedef std::list<Diff> Diffs;

  struct StatsRow
  {
    AutoTest::Time sdate;
    unsigned long colo_id;
    const char* channel_id;
    const char* trigger_type;
    const char* channel_trigger_id;
    double imps;
    double clicks;
  };

public:
  ChannelTriggerPerformanceTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseUnit(stat_var, task_name, params_var),
      conn_(open_pq()),
      today_(),
      default_colo_(fetch_int("DefaultColo")),
      default_tag_(fetch_int("TAG/468x60"))
  {};
 
  virtual ~ChannelTriggerPerformanceTest() noexcept
  {};

private:

  DBC::Conn conn_;
  AutoTest::Time today_;
  const unsigned long default_colo_;
  const unsigned long default_tag_;

  Stats stats_;
  Diffs diffs_;

  template<size_t Count>
  void add_stats(
    const std::string& prefix,
    const StatsRow (&stats)[Count],
    Stats& container,
    Diffs& diffs);

  void reason_of_impression_();
  void behavioral_params_restrictions_();
  void trigger_status_restrictions_();
  void channels_linked_with_text_campaign_();
  void last_visits_for_history_channels_();
  void trigger_performance_for_temp_users_();
  void asynchronous_logging_();

  bool run_test();

};

#endif /*AUTOTESTS_UNITS_CHANNELTRIGGERPERFORMANCETEST_HPP*/
