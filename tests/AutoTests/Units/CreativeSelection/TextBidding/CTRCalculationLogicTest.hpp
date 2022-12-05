#ifndef _UNITTEST__CTRCALCULATIONLOGICTEST_
#define _UNITTEST__CTRCALCULATIONLOGICTEST_

#include <tests/AutoTests/Commons/Common.hpp>


class CTRCalculationLogicTest: public BaseDBUnit
{
public:
 
  CTRCalculationLogicTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {};
 
private:

  void set_up();
  void tear_down();
  bool run();

  // Test cases
  void initial_case();
  void base_case();
};

#endif // _UNITTEST__CTRCALCULATIONLOGICTEST_

