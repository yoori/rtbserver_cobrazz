#ifndef _UNITTEST__TEXTANDDISPLAYCREATIVESCOMPETITIONTEST_
#define _UNITTEST__TEXTANDDISPLAYCREATIVESCOMPETITIONTEST_
 

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class TextAndDisplayCreativesCompetitionTest
 * @brief tests on text and display creatives competition
 */
class TextAndDisplayCreativesCompetitionTest: public BaseUnit
{
public:
 
  TextAndDisplayCreativesCompetitionTest(UnitStat& stat_var, 
                                         const char* task_name, 
                                         XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~TextAndDisplayCreativesCompetitionTest() noexcept
  {};
 
private:
 
  virtual bool run_test();
  void scenario1();
  void scenario2();
  void scenario3();
  void scenario4();
  void scenario5();
};

#endif
