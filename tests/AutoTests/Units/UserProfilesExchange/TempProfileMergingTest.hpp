#ifndef _AUTOTEST__TEMPPROFILEMERGINGTEST_
#define _AUTOTEST__TEMPPROFILEMERGINGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class TempProfileMergingTest: public BaseUnit
{
public:
 
  TempProfileMergingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~TempProfileMergingTest() noexcept
  {};
 
private:

  int   colo_req_timeout;
  std::string remote1;
  std::string remote2;
  std::string colo1_id;
  std::string colo2_id;

  virtual bool run_test();

  void merge_on_colo_change();
  void merge_before_get_profile();
 
};

#endif //_AUTOTEST__TEMPPROFILEMERGINGTEST_
