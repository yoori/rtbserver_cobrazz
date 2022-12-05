
#ifndef _AUTOTEST__OUTOFSCOPETOKEN_
#define _AUTOTEST__OUTOFSCOPETOKEN_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class OutOfScopeToken: public BaseDBUnit
{
public:
 
  OutOfScopeToken(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {}
 
  virtual ~OutOfScopeToken() noexcept
  {}
 
private:
 
  void set_up    ();
  void tear_down ();
  bool run();

  // Utils
  void set_creative_option();
 
};

#endif //_AUTOTEST__OUTOFSCOPETOKEN_
