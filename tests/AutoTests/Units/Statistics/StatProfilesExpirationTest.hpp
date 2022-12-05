
#ifndef _AUTOTEST__STATPROFILESEXPIRATIONTEST_
#define _AUTOTEST__STATPROFILESEXPIRATIONTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

class StatProfilesExpirationTest: public BaseDBUnit
{
public:
  StatProfilesExpirationTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {};

  virtual ~StatProfilesExpirationTest() noexcept
  {};

private:
  void set_up();
  void tear_down();
  bool run();
};

#endif //_AUTOTEST__STATPROFILESEXPIRATIONTEST_
