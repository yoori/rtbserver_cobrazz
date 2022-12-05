#ifndef _AUTOTEST__COUNTRYTARGETING_
#define _AUTOTEST__COUNTRYTARGETING_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class CountryTargeting : public BaseUnit
{
public:
  CountryTargeting(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~CountryTargeting() noexcept
  {};

private:

  virtual bool run_test();
};

#endif // _AUTOTEST__COUNTRYTARGETING_
