
#ifndef _AUTOTEST__MINUIDAGE_
#define _AUTOTEST__MINUIDAGE_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class MinUidAge : public BaseUnit
{

  typedef AutoTest::AdClient AdClient;
  
  struct TestCase
  {
    int ofset;
    const char* referer_kw;
    const char* expected_cc;
  };
  
public:
  MinUidAge(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var),
    base_time_(AutoTest::Time().get_gm_time().get_date())
  { }

  virtual ~MinUidAge() noexcept
  { }

private:

  AutoTest::Time base_time_;

  // Utils
  template <size_t Count>
  void
  process_case(
    AdClient& client,
    const TestCase(&testcases)[Count],
    unsigned long colo = 0);

  // Test cases
  void
  uid_age_();

  void
  boundary_values_();

  void
  non_optin_();

  void
  temporary_();
    
  virtual bool run_test();

  
};

#endif // _AUTOTEST__MINUIDAGE_

