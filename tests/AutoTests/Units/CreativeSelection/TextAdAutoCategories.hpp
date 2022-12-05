#ifndef _UNITTEST__TEXTADAUTOCATEGORIES_
#define _UNITTEST__TEXTADAUTOCATEGORIES_
 
#include <tests/AutoTests/Commons/Common.hpp> 
 
class TextAdAutoCategories: public BaseUnit
{
public:
 
  TextAdAutoCategories(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~TextAdAutoCategories() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif
