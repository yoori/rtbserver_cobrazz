
#ifndef _AUTOTEST__COMBINEDCHANNELSMATCHINGTEST_
#define _AUTOTEST__COMBINEDCHANNELSMATCHINGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class CombinedChannelsMatchingTest: public BaseUnit
{
public:
 
  CombinedChannelsMatchingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~CombinedChannelsMatchingTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif //_AUTOTEST__COMBINEDCHANNELSMATCHINGTEST_
