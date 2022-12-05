#ifndef _UNITTEST__SOFTTRIGGERMATCHINGTEST_
#define _UNITTEST__SOFTTRIGGERMATCHINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class SoftTriggerMatchingTest: 
  public BaseUnit
{
public:
 
  SoftTriggerMatchingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~SoftTriggerMatchingTest() noexcept
  {};
 
private:

  virtual bool run_test();
 
};

#endif
