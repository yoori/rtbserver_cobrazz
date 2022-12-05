#ifndef _UNITTEST__ACTIONGRANULARUPDATETEST_
#define _UNITTEST__ACTIONGRANULARUPDATETEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
namespace ORM = ::AutoTest::ORM;

class ActionGranularUpdateTest: public BaseDBUnit
{
public:
 
  ActionGranularUpdateTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~ActionGranularUpdateTest() noexcept
  {};

  void set_up    ();
  void tear_down ();
  bool run();
 
private:

  void add_action();
  void unlink_action();
  void action_for_inactive_ccg();
};

#endif
