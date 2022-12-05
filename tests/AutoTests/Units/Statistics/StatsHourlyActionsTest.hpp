#ifndef _AUTOTEST__STATSHOURLYACTIONSTEST_
#define _AUTOTEST__STATSHOURLYACTIONSTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  
namespace ORM = ::AutoTest::ORM;

class StatsHourlyActionsTest : public BaseDBUnit
{
  typedef ORM::HourlyStats HourlyStats;
  
public:
  StatsHourlyActionsTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(
      stat_var, task_name, params_var),
    target_request_time_(
      AutoTest::Time().get_gm_time().
        format("%d-%m-%Y:%H-00-00"))
  { }

  virtual ~StatsHourlyActionsTest() noexcept
  { }

protected:

  virtual void set_up();
  virtual bool run();
  virtual void tear_down();

private:

  void case_base_functionality();//1
  void case_action_from_different_campaign(); //2.1
  void case_triple_action();//2.2
  void case_action_before_click();//3
  void case_action_before_impression_confirmation();//4
  void case_one_action_for_multiple_creatives_in_campaign();//5
  void case_action_for_display_creative_group_with_cpc_rate();//6
  void case_actions_for_text_ad_group();//7

  AutoTest::Time target_request_time_;

};

#endif // _AUTOTEST__STATSHOURLYACTIONSTEST_
