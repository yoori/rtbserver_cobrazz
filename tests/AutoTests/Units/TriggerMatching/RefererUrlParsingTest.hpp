#ifndef _UNITTEST__REFERERURLPARSINGTEST_
#define _UNITTEST__REFERERURLPARSINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class RefererUrlParsingTest: public BaseUnit
{
public:
 
  RefererUrlParsingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~RefererUrlParsingTest() noexcept
  {};
  
private:
  virtual bool run_test();
 
};

#endif
