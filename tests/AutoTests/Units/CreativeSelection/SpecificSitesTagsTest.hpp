#ifndef _UNITTEST__SPECIFICSITESTAGSTEST_
#define _UNITTEST__SPECIFICSITESTAGSTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
class SpecificSitesTagsTest: public BaseUnit
{
public:
 
  SpecificSitesTagsTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var)  {};
 
  virtual ~SpecificSitesTagsTest() noexcept
  {};

  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;

  void case1();
  void case2();
  void case3();
  void case4();

private:
 
  virtual bool run_test();
 
};

#endif
