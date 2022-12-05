
#ifndef _AUTOTEST__OPENRTBTEST_
#define _AUTOTEST__OPENRTBTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class OpenRTBTest : public BaseDBUnit
{
  typedef AutoTest::ORM::CcgAuctionStatsDaily CcgStats;
  typedef CcgStats::Diffs CcgStatsDiff;

  struct CCGStatsExpected
  {
    const char* ccg;
    unsigned long auctions_lost;
  };
  
public:
  OpenRTBTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~OpenRTBTest() noexcept
  { }

private:

  virtual bool run();
  virtual void tear_down();

  void simple_case();
  void auctions_lost();
    
};

#endif // _AUTOTEST__OPENRTBTEST_
