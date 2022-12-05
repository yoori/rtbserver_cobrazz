
#ifndef _AUTOTEST__URLNORMALIZATIONTEST_
#define _AUTOTEST__URLNORMALIZATIONTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
class UrlNormalizationTest: public BaseDBUnit
{

  typedef AutoTest::ORM::ChannelTriggerStats TriggerStat;
  typedef AutoTest::ORM::StatsList<TriggerStat> TriggerStats;
  typedef TriggerStat::Diffs TriggerDiff;
  typedef std::list<TriggerDiff> TriggerDiffs;

  typedef AutoTest::ORM::ChannelInventoryStats ChannelStat;
  typedef AutoTest::ORM::StatsList<ChannelStat> ChannelStats;
  typedef ChannelStat::Diffs ChannelDiff;
  typedef std::list<ChannelDiff> ChannelDiffs;

public:
  UrlNormalizationTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var),
      conn_(open_pq())
  {};
 
  virtual ~UrlNormalizationTest() noexcept
  {};
 
private:

  AutoTest::DBC::Conn conn_;

  TriggerStats trigger_stats_;
  TriggerDiffs trigger_diffs_;

  ChannelStats channel_stats_;
  ChannelDiffs channel_diffs_;

  void set_up();
  void pre_condition();
  bool run();
  void post_condition();
  void tear_down();
};

#endif //_AUTOTEST__URLNORMALIZATIONTEST_
