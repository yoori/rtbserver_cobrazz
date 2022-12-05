#ifndef _AUTOTEST__VISPARAMTEST_
#define _AUTOTEST__VISPARAMTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class VISParamTest : public BaseUnit
{
public:
  VISParamTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~VISParamTest() noexcept
  {};

private:

  virtual bool run_test();

  void case01_vis_param_tests();
  void case02_ad_selection_based_on_visibility_filter();
  void case03_publisher_inventory_mode();

};

#endif // _AUTOTEST__VISPARAMTEST_
