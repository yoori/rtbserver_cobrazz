
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class LocalAudienceChannelsTest: public BaseUnit
{
public:
  LocalAudienceChannelsTest(UnitStat& stat_var, const char* task_name, XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~LocalAudienceChannelsTest() noexcept
  {};

private:



  bool run_test();

  void stage1();
  void stage2();

  // Cases
  void persistent_uid_matching_setup_();
  void persistent_uid_matching_();
};
