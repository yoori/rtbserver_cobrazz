#ifndef _AUTOTEST__STATSHOURLYNUMSHOWINGTEST_
#define _AUTOTEST__STATSHOURLYNUMSHOWINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace DB = AutoTest::DBC;
namespace ORM = AutoTest::ORM;
 
class StatsHourlyNumShowingTest: public BaseUnit
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  
public:
  StatsHourlyNumShowingTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var),
    conn_(open_pq())
  { }
 
  virtual ~StatsHourlyNumShowingTest() noexcept
  { }
 
private:
  virtual bool run_test();

  void
  make_requests_(
    const std::string& description,
    const NSLookupRequest& request,
    const std::list<std::string>& expected_ccs,
    bool impression = false);

  void set_up();
  void num_shown_one_case_();
  void num_shown_two_case_();
  void num_shown_two_pub_specific_currency_case_();
  void num_shown_two_track_imp_case_();

private:
  DB::Conn conn_;
  AutoTest::Time today_;
  ORM::StatsList<ORM::HourlyStats> stats_;
  std::list<ORM::HourlyStats::Diffs> diffs_;
};

#endif  // _AUTOTEST__STATSHOURLYNUMSHOWINGTEST_
