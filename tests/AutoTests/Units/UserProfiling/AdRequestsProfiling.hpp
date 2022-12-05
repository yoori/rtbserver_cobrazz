
#ifndef _AUTOTEST__ADREQUESTSPROFILING_
#define _AUTOTEST__ADREQUESTSPROFILING_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class AdRequestsProfiling : public BaseUnit
{
public:
  AdRequestsProfiling(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~AdRequestsProfiling() noexcept
  {};

private:

  virtual bool run_test();

  // Test cases
  void basic_case();
  void history_optimization();
  void visit_on_adrequest();
  void no_visit_on_adrequest();
  void full_text_mode();
  void merging();
};

#endif // _AUTOTEST__ADREQUESTSPROFILING_
