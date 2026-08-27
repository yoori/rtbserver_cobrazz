#pragma once

#include <tests/AutoTests/Commons/Common.hpp>

/*
 * ChannelServer load state constraints checking:
 *   check that loaded inremote & central clusters triggers is equal
 */
class RemoteTriggersCheck: public BaseUnit
{
public:
  RemoteTriggersCheck(UnitStat& stat_var, const char* task_name, XsdParams params_var)
    : BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~RemoteTriggersCheck() noexcept
  {};

private:
  virtual bool run_test();
};
