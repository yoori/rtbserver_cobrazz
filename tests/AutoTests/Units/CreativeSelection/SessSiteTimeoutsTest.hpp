#ifndef _UNITTEST__SESSSITETIMEOUTSTEST_
#define _UNITTEST__SESSSITETIMEOUTSTEST_

#include <tests/AutoTests/Commons/Common.hpp>

class SessSiteTimeoutsTest: public BaseUnit
{
public:
  struct Request;
  
public:
 
  SessSiteTimeoutsTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var)  {};
 
  virtual ~SessSiteTimeoutsTest() noexcept
  {};

private:


  virtual bool run_test();

  AutoTest::Time now;
  
  template <size_t COUNT>
  void testcase(
    const Request (&requests)[COUNT]);
  void merge_users();
  
  std::string no_track;
  std::string no_adv;
 
};

#endif // _UNITTEST__SESSSITETIMEOUTSTEST_
