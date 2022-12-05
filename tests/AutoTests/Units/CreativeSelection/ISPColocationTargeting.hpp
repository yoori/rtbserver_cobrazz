
#ifndef _AUTOTEST__ISPCOLOCATIONTARGETING_
#define _AUTOTEST__ISPCOLOCATIONTARGETING_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class ISPColocationTargeting : public BaseUnit
{
  struct TestCase
  {
    const char* colo;
    const char* referer_kw;
    const char* expected_history;
    const char* expected_ccs;
  };
  
public:
  ISPColocationTargeting(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  { }

  virtual ~ISPColocationTargeting() noexcept
  { }

private:

  virtual bool run_test();

  template <size_t Count>
  void
  process_case(
    const std::string& description,
    const TestCase(&testcases)[Count]);
  
};

#endif // _AUTOTEST__ISPCOLOCATIONTARGETING_
