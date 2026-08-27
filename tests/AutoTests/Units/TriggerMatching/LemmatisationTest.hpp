
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>

class LemmatisationTest : public BaseUnit
{
public:
  LemmatisationTest(UnitStat& stat_var, const char* task_name, XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~LemmatisationTest() noexcept
  {};

private:

  virtual bool run_test();
};
