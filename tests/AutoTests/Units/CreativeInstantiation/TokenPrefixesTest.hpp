#ifndef _UNITTEST__TOKENPREFIXESTEST_
#define _UNITTEST__TOKENPREFIXESTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

class TokenPrefixesTest: public BaseUnit
{
  typedef AutoTest::AdClient AdClient;
  
public:
 
  TokenPrefixesTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~TokenPrefixesTest() noexcept
  {};
 
private:
  
  virtual bool run_test();
  void checkBody(
    const AdClient& client,
    const char* exp_body_tag);
  void tokenPrefixesSimpleFileCase();
 
};

#endif
