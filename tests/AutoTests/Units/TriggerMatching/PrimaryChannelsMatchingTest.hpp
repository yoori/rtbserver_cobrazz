
#ifndef __PRIMARYCHANNELSMATCHINGTEST_HPP
#define __PRIMARYCHANNELSMATCHINGTEST_HPP

#ifndef _AUTOTEST__PRIMARYCHANNELSMATCHINGTEST_
#define _AUTOTEST__PRIMARYCHANNELSMATCHINGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class PrimaryChannelsMatchingTest: public BaseUnit
{

  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest  NSLookupRequest;

public:
  struct TestCase
  {
    std::string description;
    const char* param1;
    const char* param2;
    const char* matched;
    const char* unmatched;
  };

public:
 
  PrimaryChannelsMatchingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~PrimaryChannelsMatchingTest() noexcept
  {};
 
private:
 
  virtual bool run_test();

  template<size_t Count>
  void
  test_group(
    const TestCase(&tests)[Count],
    NSLookupRequest::Member member1,
    NSLookupRequest::Member member2,
    unsigned long flags = 0);

  void
  test_case(
    AdClient& client,
    const TestCase& test,
    NSLookupRequest::Member member1,
    NSLookupRequest::Member member2,
    unsigned long flags);
 
};

#endif //_AUTOTEST__PRIMARYCHANNELSMATCHINGTEST_


#endif  // __PRIMARYCHANNELSMATCHINGTEST_HPP
