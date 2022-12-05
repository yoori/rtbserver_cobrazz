
#ifndef _UNITTEST__DYNAMICTRIGGERLISTUPDATE_
#define _UNITTEST__DYNAMICTRIGGERLISTUPDATE_

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;

/**
 * @class ChannelsTestWithDB
 * @brief Test dynamic trigger list update
 */ 
class DynamicTriggerListUpdate:
  public BaseDBUnit
{
 
public:
 
  DynamicTriggerListUpdate(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var);
 
  virtual ~DynamicTriggerListUpdate() noexcept;
 
private:
  void set_up    ();
  void tear_down ();
  bool run();

  void change_trigger();
  void no_adv();
  void no_track();
};

#endif
