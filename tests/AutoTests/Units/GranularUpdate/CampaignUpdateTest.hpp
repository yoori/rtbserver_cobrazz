#ifndef AUTOTESTS_UNITS_GRANULARUPDATE_CAMPAIGNUPDATETEST
#define AUTOTESTS_UNITS_GRANULARUPDATE_CAMPAIGNUPDATETEST

#include <tests/AutoTests/Commons/Common.hpp>
 
namespace ORM = ::AutoTest::ORM;

/**
 * @class CampaignUpdateTest
 * @brief Test for campaign & creative updates
 */
class CampaignUpdateTest:
  public BaseDBUnit
{

public:
  CampaignUpdateTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~CampaignUpdateTest() noexcept
  {};

private:
  void set_up();
  void tear_down();
  bool run();

  void add_campaign_case_();
  void currency_change_case_();
  void update_channel_case_();
  void change_status_case_();
  void change_date_interval_case_();
  void change_max_pub_share_case_();
};

#endif /*AUTOTESTS_UNITS_GRANULARUPDATE_CAMPAIGNUPDATETEST*/
