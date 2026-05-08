/* $Id$
* @file SiteUpdateTest.hpp
* Test that server dynamically update Site.
* For more info see https://confluence.ocslab.com/display/ADS/SiteUpdateTest
*/
#pragma once

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class SiteUpdateTest
 * @brief Test for site updates
 */
class SiteUpdateTest: public BaseDBUnit
{
public:
  SiteUpdateTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~SiteUpdateTest() noexcept
  { }

private:

  AutoTest::Time base_time_;

  void set_up();
  void tear_down();
  bool run();

  void create_site_();
  void update_site_campaign_approval_();
  void update_noads_timeout_();
  void update_creative_exclusion_();
  void delete_site_();
  void update_site_freq_caps_();
};
