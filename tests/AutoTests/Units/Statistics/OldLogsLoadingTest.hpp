#ifndef _AUTOTEST__OLDLOGSLOADINGTEST_
#define _AUTOTEST__OLDLOGSLOADINGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class OldLogsLoadingTest: public BaseUnit
{
public:
 
  OldLogsLoadingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~OldLogsLoadingTest() noexcept
  { }
 
private:
 
  virtual bool run_test();
  
  void case_CreativeStat(
    AutoTest::DBC::Conn& conn,
    const std::string& login,
    int num);

};

#endif //_AUTOTEST__OLDLOGSLOADINGTEST_

