
#ifndef _AUTOTEST__REMOTECOLOUPDATESTATS_
#define _AUTOTEST__REMOTECOLOUPDATESTATS_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class RemoteColoUpdateStats : public BaseUnit
{
public:
  RemoteColoUpdateStats(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~RemoteColoUpdateStats() noexcept
  {};

private:

  virtual bool run_test();
};

#endif // _AUTOTEST__REMOTECOLOUPDATESTATS_

