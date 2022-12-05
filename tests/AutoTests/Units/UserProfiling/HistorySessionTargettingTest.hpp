#ifndef _UNITTEST__HISTORYSESSIONTARGETTINGTEST_
#define _UNITTEST__HISTORYSESSIONTARGETTINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
class HistorySessionTargettingTest:
  public BaseUnit
{
public:

  struct ExpectedType
  {
    const char* triggers;
    const char* history;    
  };

  struct RequestType
  {
    long time_ofset;
    const char* referer_kw;
    ExpectedType expected;
    ExpectedType unexpected;
    bool log_request;
  };
  
public:
 
  HistorySessionTargettingTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var),
    base_time(
      (AutoTest::Time().get_gm_time().format("%d-%m-%Y") +
        ":" + "12-00-00").c_str())
  {};

  virtual ~HistorySessionTargettingTest() noexcept
  {};
   
private:

  AutoTest::Time base_time;

  virtual bool run_test();

  void check(
    const RequestType* requests,
    size_t requests_size,
    const AutoTest::Time& base_time);

};

#endif
