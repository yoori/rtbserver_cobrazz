#ifndef _UNITTEST__METATEST_
#define _UNITTEST__METATEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class MetaTest: public BaseUnit
{
public:
 
  MetaTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~MetaTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif
