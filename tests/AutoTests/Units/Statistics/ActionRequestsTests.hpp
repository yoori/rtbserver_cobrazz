#ifndef _UNITTEST__ACTIONREQUESTSTESTS_
#define _UNITTEST__ACTIONREQUESTSTESTS_


#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class ActionRequestsTests
 * @brief Test logging to ActionRequests table.
 */ 
class ActionRequestsTests: public BaseUnit
{
public:
 
  ActionRequestsTests(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~ActionRequestsTests() noexcept
  {};
 
private:

  AutoTest::Time base_time;
 
  virtual bool run_test();

  void test_part1 (AutoTest::DBC::IConn& conn);
 
};

#endif
