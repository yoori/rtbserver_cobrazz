#ifndef _UNITTEST__ACCOUNTCURRENCYTEST_
#define _UNITTEST__ACCOUNTCURRENCYTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class AccountCurrencyTest: public BaseUnit
{
public:
 
  AccountCurrencyTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~AccountCurrencyTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif
