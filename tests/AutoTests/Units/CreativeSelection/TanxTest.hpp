
#ifndef _AUTOTEST__TANXTEST_
#define _AUTOTEST__TANXTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class TanxTest : public BaseUnit
{
  
public:
  TanxTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  { }

  virtual ~TanxTest() noexcept
  { }

private:

  bool run_test();

  // Cases
  void base_();
};

#endif // _AUTOTEST__TANXTEST_
