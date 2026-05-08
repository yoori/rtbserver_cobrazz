#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class ChannelTargetingTest: public BaseUnit
{
public:

  ChannelTargetingTest(
      UnitStat& stat_var,
      const char* task_name,
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~ChannelTargetingTest() noexcept
  {};

private:

  virtual bool run_test();
  void part1();
  void part2();
};
