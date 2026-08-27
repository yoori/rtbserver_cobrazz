
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class ExactUrlMatching: public BaseUnit
{
public:

  ExactUrlMatching(UnitStat& stat_var, const char* task_name, XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~ExactUrlMatching() noexcept
  {};

private:

  virtual bool run_test();

};
