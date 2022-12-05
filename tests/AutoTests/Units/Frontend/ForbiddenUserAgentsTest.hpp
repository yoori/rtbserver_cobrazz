#ifndef _UNITTEST__FORBIDDENUSERAGENTSTEST_
#define _UNITTEST__FORBIDDENUSERAGENTSTEST_

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class ForbiddenUserAgentsTest
 * @brief Test for forbidden user agents
 */ 
class ForbiddenUserAgentsTest: public BaseUnit
{
public:
 
  ForbiddenUserAgentsTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~ForbiddenUserAgentsTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif
