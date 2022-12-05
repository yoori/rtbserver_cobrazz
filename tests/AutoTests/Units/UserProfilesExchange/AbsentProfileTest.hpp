
#ifndef _AUTOTEST__ABSENTPROFILETEST_
#define _AUTOTEST__ABSENTPROFILETEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class AbsentProfileTest: public BaseUnit
{
public:
 
  AbsentProfileTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~AbsentProfileTest() noexcept
  {};
 
private:

  virtual bool run_test();
 
};

#endif //_AUTOTEST__ABSENTPROFILETEST_

