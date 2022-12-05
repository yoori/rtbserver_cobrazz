#ifndef _UNITTEST__LOGGINGTEST_
#define _UNITTEST__LOGGINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;

// need REVIEW : some case can be excess after ADSC-5744
class StatsHourlyLoggingTest: public BaseDBUnit
{
  typedef ORM::HourlyStats HourlyStats;
  typedef AutoTest::AdClient AdClient;
  typedef std::pair<AdClient, std::string> RequestPair;
  typedef std::list<RequestPair> RequestList;
  
public:
  StatsHourlyLoggingTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~StatsHourlyLoggingTest() noexcept
  { }

protected:
  
  virtual void set_up();
  virtual bool run();
  virtual void tear_down();

private:

  void
  case_all_counters_imptrack_disabled();

  void
  case_clicks_actions_noimpressions_part_1(
    ORM::StatsArray<HourlyStats, 2>& stats,
    RequestList& req_list);
  
  void
  case_clicks_actions_noimpressions_part_2(
    ORM::StatsArray<HourlyStats, 2>& stats,
    RequestList& req_list);
  
  void
  case_ammounts(
    const std::string& name_prefix);
  
  void
  case_with_sdate_tinkling();
  
  void
  case_multiple_confirmation_of_creative();
  
  void
  case_template_level_disabled_imptrack();
  
  void
  case_absent_imp_req_id();

private:
  AutoTest::Time target_request_time_;
};

#endif
