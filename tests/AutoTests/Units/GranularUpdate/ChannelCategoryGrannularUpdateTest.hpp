#ifndef _AUTOTEST__HANNELCATEGORYGRANNULARUPDATETEST_
#define _AUTOTEST__HANNELCATEGORYGRANNULARUPDATETEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class ChannelCategoryGrannularUpdateTest: public BaseDBUnit
{
public:
 
  ChannelCategoryGrannularUpdateTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~ChannelCategoryGrannularUpdateTest() noexcept
  {};
 
private:

  virtual bool run();
  void tear_down();
  void set_up();

  void add_category();
  void deactivate_category();
 
};

#endif //_AUTOTEST__HANNELCATEGORYGRANNULARUPDATETEST_
