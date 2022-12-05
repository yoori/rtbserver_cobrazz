
#ifndef _AUTOTEST__SITECREATIVEAPPROVALTEST_
#define _AUTOTEST__SITECREATIVEAPPROVALTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class SiteCreativeApprovalTest : public BaseUnit
{
public:
  SiteCreativeApprovalTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  { }

  virtual ~SiteCreativeApprovalTest() noexcept
  { }

private:

  virtual bool run_test();
};

#endif // _AUTOTEST__SITECREATIVEAPPROVALTEST_
