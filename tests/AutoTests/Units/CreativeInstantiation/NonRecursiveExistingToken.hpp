#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class NonRecursiveExistingToken: public BaseUnit
{
public:

  NonRecursiveExistingToken(
      UnitStat& stat_var,
      const char* task_name,
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~NonRecursiveExistingToken() noexcept
  {};

private:

  virtual bool run_test();

};
