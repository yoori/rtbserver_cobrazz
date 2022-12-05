#ifndef _AUTOTEST__PAUSINGCAMPAIGNREACHINGTEST_
#define _AUTOTEST__PAUSINGCAMPAIGNREACHINGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

/**
 * CampaignBudgetTest can be run only once for day or
 * after data refetching
 */
class CampaignBudgetTest: public BaseDBUnit
{
public:

  CampaignBudgetTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {}

  virtual ~CampaignBudgetTest() noexcept
  {}

private:

  Generics::Time base_time_;
  std::map<std::string, double> realized_budget;

  bool run();
  void post_condition();
  void tear_down();

  void fixed_daily_budget(bool initial = true);
  void dynamic_daily_budget(bool inital = true);

  void fixed_daily_budget_update();
  void dynamic_daily_budget_update();

  bool checker_call(
    const std::string& description,
    AutoTest::Checker* checker) /*throw(eh::Exception)*/;
};

#endif //_AUTOTEST__PAUSINGCAMPAIGNREACHINGTEST_
