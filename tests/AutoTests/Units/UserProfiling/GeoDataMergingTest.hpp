#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class GeoDataMergingTest : public BaseUnit
{
public:
  GeoDataMergingTest(UnitStat& stat_var, const char* task_name, XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  { }

  virtual ~GeoDataMergingTest() noexcept
  { }

private:

  virtual bool run_test();

  void empty_muid_();
  void empty_profile_();
  void merge_with_cleanup_();
  void same_geodata_();
};
