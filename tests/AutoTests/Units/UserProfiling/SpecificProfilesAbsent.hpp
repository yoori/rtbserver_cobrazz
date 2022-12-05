#ifndef _AUTOTEST__SPECIFICPROFILESABSENT_
#define _AUTOTEST__SPECIFICPROFILESABSENT_
  
#include <tests/AutoTests/Commons/Common.hpp>

class SpecificProfilesAbsent : public BaseUnit
{
public:
  SpecificProfilesAbsent(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~SpecificProfilesAbsent() noexcept
  {};

private:

  virtual bool run_test();

  void case_for_non_opted_in_users();//1

};

#endif // _AUTOTEST__SPECIFICPROFILESABSENT_
