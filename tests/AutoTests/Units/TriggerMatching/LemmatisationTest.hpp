
#ifndef _AUTOTEST__LEMMATISATIONTEST_
#define _AUTOTEST__LEMMATISATIONTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>

class LemmatisationTest : public BaseUnit
{
public:
  LemmatisationTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~LemmatisationTest() noexcept
  {};

private:

  virtual bool run_test();
};

#endif // _AUTOTEST__LEMMATISATIONTEST_
