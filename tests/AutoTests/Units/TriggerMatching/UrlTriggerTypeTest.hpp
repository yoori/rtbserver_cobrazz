
#ifndef _AUTOTEST__URLTRIGGERTYPETEST_
#define _AUTOTEST__URLTRIGGERTYPETEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class UrlTriggerTypeTest: public BaseUnit
{
public:
 
  UrlTriggerTypeTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~UrlTriggerTypeTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif //_AUTOTEST__URLTRIGGERTYPETEST_
