#pragma once

#include <tests/AutoTests/Commons/Common.hpp>

class ChannelSearchAdminTest:
  public BaseUnit
{
public:

  struct TestCase
  {
    const char* phrase;
    const char* expected_channels;
  };

public:

  ChannelSearchAdminTest(UnitStat& stat_var, const char* task_name, XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)  {};

  virtual ~ChannelSearchAdminTest() noexcept
  {};

private:

  virtual bool run_test();

  void test_case(unsigned int index, const TestCase& test);
};
