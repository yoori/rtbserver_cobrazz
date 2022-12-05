#ifndef _UNITTEST__CAMPAIGNSTARTUPTEST_
#define _UNITTEST__CAMPAIGNSTARTUPTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
class CampaignStartupTest: public BaseDBUnit
{
public:
  CampaignStartupTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~CampaignStartupTest() noexcept
  {};
 
private:
  void set_up();
  void initial_check();
  void base_scenario();
  void tear_down();
  bool run();

private:
  AutoTest::NSLookupRequest request;
};

#endif
