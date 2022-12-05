#ifndef _UNITTEST__BADREFERERSPROCESSINGTEST_
#define _UNITTEST__BADREFERERSPROCESSINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class BadReferersProcessingTest: 
  public BaseUnit
{
public:
 
  BadReferersProcessingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~BadReferersProcessingTest() noexcept
  {};
 
private:

  virtual bool run_test();
 
};

#endif
