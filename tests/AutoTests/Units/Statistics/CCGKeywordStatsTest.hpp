#ifndef _UNITTEST__CCGKEYWORDSTATSTEST_
#define _UNITTEST__CCGKEYWORDSTATSTEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;

class CCGKeywordStatsTest: public BaseDBUnit
{

  enum StatOption
  {
    SO_DUMP_TOTAL = 1  // Create total stat & diff
  };
  
public:
  CCGKeywordStatsTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var),
      current_total_(0)
  { }
 
  virtual ~CCGKeywordStatsTest() noexcept
  { }

protected:

  virtual void set_up();
  
  virtual bool run();

  virtual void tear_down();
 
private:

  // Cases

  void
  system_currency_case_();
  
  void
  advertiser_currency_case_();
  
  void
  mixed_currency_case_();
  
  void
  advertiser_timezone_case_();

  // Test utils
  
  void 
  add_stats_(
    const std::string& description,
    const ORM::CCGKeywordStatsHourly::Key& key,
    const ORM::CCGKeywordStatsHourly::Diffs& diff,
    unsigned long flags = SO_DUMP_TOTAL);

 
private:

  ORM::StatsList<ORM::CCGKeywordStatsTotal> total_;
  ORM::StatsList<ORM::CCGKeywordStatsHourly> hourly_;
  ORM::StatsList<ORM::CCGKeywordStatsToW> week_;
  ORM::StatsList<ORM::CCGKeywordStatsDaily> daily_;
  std::list<ORM::CCGKeywordStatsHourly::Diffs> diffs_;
  std::list<ORM::CCGKeywordStatsTotal::Diffs> total_diffs_;
  ORM::CCGKeywordStatsTotal::Diffs current_total_;

};

#endif
