
#ifndef _AUTOTEST__SESSIONEXPIRATION_
#define _AUTOTEST__SESSIONEXPIRATION_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class SessionExpiration : public BaseUnit
{
public:
  SessionExpiration(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~SessionExpiration() noexcept
  {};

private:

  virtual bool run_test();
};

#endif // _AUTOTEST__SESSIONEXPIRATION_

