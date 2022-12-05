#ifndef _AUTOTEST__REACHINGZEROBALANCETEST_
#define _AUTOTEST__REACHINGZEROBALANCETEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class AccountBudgetTest: public BaseUnit
{
public:
 
  AccountBudgetTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~AccountBudgetTest() noexcept
  {};
 
private:
  virtual bool run_test();

};

#endif //_AUTOTEST__REACHINGZEROBALANCETEST_

