#ifndef _UNITTEST__REFERERMATCHINGTEST_
#define _UNITTEST__REFERERMATCHINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>

 
class RefererMatchingTest: 
  public BaseUnit
{
public:
 
  RefererMatchingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~RefererMatchingTest() noexcept
  {};
 
private:

  virtual bool run_test();
 
};

#endif
