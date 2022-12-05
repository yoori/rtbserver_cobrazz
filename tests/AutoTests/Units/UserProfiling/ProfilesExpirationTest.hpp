
#ifndef _AUTOTEST__PROFILESEXPIRATIONTEST_
#define _AUTOTEST__PROFILESEXPIRATIONTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class ProfilesExpirationTest: public BaseUnit
{
public:
 
  ProfilesExpirationTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var), remote_case_(false)
  {};
 
  virtual ~ProfilesExpirationTest() noexcept
  {};
 
private:

  bool remote_case_;

  void
  check_profiles_exist(
    const std::string& uid,
    bool exists,
    bool temp_user);

  void
  expired_visits_removal_();

  void
  user_profiles_removal_(
    AutoTest::AdClient& user1,
    unsigned long time1,
    AutoTest::AdClient& user2,
    unsigned long time2,
    bool temp_user = false);
 
  virtual bool run_test();
 
};

#endif //_AUTOTEST__PROFILESEXPIRATIONTEST_
