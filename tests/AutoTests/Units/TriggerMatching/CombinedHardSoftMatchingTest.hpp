#ifndef _UNITTEST__COMBINEDHARDSOFTMATCHINGTEST_
#define _UNITTEST__COMBINEDHARDSOFTMATCHINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class CombinedHardSoftMatchingTest: 
  public BaseUnit
{
public:
 
  CombinedHardSoftMatchingTest(UnitStat& stat_var, 
                               const char* task_name, 
                               XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var),
    tid(fetch_string("Tags/Default"))
  {
  };

  virtual ~CombinedHardSoftMatchingTest() noexcept
  {};
 
private:
  std::string tid;

  virtual bool run_test();
 
};

#endif
