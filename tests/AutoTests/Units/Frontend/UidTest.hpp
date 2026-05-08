#pragma once

#include <tests/AutoTests/Commons/Common.hpp>

class UidTest: public BaseUnit
{
public:

  UidTest(
      UnitStat& stat_var,
      const char* task_name,
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~UidTest() noexcept
  {};

private:
  virtual bool run_test();
  void uid_installing();
  void probe_uid();

};
