#ifndef _UNITTEST__OPTOUTFREQCAPTEST_
#define _UNITTEST__OPTOUTFREQCAPTEST_

#include <tests/AutoTests/Commons/Common.hpp>



class OptOutFreqCapTest: public BaseUnit
{
public:
 
  OptOutFreqCapTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~OptOutFreqCapTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::OptOutRequest OptOutRequest;
  void  process_testcase(AdClient& test_client);
 
};

#endif
