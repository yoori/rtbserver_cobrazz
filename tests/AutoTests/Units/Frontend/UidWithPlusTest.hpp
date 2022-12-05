#ifndef _UNITTEST__UIDWITHPLUSTEST_
#define _UNITTEST__UIDWITHPLUSTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
class UidWithPlusTest: public BaseUnit
{
public:
  UidWithPlusTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~UidWithPlusTest() noexcept
  {};
 
private:
  virtual bool run_test(); 
};

#endif
