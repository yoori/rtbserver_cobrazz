
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class TestRequestNoDBModeTest: public BaseDBUnit
{
public:

  TestRequestNoDBModeTest(UnitStat& stat_var, const char* task_name, XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {};

  virtual ~TestRequestNoDBModeTest() noexcept
  {};

private:

  struct TestCase
  {
    std::string description;
    std::string prefix;
  };

  static const TestCase CASES[];

  void process_case(const TestCase&);
  bool run();
  void tear_down();

};
