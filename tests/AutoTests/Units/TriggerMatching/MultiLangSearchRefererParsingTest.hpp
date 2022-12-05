#ifndef _UNITTEST__MULTILANGSEARCHREFERERPARSINGTEST_
#define _UNITTEST__MULTILANGSEARCHREFERERPARSINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
class MultiLangSearchRefererParsingTest: 
  public BaseUnit
{
public:
 
  MultiLangSearchRefererParsingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var),
    tid(fetch_string("Tags/Default"))
  {};

  virtual ~MultiLangSearchRefererParsingTest() noexcept
  {};
 
private:
  std::string tid;
 
  virtual bool run_test();
 
};

#endif
