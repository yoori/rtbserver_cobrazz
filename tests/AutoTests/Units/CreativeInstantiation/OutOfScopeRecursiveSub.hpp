
#ifndef _AUTOTEST__OUTOFSCOPERECURSIVESUB_
#define _AUTOTEST__OUTOFSCOPERECURSIVESUB_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class OutOfScopeRecursiveSub: public BaseUnit
{
public:
 
  OutOfScopeRecursiveSub(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~OutOfScopeRecursiveSub() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif //_AUTOTEST__OUTOFSCOPERECURSIVESUB_
