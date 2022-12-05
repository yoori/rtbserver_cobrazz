#ifndef _AUTOTEST__BANNEDCHADREQPROFDISABLING_
#define _AUTOTEST__BANNEDCHADREQPROFDISABLING_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class BannedChAdReqProfDisabling: public BaseUnit
{
public:
 
  BannedChAdReqProfDisabling(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~BannedChAdReqProfDisabling() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif //_AUTOTEST__BANNEDCHADREQPROFDISABLING_
