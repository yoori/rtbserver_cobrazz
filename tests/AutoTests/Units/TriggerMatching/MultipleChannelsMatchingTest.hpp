#ifndef _UNITTEST__MULTIPLECHANNELSMATCHINGTEST_
#define _UNITTEST__MULTIPLECHANNELSMATCHINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>

 
class MultipleChannelsMatchingTest:
  public BaseUnit
{
public:
 
  MultipleChannelsMatchingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var),
    tid(fetch_string("Tags/Default"))
  {};

  virtual ~MultipleChannelsMatchingTest() noexcept
  {};
 
private:
  std::string tid;

  virtual bool run_test();
 
};

#endif
