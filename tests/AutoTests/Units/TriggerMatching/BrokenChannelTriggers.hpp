
#ifndef _AUTOTEST__BROKENCHANNELTRIGGERS_
#define _AUTOTEST__BROKENCHANNELTRIGGERS_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class BrokenChannelTriggers: public BaseUnit
{
public:

  typedef AutoTest::AdClient AdClient;
  
  struct TestCase
  {
    const std::string description;
    const char* referer_kw;
    const char* referer;
    const char* matched;
    const char* unmatched;
  };
  
public:
 
  BrokenChannelTriggers(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~BrokenChannelTriggers() noexcept
  {};
 
private:
 
  virtual bool run_test();
  
  template<size_t Count>
  void test_group(const TestCase(&tests)[Count]);
  
  void test_case(
    AdClient& client,
    const TestCase& test);
  
};

#endif //_AUTOTEST__BROKENCHANNELTRIGGERS_
