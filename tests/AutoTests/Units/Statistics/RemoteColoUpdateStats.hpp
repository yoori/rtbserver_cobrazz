
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class RemoteColoUpdateStats : public BaseUnit
{
public:
  RemoteColoUpdateStats(UnitStat& stat_var, const char* task_name, XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~RemoteColoUpdateStats() noexcept
  {};

private:

  virtual bool run_test();
};
