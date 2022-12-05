#ifndef _AUTOTEST__DISPLAYADNETANDGROSSTEST_
#define _AUTOTEST__DISPLAYADNETANDGROSSTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
namespace ORM = AutoTest::ORM;

/**
 * @class DisplayAdNetAndGrossTest
 * @brief test for creative selection in case Gross/Net accounts
 */
class DisplayAdNetAndGrossTest: public BaseDBUnit
{
public:
 
  DisplayAdNetAndGrossTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~DisplayAdNetAndGrossTest() noexcept
  {};
 
private:

  typedef ORM::StatsArray<ORM::HourlyStats, 5> Stats;

  virtual void pre_condition();
  virtual bool run();
  virtual void post_condition();
  virtual void tear_down();

  void case_net_campaign_win();
  void case_gross_campaign_win();
  void case_net_campaign_win_with_commission();
  void case_publisher_commission();

  Stats stats_;
  Generics::Time target_request_time_;
};

#endif //_AUTOTEST__DISPLAYADNETANDGROSSTEST_
