
#include "ChannelPriceRangeLogging.hpp"

REFLECT_UNIT(ChannelPriceRangeLogging) (
  "Statistics",
  AUTO_TEST_SLOW
);

//ATTENTION! inventory_users_percentage must be 100 for this test
//Before running test modify files ExpressionMatcherConfig.xml &
//CampaignManageConfigr.xml

namespace
{
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::Statistics::UserSet UserSet;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  
  //Test constant
  const int            REPEAT_COUNT         = 400;   
  const char           COUNTRY_CODE[]       = "GN";
  const char           COUNTRY[]            = "gn";
  const char           LU_CODE[]            = "LU";
  const char           LU[]                 = "lu";
  const char           US_CODE[]            = "US";
  const char           US[]                 = "us";

  // Common eCPMs
  const unsigned short ECPM1               = 10;
  const unsigned short ECPM2               = 20;
  const unsigned short ECPM3               = 40;
  const unsigned short ECPM4               = 15;

  /**
   * @class SelectedCreativesCheck
   * @brief Check creative selection on user collection.
   */
  class SelectedCreativesCheck : public AutoTest::Checker
  {
  public:
    /**
     * @brief Constructor.
     *
     * @param test.
     * @param user set (collection).
     * @param expected CC ids.
     * @param description.
     * @param first user index.
     * @param last user index.
     */
    SelectedCreativesCheck(
      BaseUnit* test,
      UserSet& users,
      const char* exp_ccids,
      const std::string& description,
      unsigned long start_index = 0,
      unsigned long stop_index = 0)
      : test_(test),
        users_(users),
        exp_ccids_(exp_ccids),
        description_(description),
        start_index_(start_index),
        stop_index_(stop_index? stop_index: users.size())
    { }

    /**
     * @brief Destructor.
     */
    virtual ~SelectedCreativesCheck() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool = true) /*throw(eh::Exception)*/
    {
      std::list<std::string> exp_ccids;
      test_->fetch_objects(
        std::inserter(exp_ccids, exp_ccids.begin()),
        exp_ccids_.c_str());

      for (unsigned int i = std::min(start_index_, stop_index_);
           i < std::max(start_index_, stop_index_); ++i)
      {
        if (!AutoTest::equal_seq(exp_ccids,
            SelectedCreativesCCID(users_[i])))
        {
          Stream::Error ostr;
          ostr << description_ << ". User#" << i << 
            ": " << AutoTest::seq_to_str(exp_ccids) <<
            " not equal to " <<
            AutoTest::seq_to_str(SelectedCreativesCCID(users_[i]));
          throw AutoTest::CheckFailed(ostr);
        }
      }
      return true;
    }

  private:
    BaseUnit* test_;          
    UserSet& users_;
    std::string exp_ccids_;
    std::string description_;
    unsigned long start_index_;
    unsigned long stop_index_;
  };
}


ChannelPriceRangeLogging::ChannelPriceRangeLogging(
  UnitStat& stat_var,
  const char* task_name,
  XsdParams params_var):
  BaseDBUnit(stat_var, task_name, params_var)
{ }

bool 
ChannelPriceRangeLogging::run()
{
  creative_size_id = fetch_int("CreativeSize/Common");
  default_colo_ = fetch_int("DefaultColo");

  AUTOTEST_CASE(
    one_ecpm_group(),
    "One ECPM group");

  AdClient tag_cpm_client(AdClient::create_user(this));
  
  AUTOTEST_CASE(
    tag_cpm_part_1(tag_cpm_client),
    "Tag CPM");
  
  AutoTest::Statistics::UserSet groups_clients;
  groups_clients.initialize(this, 3*REPEAT_COUNT);

  AUTOTEST_CASE(
    competitive_ecpm_groups(groups_clients),
    "Competitive ECPM groups");

  AdClient day_switch_client(AdClient::create_user(this));

  AUTOTEST_CASE(
    day_switching_part_1(day_switch_client),
    "Expression matcher day switching");

  AUTOTEST_CASE(
    key_variation(),
    "Key variation");

  AUTOTEST_CASE(
    no_impression(),
    "No impression");

  AUTOTEST_CASE(
    currency(),
    "Currency");

//   AUTOTEST_CASE(
//     text_advertising(),
//     "Text advertising");

//   AUTOTEST_CASE(
//     tag_adjustment(),
//     "Tag Adjustment");

  check();
    
  AUTOTEST_CASE(
    one_ecpm_group(),
    "One ECPM group");

  AUTOTEST_CASE(
    tag_cpm_part_2(tag_cpm_client),
    "Tag CPM");

//   AUTOTEST_CASE(
//     move_between_ecpm_groups(0, groups_clients),
//     "Competitive ECPM groups");

  AUTOTEST_CASE(
    day_switching_part_2(day_switch_client),
    "Expression matcher day switching");

  check();

  AUTOTEST_CASE(
    one_ecpm_group(),
    "One ECPM group");

//   AUTOTEST_CASE(
//     move_between_ecpm_groups(1, groups_clients),
//     "Competitive ECPM groups");
  
  AUTOTEST_CASE(
    day_switching_part_3(day_switch_client),
    "Expression matcher day switching");

//   check();

//   AUTOTEST_CASE(
//     move_between_ecpm_groups(2, groups_clients),
//     "Competitive ECPM groups");

//   check();
  
//   AUTOTEST_CASE(
//     new_day(groups_clients),
//     "Competitive ECPM groups");
  
  return true;
}

void ChannelPriceRangeLogging::tear_down()
{ }

void ChannelPriceRangeLogging::one_ecpm_group()
{

  AutoTest::Statistics::UserSet clients;
  clients.initialize(this, REPEAT_COUNT);
  
  ORM::StatsArray<ORM::ChannelInventoryByCPMStats, 4> stats;

  stats[0].key().
    channel_id(fetch_int("ONE_GROUP/Expr1")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM1).
    sdate(today);
  stats[1].key().
    channel_id(fetch_int("ONE_GROUP/Expr2")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM1).
    sdate(today);
  stats[2].key().
    channel_id(fetch_int("ONE_GROUP/Expr3")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM1).
    sdate(today);
  stats[3].key().
    channel_id(fetch_int("ONE_GROUP/Expr4")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM1).
    sdate(today);

  stats.select(pq_conn_);

  clients.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(fetch_string("ONE_GROUP/TAG/COMMON")).
      referer_kw(fetch_string("ONE_GROUP/Kwd/B1")).
      debug_time(today));

  FAIL_CONTEXT(
    SelectedCreativesCheck(
      this, clients,"ONE_GROUP/CC/Display1",
      "Check CC#1").check());
   
  clients.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(fetch_string("ONE_GROUP/TAG/COMMON")).
      referer_kw(map_objects("ONE_GROUP/Kwd/B1,ONE_GROUP/Kwd/B2")).
      debug_time(today));

  const Diff diffs[] =
  {
    Diff().
      user_count(REPEAT_COUNT).
      impops(2*REPEAT_COUNT),
    Diff(REPEAT_COUNT),
    Diff().
      user_count(REPEAT_COUNT).
      impops(2*REPEAT_COUNT),
    Diff(REPEAT_COUNT)
  };
    
  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_,
      diffs,
      stats));
}

void ChannelPriceRangeLogging::tag_cpm_part_1(
  AdClient& client)
{
  ORM::StatsArray<ORM::ChannelInventoryByCPMStats,  5> stats;
  stats[0].key().
    channel_id(fetch_int("TAG_CPM/Expr1")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2).
    sdate(today);
  stats[1].key().
    channel_id(fetch_int("TAG_CPM/Expr2")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2).
    sdate(today);
  stats[2].key().
    channel_id(fetch_int("TAG_CPM/Expr3")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(fetch_float("TAG_CPM/MINECPM/COMMON")).
    sdate(today);
  stats[3].key().
    channel_id(fetch_int("TAG_CPM/Expr1")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM1).
    sdate(today);
  stats[4].key().
    channel_id(fetch_int("TAG_CPM/Expr2")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM1).
    sdate(today);

  stats.select(pq_conn_);

  client.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(fetch_string("TAG_CPM/TAG/Display2")).
      referer_kw(map_objects("TAG_CPM/Kwd/B2")).
      debug_time(today));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Request#1");


  client.process_request (
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(fetch_string("TAG_CPM/TAG/Display2")).
      referer_kw(map_objects("TAG_CPM/Kwd/B2,TAG_CPM/Kwd/B1")).
      debug_time(today));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("TAG_CPM/CC/Display2"),
      client.debug_info.ccid).check(),
    "Request#2");

  const Diff diffs[] =
  {
    // Expr#1, ECPM2
    Diff().
      user_count(1).
      impops(1),
    // Expr#3, ECPM2
    Diff().
      user_count(1).
      impops(1),
    // Expr#5, TAG CPM
    Diff().
      user_count(1).
      impops(1),
    // Expr#1, ECPM1
    Diff(0),
    // Expr#3, ECPM1
    Diff(0)
  };

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void ChannelPriceRangeLogging::tag_cpm_part_2(
  AdClient& client)
{
  ORM::StatsArray<ORM::ChannelInventoryByCPMStats,  5> stats;
  stats[0].key().
    channel_id(fetch_int("TAG_CPM/Expr1")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2).
    sdate(today);
  stats[1].key().
    channel_id(fetch_int("TAG_CPM/Expr2")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2).
    sdate(today);
  stats[2].key().
    channel_id(fetch_int("TAG_CPM/Expr3")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(fetch_float("TAG_CPM/MINECPM/COMMON")).
    sdate(today);
  stats[3].key().
    channel_id(fetch_int("TAG_CPM/Expr1")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM1).
    sdate(today);
  stats[4].key().
    channel_id(fetch_int("TAG_CPM/Expr2")).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM1).
    sdate(today);

  stats.select(pq_conn_);

  client.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(fetch_string("TAG_CPM/TAG/COMMON")).
      referer_kw(map_objects("TAG_CPM/Kwd/B2,TAG_CPM/Kwd/B1")).
      debug_time(today));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("TAG_CPM/CC/Display1"),
      client.debug_info.ccid).check(),
    "Check CC");
  
  const Diff diffs[] =
  {
    // Expr#1, ECPM2
    Diff().
      user_count(-1).
      impops(0),
    // Expr#3, ECPM2
    Diff().
      user_count(-1).
      impops(0),
    // Expr#5, TAG CPM
    Diff(0),
    // Expr#1, ECPM1
    Diff().
      user_count(1).
      impops(1),
    // Expr#3, ECPM1
    Diff().
      user_count(1).
      impops(1)
  };

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void
ChannelPriceRangeLogging::competitive_ecpm_groups(
  AutoTest::Statistics::UserSet& clients)
{
  std::string tid1     = fetch_string("GROUPS/TAG/Display3");
  std::string tid2     = fetch_string("GROUPS/TAG/Display4");
  std::string keyword  = fetch_string("GROUPS/Kwd/B1");
  double ecpm2 = fetch_float("GROUPS/CPM/Display7");
  unsigned long b_channel = fetch_int("GROUPS/B4");
  unsigned long e_channel = fetch_int("GROUPS/Expr6");
  unsigned long expression1 = fetch_int("GROUPS/Expr1");
  unsigned long expression3 = fetch_int("GROUPS/Expr3");
  unsigned long colo1 = fetch_int("COLO/Colo1");

  Stats stats;
  Diffs diffs;

  const StatKey STATS[] =
  {
    //sdate   colo_id         channel_id    creative_size     country       ECPM    user_count      impops
    { today,  default_colo_,  expression1,  creative_size_id, COUNTRY_CODE, ECPM2,  3*REPEAT_COUNT, 3*REPEAT_COUNT },
    { today,  default_colo_,  expression1,  creative_size_id, COUNTRY_CODE, ECPM3,  0,              3*REPEAT_COUNT },
    { today,  default_colo_,  expression3,  creative_size_id, COUNTRY_CODE, ECPM2,  3*REPEAT_COUNT, 3*REPEAT_COUNT },
    { today,  default_colo_,  expression3,  creative_size_id, COUNTRY_CODE, ECPM3,  0,              3*REPEAT_COUNT },
    { today,  colo1,          e_channel,    creative_size_id, LU_CODE,      ecpm2,  3*REPEAT_COUNT, 3*REPEAT_COUNT },
    { today,  colo1,          b_channel,    creative_size_id, LU_CODE,      ecpm2,  3*REPEAT_COUNT, 3*REPEAT_COUNT },
    { today,  default_colo_,  e_channel,    creative_size_id, LU_CODE,      ecpm2,  0, 0 },
    { today,  default_colo_,  b_channel,    creative_size_id, LU_CODE,      ecpm2,  0, 0 }
  };
  
  add_stats(STATS, stats, diffs);

  clients.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(tid1).
      referer_kw(keyword).
      debug_time(today));

  FAIL_CONTEXT(
      SelectedCreativesCheck(
        this, clients, "GROUPS/CC/Display3",
        "Check CC#1").check());

  clients.process_request(NSLookupRequest().
    loc_name(COUNTRY).
    tid(tid2).
    referer_kw(keyword).
    debug_time(today));

  FAIL_CONTEXT(
    SelectedCreativesCheck(
      this, clients,"GROUPS/CC/Display4",
      "Check CC#2").check());

  clients.process_request(NSLookupRequest().
    loc_name(LU).
    tid(fetch_int("GROUPS/TAG/Display7")).
    referer_kw(fetch_string("GROUPS/Kwd/B4")).
    colo(colo1).
    debug_time(today));

  FAIL_CONTEXT(
    SelectedCreativesCheck(
      this, clients,"GROUPS/CC/Display7",
      "Check cc#3").check());

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_,
      diffs,
      stats));
}

void
ChannelPriceRangeLogging::move_between_ecpm_groups(
  unsigned int index,
  AutoTest::Statistics::UserSet& clients)
{
  std::string tid1     = fetch_string("GROUPS/TAG/Display3");
  std::string tid2     = fetch_string("GROUPS/TAG/Display5");
  std::string keyword  = fetch_string("GROUPS/Kwd/B1");
  double ecpm1 = fetch_float("GROUPS/CPM/Display6");
  double ecpm2 = fetch_float("GROUPS/CPM/Display7");
  unsigned long b_channel = fetch_int("GROUPS/B4");
  unsigned long e_channel = fetch_int("GROUPS/Expr6");
  unsigned long expression1 = fetch_int("GROUPS/Expr1");
  unsigned long expression3 = fetch_int("GROUPS/Common-Expr3");
  unsigned long colo2 = fetch_int("COLO/Colo2");

  Stats stats;
  Diffs diffs;

  const StatKey STATS[] =
  {
    //sdate   colo_id         channel_id    creative_size     country       ECPM    user_count      impops
    { today,  default_colo_,  expression1,  creative_size_id, COUNTRY_CODE, ECPM2, -REPEAT_COUNT, REPEAT_COUNT },
    { today,  default_colo_,  expression3,  creative_size_id, COUNTRY_CODE, ECPM2, -REPEAT_COUNT, REPEAT_COUNT },
    { today,  default_colo_,  expression1,  creative_size_id, COUNTRY_CODE, ECPM4,  REPEAT_COUNT, REPEAT_COUNT },
    { today,  default_colo_,  expression3,  creative_size_id, COUNTRY_CODE, ECPM4,  REPEAT_COUNT, REPEAT_COUNT },
    { today,  colo2,          e_channel,    creative_size_id, LU_CODE,      ecpm1,  REPEAT_COUNT, REPEAT_COUNT },
    { today,  colo2,          b_channel,    creative_size_id, LU_CODE,      ecpm1,  REPEAT_COUNT, REPEAT_COUNT },
    { today,  colo2,          e_channel,    creative_size_id, LU_CODE,      ecpm2, -REPEAT_COUNT, REPEAT_COUNT },
    { today,  colo2,          b_channel,    creative_size_id, LU_CODE,      ecpm2, -REPEAT_COUNT, REPEAT_COUNT },
  };

  add_stats(STATS, stats, diffs);

  unsigned long index1 = index * REPEAT_COUNT;
  unsigned long index2  = (index + 1) * REPEAT_COUNT;
  clients.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(tid1).
      referer_kw(keyword).
      debug_time(today),
    index1, index2);

  FAIL_CONTEXT(
    SelectedCreativesCheck(
      this, clients,"GROUPS/CC/Display3",
      "Check CC#1",
      index1, index2).check());
    
  clients.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(tid2).
      referer_kw(keyword).
      debug_time(today),
    index1, index2);

  FAIL_CONTEXT(
    SelectedCreativesCheck(
      this, clients,"GROUPS/CC/Display5",
      "Check CC#2",
      index1, index2).check());

  clients.process_request(
    NSLookupRequest().
      loc_name(LU).
      tid(fetch_int("GROUPS/TAG/Display7")).
      referer_kw(fetch_string("GROUPS/Kwd/B4")).
      colo(colo2).
      debug_time(today),
    index1, index2);
    
  FAIL_CONTEXT(
    SelectedCreativesCheck(
      this, clients,"GROUPS/CC/Display7",
      "Check CC#3",
      index1, index2).check());

  clients.process_request(
    NSLookupRequest().
      loc_name(LU).
      tid(fetch_int("GROUPS/TAG/Display6")).
      referer_kw(fetch_string("GROUPS/Kwd/B4")).
      colo(colo2).
      debug_time(today),
    index1, index2);
 
  FAIL_CONTEXT(
    SelectedCreativesCheck(
      this, clients,"GROUPS/CC/Display6",
      "Check CC#4",
      index1, index2).check());

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));  
}

void
ChannelPriceRangeLogging::new_day(
  AutoTest::Statistics::UserSet& clients)
{
  std::string tid     = fetch_string("GROUPS/TAG/Display4");
  std::string keyword = fetch_string("GROUPS/Kwd/B1");
  
  AutoTest::Time tomorrow = today + 24*60*60;

  ORM::StatsArray<ORM::ChannelInventoryByCPMStats,  4> stats;
  stats[0].key().
    channel_id(fetch_int("GROUPS/Expr1")).
    sdate(today).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE);
  stats[1].key().
    channel_id(fetch_int("GROUPS/Expr3")).
    sdate(today).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE);
  stats[2].key().
    channel_id(fetch_int("GROUPS/Expr1")).
    sdate(tomorrow).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE);
  stats[3].key().
    channel_id(fetch_int("GROUPS/Expr3")).
    sdate(tomorrow).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM3);
  stats.select(pq_conn_);

  clients.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(tid).
      referer_kw(keyword).
      debug_time(tomorrow));

  FAIL_CONTEXT(
      SelectedCreativesCheck(
        this, clients, "GROUPS/CC/Display4",
        "Check CC").check());

  const Diff diffs[] =
  {
    // Expr#1,today
    Diff(0),
    // Expr#3, today
    Diff(0),
    // Expr#1, ECPM3, tomorrow
    Diff().
      user_count(3*REPEAT_COUNT).
      impops(3*REPEAT_COUNT),
    // Expr#3, ECPM3, tomorrow
    Diff().
      user_count(3*REPEAT_COUNT).
      impops(3*REPEAT_COUNT)
  };

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));  
}

void ChannelPriceRangeLogging::day_switching_part_1(
  AdClient& client)
{
  std::string tid1    = fetch_string("DAY_SWITCH/TAG/Display1");
  std::string keyword = fetch_string("DAY_SWITCH/Kwd/B1");

  AutoTest::Time  yesterday  = today - 24*60*60;

  ORM::StatsArray<ORM::ChannelInventoryByCPMStats,  2> stats;
  stats[0].key().
    channel_id(fetch_int("DAY_SWITCH/Expr1")).
    sdate(yesterday).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2);
  stats[1].key().
    channel_id(fetch_int("DAY_SWITCH/Expr2")).
    sdate(yesterday).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2);
  stats.select(pq_conn_);

  client.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(tid1).
      referer_kw(keyword).
      debug_time(yesterday));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("DAY_SWITCH/CC/Display1"),
      client.debug_info.ccid).check(),
    "Check CC");

  const Diff diffs[] =
  {
    Diff(1),
    Diff(1),
  };

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));

}

void ChannelPriceRangeLogging::day_switching_part_2(
  AdClient& client)
{
  std::string tid1    = fetch_string("DAY_SWITCH/TAG/Display1");
  std::string keyword = fetch_string("DAY_SWITCH/Kwd/B1");
  
  ORM::StatsArray<ORM::ChannelInventoryByCPMStats,  2> stats;
  stats[0].key().
    channel_id(fetch_int("DAY_SWITCH/Expr1")).
    sdate(today).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2);
  stats[1].key().
    channel_id(fetch_int("DAY_SWITCH/Expr2")).
    sdate(today).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2);
  stats.select(pq_conn_);

  client.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(tid1).
      referer_kw(keyword).
      debug_time(today));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("DAY_SWITCH/CC/Display1"),
      client.debug_info.ccid).check(),
    "Check CC");
  
  const Diff diffs[] =
  {
    Diff(1),
    Diff(1),
  };
    
  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}
                                               
void ChannelPriceRangeLogging::day_switching_part_3(
  AdClient& client)
{
  std::string tid2    = fetch_string("DAY_SWITCH/TAG/Display2");
  std::string keyword = fetch_string("DAY_SWITCH/Kwd/B1");

  AutoTest::Time  yesterday  = today - 24*60*60;

  ORM::StatsArray<ORM::ChannelInventoryByCPMStats,  4> stats;
  stats[0].key().
    channel_id(fetch_int("DAY_SWITCH/Expr1")).
    sdate(yesterday).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2);
  stats[1].key().
    channel_id(fetch_int("DAY_SWITCH/Expr2")).
    sdate(yesterday).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM2);
  stats[2].key().
    channel_id(fetch_int("DAY_SWITCH/Expr1")).
    sdate(yesterday).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM4);
  stats[3].key().
    channel_id(fetch_int("DAY_SWITCH/Expr2")).
    sdate(yesterday).
    creative_size_id(creative_size_id).
    country_code(COUNTRY_CODE).
    ecpm(ECPM4);
  stats.select(pq_conn_);

  client.process_request(
    NSLookupRequest().
      loc_name(COUNTRY).
      tid(tid2).
      referer_kw(keyword).
      debug_time(yesterday));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("DAY_SWITCH/CC/Display2"),
      client.debug_info.ccid).check(),
    "Check CC");
  
  const Diff diffs[] =
  {
    Diff().
      user_count(-1).
      impops(0),
    Diff().
      user_count(-1).
      impops(0),
    Diff(1),
    Diff(1),
  };

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void ChannelPriceRangeLogging::key_variation()
{
  Stats stats;
  Diffs diffs;

  double ecpm1 = fetch_float("KeyVariation/CPM/Display1");
  double ecpm2 = fetch_float("KeyVariation/CPM/Display2");
  double min_ecpm = fetch_float("KeyVariation/MINECPM/Country");
  unsigned long other_size_id = fetch_int("KeyVariation/CreativeSize");
  unsigned long b_channel = fetch_int("KeyVariation/B1");
  unsigned long e_channel = fetch_int("KeyVariation/Expr1");
  
  const StatKey KEY_VARIATION_STATS[] =
  {
    //sdate   colo_id         channel_id  creative_size     country   ECPM  user_count  impops
    { today,  default_colo_,  e_channel,  creative_size_id, LU_CODE,  ecpm1,    1,        1 },
    { today,  default_colo_,  b_channel,  creative_size_id, LU_CODE,  ecpm1,    1,        1 },
    { today,  default_colo_,  e_channel,  creative_size_id, US_CODE,  min_ecpm, 1,        1 },
    { today,  default_colo_,  b_channel,  creative_size_id, US_CODE,  min_ecpm, 1,        1 },
    { today,  default_colo_,  e_channel,  other_size_id,    LU_CODE,  ecpm2,    1,        1 },
    { today,  default_colo_,  b_channel,  other_size_id,    LU_CODE,  ecpm2,    1,        1 },
  };

  add_stats(KEY_VARIATION_STATS, stats, diffs);

  const  ChannelPriceRangeLogging::UserRequest KEY_VARIATION[] =
  {
    { "KeyVariation/Kwd/B1", "KeyVariation/TAG/COMMON", LU,
      0, "KeyVariation/BP/B1", "KeyVariation/CC/Display1"},
    { "KeyVariation/Kwd/B1", "KeyVariation/TAG/Country", US,
      0, "KeyVariation/BP/B1", 0},
    { "KeyVariation/Kwd/B1", "KeyVariation/TAG/Display2", LU,
      0, "KeyVariation/BP/B1", "KeyVariation/CC/Display2"}
  };

  process_requests(KEY_VARIATION);

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void
ChannelPriceRangeLogging::no_impression()
{
  Stats stats;
  Diffs diffs;

  double min_ecpm = fetch_float("NoImpression/MINECPM/PUBCPM");
  double fixed_margin = fetch_float("FixedMargin");
  unsigned long channel1 = fetch_int("NoImpression/B1");
  unsigned long channel2 = fetch_int("NoImpression/B2");
  const StatKey NO_IMRESSION_STATS[] =
  {
    //sdate   colo_id         channel_id  creative_size     country       ECPM          user_count  impops
    { today,  default_colo_,  channel1,   creative_size_id, COUNTRY_CODE, min_ecpm,         1,        1 },
    { today,  default_colo_,  channel2,   creative_size_id, COUNTRY_CODE, fixed_margin,     1,        1 }
  };

  add_stats(NO_IMRESSION_STATS, stats, diffs);

  const  ChannelPriceRangeLogging::UserRequest NO_IMPRESSION[] =
  {
    { "NoImpression/Kwd/B1", "NoImpression/TAG/PUBCPM",
      COUNTRY, 0, "NoImpression/BP/B1", 0},
    { "NoImpression/Kwd/B2", "NoImpression/TAG/FIXEDMARGIN",
      COUNTRY, 0, "NoImpression/BP/B2", 0}
  };

  process_requests(NO_IMPRESSION);

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void ChannelPriceRangeLogging::currency()
{
  Stats stats;
  Diffs diffs;

  double ecpm =
    fetch_float("Currency/CPM/Display1") / fetch_float("Currency/Adv/Rate");
  double min_ecpm =
    fetch_float("Currency/MINECPM/2") / fetch_float("Currency/Pub/Rate");
  unsigned long b_channel1 = fetch_int("Currency/B1");
  unsigned long b_channel2 = fetch_int("Currency/B2");
  unsigned long e_channel1 = fetch_int("Currency/Expr1");
  unsigned long e_channel2 = fetch_int("Currency/Expr2");
  
  const StatKey CURRENCY_STATS[] =
  {
    //sdate   colo_id         channel_id  creative_size     country       ECPM  user_count  impops
    { today,  default_colo_,  b_channel1, creative_size_id, COUNTRY_CODE, ecpm,     1,        1 },
    { today,  default_colo_,  b_channel2, creative_size_id, COUNTRY_CODE, min_ecpm, 1,        1 },
    { today,  default_colo_,  e_channel1, creative_size_id, COUNTRY_CODE, ecpm,     1,        1 },
    { today,  default_colo_,  e_channel2, creative_size_id, COUNTRY_CODE, min_ecpm, 1,        1 },
    { today,  default_colo_,  b_channel1, creative_size_id, COUNTRY_CODE, min_ecpm, 0,        0 },
    { today,  default_colo_,  b_channel2, creative_size_id, COUNTRY_CODE, ecpm,     0,        0 },
    { today,  default_colo_,  e_channel1, creative_size_id, COUNTRY_CODE, min_ecpm, 0,        0 },
    { today,  default_colo_,  e_channel2, creative_size_id, COUNTRY_CODE, ecpm,     0,        1 },
  };
  
  add_stats(CURRENCY_STATS, stats, diffs);

  const  ChannelPriceRangeLogging::UserRequest CURRENCY[] =
  {
    { "Currency/Kwd/B1", "Currency/TAG/COMMON",
      COUNTRY, 0, "Currency/BP/B1", "Currency/CC/Display1"},
    { "Currency/Kwd/B2", "Currency/TAG/2",
      COUNTRY, 0, "Currency/BP/B2", 0}
  };

  process_requests(CURRENCY);

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
 
}

void ChannelPriceRangeLogging::text_advertising()
{
  Stats stats;
  Diffs diffs;

  unsigned long ta_size_id = fetch_int("TA/CreativeSize");
  double ecpmText = fetch_float("TAText/ECPM");
  double ecpmChannel = fetch_float("TAChannel/CPM/Channel1") +
    fetch_float("TAChannel/CPM/Channel2");
  double ecpmMixed = fetch_float("TAMixed/ECPM");
  unsigned long k_channel1 = fetch_int("TAText/B1");
  unsigned long k_channel2 = fetch_int("TAText/B2");
  unsigned long b_channel1 = fetch_int("TAChannel/B1");
  unsigned long b_channel2 = fetch_int("TAChannel/B2");
  unsigned long b_channel_m = fetch_int("TAMixed/B2");
  unsigned long k_channel_m = fetch_int("TAMixed/B1");
  
  const StatKey TEXT_ADVERTISING_STATS[] =
  {
    { today,  default_colo_,  k_channel1,   ta_size_id, COUNTRY_CODE, ecpmText, 0,  0 },
    { today,  default_colo_,  k_channel2,   ta_size_id, COUNTRY_CODE, ecpmText, 0,  0 },
    { today,  default_colo_,  b_channel1,   ta_size_id, COUNTRY_CODE, ecpmChannel, 1,  1 },
    { today,  default_colo_,  b_channel2,   ta_size_id, COUNTRY_CODE, ecpmChannel, 1,  1 },
    { today,  default_colo_,  b_channel_m,  ta_size_id, COUNTRY_CODE, ecpmMixed, 1,  1 },
    { today,  default_colo_,  k_channel_m,  0,          0,            ecpmMixed, 0,  0 },
  };

  add_stats(TEXT_ADVERTISING_STATS, stats, diffs);

  const  ChannelPriceRangeLogging::UserRequest TA_TEXT[] =
  {
    { "TAText/Kwd/B1,TAText/Kwd/B2", "TAText/TAG/COMMON",
      COUNTRY, 0, "TAText/BP/B1,TAText/BP/B2",
      "TAText/CC/Text1,TAText/CC/Text2"},
  };
  
  process_requests(TA_TEXT);

  const  ChannelPriceRangeLogging::UserRequest TA_CHANNEL[] =
  {
    { "TAChannel/Kwd/B1,TAChannel/Kwd/B2", "TAChannel/TAG/COMMON",
      COUNTRY, 0, "TAChannel/BP/B1,TAChannel/BP/B2",
      "TAChannel/CC/Channel1,TAChannel/CC/Channel2"},
  };

  process_requests(TA_CHANNEL);

  const  ChannelPriceRangeLogging::UserRequest TA_MIXED[] =
  {
    { "TAMixed/Kwd/B1,TAMixed/Kwd/B2", "TAMixed/TAG/COMMON",
      COUNTRY, 0, "TAMixed/BP/B1,TAMixed/BP/B2",
      "TAMixed/CC/Text1,TAMixed/CC/Channel1"},
  };

  process_requests(TA_MIXED);

  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
  
}

void ChannelPriceRangeLogging::tag_adjustment()
{
  Stats stats;
  Diffs diffs;
  
  double ecpmAdj = fetch_float("TagAdjustment/ECPM");
  unsigned long channel1 = fetch_int("TagAdjustment/B1");
  unsigned long channel2 = fetch_int("TagAdjustment/B2");
  unsigned long channel3 = fetch_int("TagAdjustment/B3");
  
  const StatKey TAG_ADJUSTMENT_STATS[] =
  {
    { today,  default_colo_, channel1, 0, 0, ecpmAdj, 1,  1 },
    { today,  default_colo_, channel2, 0, 0, ecpmAdj, 1,  1 },
    { today,  default_colo_, channel3, 0, 0, ecpmAdj, 0,  0 }
  };
  
  add_stats(TAG_ADJUSTMENT_STATS, stats, diffs);

  const  ChannelPriceRangeLogging::UserRequest TAG_ADJUSTMENT[] =
  {
    { "TagAdjustment/Kwd/B1", "TagAdjustment/TAG/COMMON",
      COUNTRY, 0, "TagAdjustment/BP/B1", "TagAdjustment/CC/Display1"},
    { "TagAdjustment/Kwd/B2", "TagAdjustment/TAG/COMMON",
      COUNTRY, 0, "TagAdjustment/BP/B2", "TagAdjustment/CC/Channel1"},
    { "TagAdjustment/Kwd/B3", "TagAdjustment/TAG/COMMON",
      COUNTRY, 0, "TagAdjustment/BP/B3", "TagAdjustment/CC/Text1"},
  };
  
  process_requests(TAG_ADJUSTMENT);
  
  ADD_WAIT_CHECKER(
    "Check ChannelInventoryByCpm",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

template<size_t Count>
void
ChannelPriceRangeLogging::process_requests(
  const UserRequest(&requests)[Count])
{

  AdClient client(AdClient::create_user(this));
  
  for (size_t i = 0; i < Count; ++i)
  {
    NSLookupRequest request;
    request.referer_kw = map_objects(requests[i].referer_kws);
    if (requests[i].tid)
    {
      request.tid = fetch_string(requests[i].tid);
    }
    if (requests[i].colo)
    {
      request.colo = fetch_string(requests[i].colo);
    }
    request.loc_name = requests[i].country;
    request.debug_time = today;

    client.process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        requests[i].expected_channels,
        client.debug_info.trigger_channels).check(),
      "trigger_channels check#" + strof(i));

    if (requests[i].expected_ccids)
    {
      std::list<std::string> expected;
      fetch_objects(
        std::inserter(expected, expected.begin()),
        requests[i].expected_ccids);
      
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          AutoTest::SelectedCreativesCCID(client)).check(),
        "Check CC#"  + strof(i));
      
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          "0",
          client.debug_info.ccid).check(),
        "Check CC#"  + strof(i));
    }
  }
}

template<size_t Count>
void
ChannelPriceRangeLogging::add_stats(
  const StatKey(&stats)[Count],
  Stats& stats_container,
  Diffs& diffs_container)
{
  for (size_t i = 0; i < Count; ++i)
  {
    Stat stat;
    stat.key().
      channel_id(stats[i].channel_id).
      sdate(today).
      ecpm(stats[i].ecpm);

    if (stats[i].country_code)
    {
      stat.key().country_code(stats[i].country_code);
    }
   
    if (stats[i].creative_size)
    {
      stat.key().creative_size_id(stats[i].creative_size);
    }

    if (stats[i].colo_id)
    {
      stat.key().colo_id(stats[i].colo_id);
    }
    stat.description("#" + strof(i+1));
    stat.select(pq_conn_);
    stats_container.push_back(stat);

    diffs_container.push_back(
      Diff().user_count(stats[i].user_count).impops(stats[i].impops));
  }
}

