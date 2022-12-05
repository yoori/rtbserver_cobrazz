#ifndef _UNITTEST__SITECHANNELSTATSTEST_
#define _UNITTEST__SITECHANNELSTATSTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class SiteChannelStatsTest: public BaseUnit
{
public:
 
  SiteChannelStatsTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~SiteChannelStatsTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif
