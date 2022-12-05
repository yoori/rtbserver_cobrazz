
#ifndef _AUTOTEST__UNEXISTINGTOKENINCREATIVEOPTION_
#define _AUTOTEST__UNEXISTINGTOKENINCREATIVEOPTION_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class UnexistingTokenInCreativeOption:   public BaseDBUnit
{
public:
 
  UnexistingTokenInCreativeOption(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {}
 
  virtual ~UnexistingTokenInCreativeOption() noexcept
  {}
 
private:

  void set_up    ();
  void tear_down ();
  bool run();

  // Utils
  void set_creative_option();
 
};

#endif //_AUTOTEST__UNEXISTINGTOKENINCREATIVEOPTION_
