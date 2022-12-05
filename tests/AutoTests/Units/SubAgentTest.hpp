#ifndef _AUTOTEST__SUBAGENTTEST_
#define _AUTOTEST__SUBAGENTTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class SubAgentTest : public BaseUnit
{
public:
  SubAgentTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~SubAgentTest() noexcept
  {};

private:

  virtual bool run_test();
  void exec_cmd(const char* function);
};

#endif // _AUTOTEST__SUBAGENTTEST_
