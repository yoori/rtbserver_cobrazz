#ifndef _UNITTEST__TEXTADVERTISINGTEST_
#define _UNITTEST__TEXTADVERTISINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
class TextAdvertisingTest: public BaseUnit
{
public:
 
  TextAdvertisingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~TextAdvertisingTest() noexcept
  {};
 
private:
 
  void 
  generic_scenario(const AutoTest::NSLookupRequest &request,
                   const std::vector<std::string> &ccid_exp,
                   const std::vector<AutoTest::Money> &actual_cpc_exp);

  virtual bool run_test();
 
};

#endif
