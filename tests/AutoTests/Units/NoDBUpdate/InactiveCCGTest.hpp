

#ifndef _AUTOTEST__INACTIVECCGTEST_
#define _AUTOTEST__INACTIVECCGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class InactiveCCGTest: public BaseUnit
{
public:
 
  InactiveCCGTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~InactiveCCGTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif //_AUTOTEST__INACTIVECCGTEST_
