
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class ChangingCreativeWeightsTest: public BaseDBUnit
{
public:
  ChangingCreativeWeightsTest(UnitStat& stat_var, const char* task_name, XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {};

  virtual ~ChangingCreativeWeightsTest() noexcept
  {};

private:
  bool run();
  void tear_down();

};
