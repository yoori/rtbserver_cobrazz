#ifndef _UNITTEST__TEXTDISPLAYADVERTISINGWATERSHEDTEST_
#define _UNITTEST__TEXTDISPLAYADVERTISINGWATERSHEDTEST_
 

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class TextDisplayAdvertisingWatershedTest
 * @brief Test display advertising versus text advertising.
 */
 
class TextDisplayAdvertisingWatershedTest
  :public BaseUnit
{
public:
 
  TextDisplayAdvertisingWatershedTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~TextDisplayAdvertisingWatershedTest() noexcept
  {};
 
private:
 
  virtual bool run_test();

  // Cases
  void
  display_wins_();

  void
  text_wins_();
 
};

#endif
