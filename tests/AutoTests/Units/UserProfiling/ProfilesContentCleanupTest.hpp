
#ifndef _AUTOTEST__PROFILESCONTENTCLEANUPTEST_
#define _AUTOTEST__PROFILESCONTENTCLEANUPTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
class ProfilesContentCleanupTest: public BaseUnit
{
public:
 
  ProfilesContentCleanupTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var),
    base_time_(
      (AutoTest::Time().get_gm_time().format("%d-%m-%Y") +
        ":" + "00-01-00").c_str())
  {};
 
  virtual ~ProfilesContentCleanupTest() noexcept
  {};
 
private:

  AutoTest::Time base_time_;
  std::string channel_,
              cc_id_,
              ccg_id_;

  void cleanup_by_visits_count();
  void cleanup_by_visits_date();

  virtual bool run_test();
 
};

#endif //_AUTOTEST__PROFILESCONTENTCLEANUPTEST_
