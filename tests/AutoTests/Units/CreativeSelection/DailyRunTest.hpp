#ifndef _UNITTEST__DAILYRUNTEST_
#define _UNITTEST__DAILYRUNTEST_

#include <tests/AutoTests/Commons/Common.hpp>

 
class DailyRunTest: public BaseUnit
{

public:

  enum WeekDays
  {
    WD_MONDAY  = 0,
    WD_TUESDAY = 1,
    WD_WEDNESDAY = 2,
    WD_THURSDAY = 3,
    WD_FRIDAY = 4,
    WD_SATURDAY = 5,
    WD_SUNDAY = 6,
    WD_CURRENT = 7
  };
  
  struct RequestTimeResult
  {
    WeekDays wday;
    const char *time;
    bool have_cc;
  };
  
public:
 
  DailyRunTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~DailyRunTest() noexcept
  {};
 
private:
 
  virtual bool run_test();

  void process_testcase(
    const char *keyword,
    const char *tz_name,
    const RequestTimeResult expect[],
    size_t expect_count);

  Generics::Time
  get_gmt(
    DailyRunTest::WeekDays wday,
    const char* local_tm,
    const char* time_zone);

};

#endif
