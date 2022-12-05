
#ifndef __OPTOUTMATCHING_HPP
#define __OPTOUTMATCHING_HPP

#ifndef _AUTOTEST__OPTOUTMATCHING_
#define _AUTOTEST__OPTOUTMATCHING_
  
#include <tests/AutoTests/Commons/Common.hpp>

typedef AutoTest::AdClient AdClient;
typedef AutoTest::NSLookupRequest NSLookupRequest;  

class OptoutMatching : public BaseDBUnit
{

  typedef AutoTest::ORM::ChannelTriggerStats TriggerStat;
  typedef AutoTest::ORM::StatsArray<TriggerStat, 3> TriggerStats;
  typedef TriggerStat::Diffs TriggerDiff;

  typedef AutoTest::ORM::ChannelInventoryStats ChannelStat;
  typedef AutoTest::ORM::StatsArray<ChannelStat, 3> ChannelStats;
  typedef ChannelStat::Diffs ChannelDiff;

public:
  OptoutMatching(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var),
    conn_(open_pq())
  {};

  virtual ~OptoutMatching() noexcept
  {};

private:

  AutoTest::DBC::Conn conn_;

  TriggerStats trigger_stats_;
  ChannelStats channel_stats_;

  void set_up();
  void pre_condition();
  bool run();
  void post_condition();
  void tear_down();

  // Helpers
  void run_case(unsigned int i, unsigned int user, 
    AdClient& client, NSLookupRequest& request);

  void fill_expected(std::list<std::string>& list,
                     const char* namesuffix,
                     const char* names = 0);
};

#endif // _AUTOTEST__OPTOUTMATCHING_

#endif  // __OPTOUTMATCHING_HPP
