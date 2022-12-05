#ifndef _UNITTEST__INVALIDURLPARAMTEST_
#define _UNITTEST__INVALIDURLPARAMTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class InvalidURLParamTest: public BaseUnit
{
public:
 
  InvalidURLParamTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~InvalidURLParamTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif
