
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class DynamicCreativeTest : public BaseUnit
{
public:
  DynamicCreativeTest(UnitStat& stat_var, const char* task_name, XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~DynamicCreativeTest() noexcept
  {};

private:

  virtual bool run_test();

  void dcreatives_frontend();
  void token_substitution();

  std::string frontend;
};
