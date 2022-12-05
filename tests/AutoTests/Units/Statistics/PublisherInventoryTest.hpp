#ifndef _AUTOTEST__PUBLISHERINVENTORYTEST_
#define _AUTOTEST__PUBLISHERINVENTORYTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
namespace ORM = AutoTest::ORM;
namespace DBC = AutoTest::DBC;

class PublisherInventoryTest: public BaseUnit
{

typedef ORM::PublisherInventory PublisherInventory;
typedef ORM::StatsList<PublisherInventory> PublisherInventoryStats;
typedef PublisherInventory::Diffs Diff;
typedef std::list<Diff> Diffs;

typedef ORM::HourlyStats HourlyStat;
typedef ORM::StatsList<HourlyStat> HourlyStats;
typedef HourlyStat::Diffs HourlyDiff;
typedef std::list<HourlyDiff> HourlyDiffs;

typedef ORM::SiteUserStats SiteUserStat;
typedef ORM::StatsList<SiteUserStat> SiteUserStats;
typedef SiteUserStat::Diffs SiteUserDiff;
typedef std::list<SiteUserDiff> SiteUserDiffs;

typedef ORM::ChannelInventoryStats ChannelInventoryStat;
typedef ORM::StatsList<ChannelInventoryStat> ChannelInventoryStats;
typedef ChannelInventoryStat::Diffs ChannelInventoryDiff;
typedef std::list<ChannelInventoryDiff> ChannelInventoryDiffs;

public:
 
  PublisherInventoryTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var),
    pgconn_(open_pq()),
    debug_time_((AutoTest::Time().get_gm_time().format("%d-%m-%Y") +
        ":" + "00-01-00").c_str())
  { }
 
  virtual ~PublisherInventoryTest() noexcept
  { }
 
private:

  DBC::Conn pgconn_;
  AutoTest::Time debug_time_;

  double ctr_;
  double pub_rate_;
  double adv_rate_;

  void base_scenario(AutoTest::AdClient& client,
                     unsigned int tag_id,
                     double pub_rate,
                     unsigned int colo_id = 0);
  void ta_campaigns_scenario();
  void pub_adv_commission_scenario(unsigned int tag,
                                   double pub_rate = 1.0);
  void no_impression_scenario();
  void billing_stats_logging();
  void non_billing_stats_logging();
  void publisher_with_adjustment_coef();

  // Related to ADSC-5384
  void virtual_scenario();

  virtual bool run_test();

  PublisherInventoryStats publisher_inventory_stats_;
  Diffs publisher_inventory_diffs_;

  HourlyStats hourly_stats_;
  HourlyDiffs hourly_diffs_;

  SiteUserStats site_user_stats_;
  SiteUserDiffs site_user_diffs_;

  ChannelInventoryStats channel_inventory_stats_;
  ChannelInventoryDiffs channel_inventory_diffs_;

  void add_publisher_inventory_stat(
    const char* description,
    unsigned int tag_id,
    double cpm,
    int imps,
    int requests,
    double revenue,
    AutoTest::Time sdate = Generics::Time::ZERO);

  void add_hourly_stat(
    const char* description,
    unsigned int pub_account,
    unsigned int tag_id,
    int requests);

  void add_site_user_stat_(
    const char* description,
    unsigned int site_id,
    int unique_users,
    bool set_lad_param = false,
    const AutoTest::Time& last_appearance_date = Generics::Time::ZERO);

  void add_channel_inv_stat_(
    const char* description,
    unsigned long channel_id,
    int hits,
    int active_users,
    int total_users);
};

#endif //_AUTOTEST__PUBLISHERINVENTORYTEST_

