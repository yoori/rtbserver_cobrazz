
#ifndef _AUTOTEST__CMPSTATTEST_
#define _AUTOTEST__CMPSTATTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;

class CMPStatTest: public BaseDBUnit
{
public:

  typedef ORM::CMPRequestStats CMPStat;
  typedef CMPStat::Diffs ExpectedDiff;

private:

  struct Request
  {
    const char* referer_kws;
    const char* tid;
    const char* channels;
    const char* ccs;
  };

  enum CmpDateField
  {
    CDF_SDATE,      // Use sdate
    CDF_ADV_SDATE,  // Use adv_sdate
    CDF_CMP_SDATE   // Use cmp_sdate
  };

  struct CaseStat
  {
    // Key
    const char* adv_account;
    const char* cmp_account;
    const char* colo;
    const char* channel;
    const char* cc;
    bool walled_garden;
    CmpDateField date_type;
    AutoTest::Time date;
    ExpectedDiff diff; // Expected result
  };

  typedef ORM::StatsList<CMPStat> CMPStats;
  typedef std::list<ExpectedDiff> CMPDiffs;
  
public: 
  CMPStatTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~CMPStatTest() noexcept
  { }

protected:

  virtual void set_up();
  virtual bool run();
  virtual void tear_down();
 
private:

  template<size_t Count>
  void
  initialize_stats(
    CMPStats& stats,
    CMPDiffs& diffs,
    const CaseStat(&case_stats) [Count]);

  template<size_t Count>
  void
  send_requests(
    const AutoTest::Time& debug_time,
    const Request(&requests) [Count]);
  
  void
  pub_exp_ch_scenario();
  
  void
  base_scenario();
  
  void
  adv_exp_ch_scenario();
  
  void
  cmp_exp_ch_scenario();
  
  void
  currency_scenario();
  
  void
  expression_scenario();
  
  void
  ta_scenario();
};

#endif //_AUTOTEST__CMPSTATTEST_
