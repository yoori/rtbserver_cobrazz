#ifndef _UNITTEST__KEYWORDCLICKURLTEST_
#define _UNITTEST__KEYWORDCLICKURLTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
/**
 * @class KeywordClickUrlTest
 * @brief Test for keyword click_url vs creative click_url behaviour
 */
class KeywordClickUrlTest: public BaseUnit
{
public:
 
  KeywordClickUrlTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~KeywordClickUrlTest() noexcept
  {};
 
private:

  std::string tid;
 
  virtual bool run_test();
  void part1 ();
  void part2 ();
  void part3 ();
  void part4 ();
};

#endif
