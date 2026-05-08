
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class UrlTriggerTypeTest: public BaseUnit
{
public:

  UrlTriggerTypeTest(
      UnitStat& stat_var,
      const char* task_name,
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~UrlTriggerTypeTest() noexcept
  {};

private:

  virtual bool run_test();

};
