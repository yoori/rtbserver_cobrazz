#ifndef _UNITTEST__LONGHEADERAPACHESTABILITYTEST_
#define _UNITTEST__LONGHEADERAPACHESTABILITYTEST_

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class LongHeaderApacheStabilityTest
 * @brief test server stability on long headers.
 */
class LongHeaderApacheStabilityTest:
  public BaseUnit
{
public:

  LongHeaderApacheStabilityTest(
      UnitStat& stat_var,
      const char* task_name,
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~LongHeaderApacheStabilityTest() noexcept
  {};

private:

  virtual bool run_test();

};

#endif
