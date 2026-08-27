
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>


class BrokenRequestTest: public BaseUnit
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
public:

  BrokenRequestTest(UnitStat& stat_var, const char* task_name, XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  { }

  virtual ~BrokenRequestTest() noexcept
  { }

private:

  virtual bool run_test();

  void test_case(
    const std::string& description,
    AdClient& client,
    const NSLookupRequest& request,
    const char* expected,
    const char* unexpected = 0);

};
