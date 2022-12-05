#ifndef _UNITTEST__INVALIDREDIRECTURLTEST_
#define _UNITTEST__INVALIDREDIRECTURLTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class InvalidRedirectURLTest: public BaseUnit
{
public:
 
  InvalidRedirectURLTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~InvalidRedirectURLTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif
