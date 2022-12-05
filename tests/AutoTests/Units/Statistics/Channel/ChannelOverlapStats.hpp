
#ifndef _AUTOTEST__CHANNELOVERLAPSTATS_
#define _AUTOTEST__CHANNELOVERLAPSTATS_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

namespace ORM = AutoTest::ORM;

class ChannelOverlapStats : public BaseUnit
{
  typedef AutoTest::AdClient AdClient;
  typedef ORM::ChannelOverlapUserStats Stat;
  typedef ORM::StatsList<Stat> Stats;
  typedef ORM::ChannelOverlapUserStats::Diffs Diff;
  typedef std::list<Diff> Diffs;
  
  struct Request
  {
    const char* kwds;
    const char* expected;
    const char* unexpected;
    int time_ofset;
  };

  struct Expected
  {
    const char* channel1;
    const char* channel2;
    unsigned long users;
  };

public:
  ChannelOverlapStats(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var),
    conn_(open_pq())
  {};

  virtual ~ChannelOverlapStats() noexcept
  {};

private:

  virtual bool run_test();


  // Utils
  template<size_t Count>
  void
  send_requests(
    const std::string& description,
    AdClient* client,
    const Request(&requests) [Count]);

  void
  add_stat(
    const std::string& description,
    const char* channel1,
    const char* channel2,
    unsigned long users,
    unsigned long time_ofset);
  
  template<size_t Count>
  void
  add_stats(
    const std::string& description,
    const Expected(&expected) [Count],
    unsigned long time_ofset = 0);

  void
  check_stats(const std::string& description);


  //Cases
  void flags_case();
  void country_case();
  void channel_type_case();
  void non_optin_case();
  void config_case();
  void expression_case();
  void history_today_case(AdClient& client);
  void history_tomorrow_case(AdClient& client);
  void daily_proc_case();
  void status_case();
  
  AutoTest::Time now_;
  AutoTest::DBC::Conn conn_;
  Stats stats_;
  Diffs diffs_;

};

#endif // _AUTOTEST__CHANNELOVERLAPSTATS_
