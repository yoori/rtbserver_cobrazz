#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class CountryTargeting : public BaseUnit
{
public:
  CountryTargeting(UnitStat& stat_var, const char* task_name, XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~CountryTargeting() noexcept
  {};

private:

  virtual bool run_test();
};
