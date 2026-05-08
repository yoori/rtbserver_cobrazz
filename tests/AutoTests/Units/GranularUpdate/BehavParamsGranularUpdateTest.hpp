#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class BehavParamsGranularUpdateTest : public BaseDBUnit
{
public:
  BehavParamsGranularUpdateTest(UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {}

  virtual ~BehavParamsGranularUpdateTest() noexcept
  {}

private:

  bool run();
  void tear_down() {}

  void empty();
  void assign();
  void change();
  void remove();
  void add_list();
  void del_param();

};
