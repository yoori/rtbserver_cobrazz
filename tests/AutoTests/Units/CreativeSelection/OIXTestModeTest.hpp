#ifndef _UNITTEST__OIXTESTMODETEST_
#define _UNITTEST__OIXTESTMODETEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;
 
/**
 * @class OIXTestModeTest
 * @brief Test for OIX test mode
 */
class OIXTestModeTest: public BaseDBUnit
{
public: 
  OIXTestModeTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var),
      now_((Generics::Time::get_time_of_day().
        get_gm_time().format("%d-%m-%Y:%H-") +
          "00-00").c_str())
  {};
 
  virtual ~OIXTestModeTest() noexcept
  {};
 
private:

  AutoTest::Time now_;

  // Helper function
  void make_derivative_requests_(
    AutoTest::AdClient& client,
    size_t testrequest,
    const std::string& exp_ccid,
    bool expected_adv_action_url);

  void process_test_case_(size_t index);
  void both_campaigns_can_match_();
  void inventory_mode_tag_();
  void channel_overlaps_();

  virtual void pre_condition();
  virtual bool run();
  virtual void post_condition();
  virtual void tear_down();

private:
  ORM::StatsArray<ORM::ChannelIdBasedStats, 4> non_test_ch_stats_;
  ORM::StatsArray<ORM::ChannelIdBasedStats, 4> test_ch_stats_;
  ORM::StatsArray<ORM::ChannelIdBasedStats, 2> ctx_ch_stats_;
  ORM::StatsArray<ORM::ColoIdBasedStats, 2> colo_stats_;
  ORM::StatsArray<ORM::SiteIdBasedStats, 3> site_stats_;
  ORM::StatsArray<ORM::TagIdBasedStats, 4> tag_stats_;
  ORM::StatsArray<ORM::HourlyStats, 4> test_stats_;
  ORM::ChannelOverlapUserStats overlap_stats_;
  ORM::StatsList<ORM::ChannelOverlapUserStats> overlap_empty_stats_;
};

#endif
