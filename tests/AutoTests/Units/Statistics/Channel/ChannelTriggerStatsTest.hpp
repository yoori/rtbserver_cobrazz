
#ifndef _UNITTEST__CHANNELTRIGGERSTATLOGGINGTEST_
#define _UNITTEST__CHANNELTRIGGERSTATLOGGINGTEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;
namespace DB = ::AutoTest::DBC;

/**
 * @class ChannelTriggerStatsTest
 * @brief test channel trigger stat logging behaviour
 * see https://confluence.ocslab.com/display/ADS/ChannelTriggerStatsTest
 */  
class ChannelTriggerStatsTest: public BaseDBUnit
{
  typedef ORM::ChannelTriggerStats Stat;
  typedef ORM::StatsList<Stat> Stats;
  typedef Stat::Diffs Diff;
  typedef std::list<Diff> Diffs;

public:
  ChannelTriggerStatsTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var),
    default_colo_(fetch_int("DefaultColo"))
  { }
 
  virtual ~ChannelTriggerStatsTest() noexcept
  { }

protected:

  virtual bool run();

  virtual void set_up();
  
  virtual void tear_down();

private:

  AutoTest::Time now_;

  struct ChannelTriggerStatsRow
  {
    unsigned long colo_id;
    const char* channel_trigger_id;
    const char* trigger_type;
    int hits;
  };

  struct ChannelTriggerRequest
  {
    unsigned long colo;
    unsigned long tid;
    unsigned long ccid;
    const char* referer_kw;
    const char* referer;
    const char* search;
    const char* ft;
  };

  unsigned long default_colo_;

  void no_tid_case_();
  void with_ad_case_();
  void url_kwd_();
  void adsc_6348_();
  void adsc_7962_();

  // help funs
  template<size_t Count>
  void add_stats_(
    Stats& stats,
    Diffs& diffs,
    const ChannelTriggerStatsRow (&expected)[Count]);

  template<size_t Count>
  void process_requests_(
    const ChannelTriggerRequest (&requests)[Count]);
};

#endif
