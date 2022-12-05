#ifndef _UNITTEST__COUNTRYTEST_
#define _UNITTEST__COUNTRYTEST_

#include <tests/AutoTests/Commons/Common.hpp>

typedef AutoTest::AdClient AdClient;

class CountryTest: public BaseUnit
{
public:
 
  CountryTest(
              UnitStat& stat_var, 
              const char* task_name, 
              XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~CountryTest() noexcept
  {};
 
private:
 
  virtual bool run_test();

  AutoTest::NSLookupRequest request;

  void process_testcase(AdClient& test_client,
                        const char *cc_id);
};

#endif
