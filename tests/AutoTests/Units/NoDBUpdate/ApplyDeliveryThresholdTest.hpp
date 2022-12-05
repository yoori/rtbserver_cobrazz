
#ifndef _AUTOTEST__APPLYDELIVERYTHRESHOLDTEST_
#define _AUTOTEST__APPLYDELIVERYTHRESHOLDTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class ApplyDeliveryThresholdTest: public BaseUnit
{
public:
 
  ApplyDeliveryThresholdTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~ApplyDeliveryThresholdTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif //_AUTOTEST__APPLYDELIVERYTHRESHOLDTEST_
