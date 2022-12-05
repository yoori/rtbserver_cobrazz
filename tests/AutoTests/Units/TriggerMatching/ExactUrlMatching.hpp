
#ifndef _AUTOTEST__EXACTURLMATCHING_
#define _AUTOTEST__EXACTURLMATCHING_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class ExactUrlMatching: public BaseUnit
{
public:
 
  ExactUrlMatching(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~ExactUrlMatching() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif //_AUTOTEST__EXACTURLMATCHING_

