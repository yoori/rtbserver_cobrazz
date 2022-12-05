
#ifndef _AUTOTEST__DIFFCOLOTZTEST_
#define _AUTOTEST__DIFFCOLOTZTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class DiffColoTZTest: public BaseUnit
{
public:
 
  DiffColoTZTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~DiffColoTZTest() noexcept
  {};
 
private:

  double tz_ofset;
  int   colo_req_timeout;
  std::string remote1;
  std::string remote2;
  std::string colo1_id;
  std::string colo2_id;
  Generics::Time today;
  virtual bool run_test();

  void local_day_switch();
  void gmt_day_switch();
 
};

#endif //_AUTOTEST__DIFFCOLOTZTEST_
