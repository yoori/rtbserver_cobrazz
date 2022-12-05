#include <Generics/Rand.hpp>

#include "ExpressionChannelPerformancesTest.hpp"

REFLECT_UNIT(ExpressionChannelPerformancesTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest:: NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;

  enum CheckUrlEnum
  {
    CUE_Impression = 1,  // DebugInfo must contain track_pixel_url
    CUE_Click      = 2,  // DebugInfo must contain click_url
    CUE_Action     = 4   // DebugInfo must contain adv_track_url
  };

  const ExpressionChannelPerformancesTest::TestDescription TestData[] =
  {
    // Test case diffs
    //   ChannelUsageStatsTotal
    //     last_use colo_id channel_id  imps  clicks actions revenue
    //     today       1    channel#1  +525   +175    +35    +26.3
    //     today       1    channel#2  +525   +175    +35    +26.3
    //     today       1  expression#1 +525   +175    +35    +26.3
    //     today       1  expression#2 +525   +175    +35    +26.3
    //   ChannelUsageStatsHourly
    //     sdate colo_id channel_id  ccg_id   imps  clicks actions revenue
    //     today    1    channel#1  display#1 +525   +175    +35    +26.3
    //     today    1    channel#2  display#1 +525   +175    +35    +26.3
    //     today    1  expression#1 display#1 +525   +175    +35    +26.3
    //     today    1  expression#2 display#1 +525   +175    +35    +26.3
    //   ExpressionPerformance
    //     sdate cc_id  colo_id       expression      imps_verified clicks actions
    //     today ccid#1     1        expression#2         +525      +175    +35
    {
       "1. Display CCG with default currency.",
       1050,
       0,
       "KW/1,KW/2",
       0,
       "DisplayTagId/1",
       "unit-test-imp",
       "DisplayCC/1",
       0,
       CUE_Impression | CUE_Click | CUE_Action,
       true
    },
    //   ChannelUsageStatsTotal
    //     last_use colo_id channel_id   imps  clicks actions revenue
    //     today    colo#1  channel#2   +1155   +385    +77    +1.16
    //     today    colo#1  channel#3   +1155   +385    +77    +1.16
    //     today    colo#1 expression#1 +1155   +385    +77    +1.16
    //     today    colo#1 expression#2 +1155   +385    +77    +1.16
    //   ChannelUsageStatsHourly
    //     sdate colo_id channel_id   ccg_id     imps  clicks actions revenue
    //     today colo#1  channel#2   display#2  +1155   +385    +77    +1.16
    //     today colo#1  channel#3   display#2  +1155   +385    +77    +1.16
    //     today colo#1 expression#1 display#2  +1155   +385    +77    +1.16
    //     today colo#1 expression#2 display#2  +1155   +385    +77    +1.16
    //   ExpressionPerformance
    //     sdate cc_id  colo_id       expression          imps_verified clicks actions
    //     today ccid#2 colo#1  expression#2 | channel#3     +1155     +385    +77
    {
       "2. Display CCG with NOT default currency.",
       1155,
       "Colocation",
       "KW/2",
       "URL/1",
       "DisplayTagId/2",
       "unit-test",
       "DisplayCC/2",
       0,
       CUE_Click | CUE_Action,
       false
    },
    //   ChannelUsageStatsTotal
    //     last_use colo_id channel_id   imps  clicks actions revenue
    //     tomorrow   1     channel#1   +1800     +0    +0      +126
    //     tomorrow   1     channel#3   +900     +0    +0       +36
    //     tomorrow   1    expression#2 +900     +0    +0       +90
    //     tomorrow   1    expression#3 +900     +0    +0       +36
    //   ChannelUsageStatsHourly
    //     sdate   colo_id channel_id   ccg_id  imps  clicks actions revenue
    //     tomorrow   1     channel#1   text#1  +900    +0    +0      +90
    //     tomorrow   1     channel#1   text#2  +900    +0    +0      +36
    //     tomorrow   1     channel#3   text#2  +900    +0    +0      +36
    //     tomorrow   1    expression#2 text#1  +900    +0    +0      +90
    //     tomorrow   1    expression#3 text#2  +900    +0    +0      +36
    //   ExpressionPerformance
    //     sdate    cc_id  colo_id      expression    imps_verified clicks actions
    //     tomorrow tacc#1    1         channel#1         +900        +0     +0
    //     tomorrow tacc#2    1    channel#1 & channel#3  +900        +0     +0
    {
       "3. Text CCG. One triggers for 2 CCGs.",
       900,
       0,
       "KW/1",
       "URL/1",
       "TATagId/1",
       "unit-test",
       "TACC/1,TACC/2",
       24*60*60,
       0,
       false
    },

    //   ChannelUsageStatsTotal
    //       last_use  colo_id channel_id   imps  clicks actions revenue
    //     2 day later    1     channel#2   +600     +0    +0      +60
    //     2 day later    1     channel#4   +600     +0    +0      +24 
    //     2 day later    1    expression#2 +600     +0    +0      +60
    //     2 day later    1    expression#3 +600     +0    +0      +24
    //   ChannelUsageStatsHourly
    //        sdate    colo_id channel_id   ccg_id  imps  clicks actions revenue
    //     2 day later    1     channel#2   text#1  +600     +0    +0      +60
    //     2 day later    1     channel#4   text#2  +600     +0    +0      +24
    //     2 day later    1    expression#2 text#1  +600     +0    +0      +60
    //     2 day later    1    expression#3 text#2  +600     +0    +0      +24
    //   ExpressionPerformance
    //         sdate   cc_id  colo_id      expression    imps_verified clicks actions
    //     2 day later tacc#1    1         channel#2         +600        +0     +0
    //     2 day later tacc#2    1         channel#4         +600        +0     +0
    {
       "4. Text CCG. One triggers for each CCGs.",
       600,
       0,
       "KW/2,KW/3",
       0,
       "TATagId/1",
       "unit-test",
       "TACC/1,TACC/2",
       48*60*60,
       0,
       false
    },
    //   ChannelUsageStatsTotal
    //       last_use  colo_id channel_id   imps  clicks actions revenue
    //     3 day later    1     channel#1   +300     +0    +0      +42
    //     3 day later    1     channel#2   +300     +0    +0      +30 
    //     3 day later    1     channel#3   +300     +0    +0      +12
    //     3 day later    1     channel#4   +300     +0    +0      +12
    //     3 day later    1    expression#2 +300     +0    +0      +30
    //     3 day later    1    expression#3 +300     +0    +0      +12
    //   ChannelUsageStatsHourly
    //        sdate    colo_id channel_id   ccg_id imps  clicks actions revenue
    //     3 day later    1     channel#1   text#1 +300     +0    +0      +30
    //     3 day later    1     channel#1   text#2 +300     +0    +0      +12
    //     3 day later    1     channel#2   text#1 +300     +0    +0      +30
    //     3 day later    1     channel#3   text#2 +300     +0    +0      +12
    //     3 day later    1     channel#4   text#2 +300     +0    +0      +12
    //     3 day later    1    expression#2 text#1 +300     +0    +0      +30
    //     3 day later    1    expression#3 text#2 +300     +0    +0      +12
    //   ExpressionPerformance
    //         sdate   cc_id  colo_id         expression            imps_verified clicks actions
    //     3 day later tacc#1    1       channel#1 | channel#2          +300        +0     +0
    //     3 day later tacc#2    1  channel#1 & channel#3 | channel#4   +300        +0     +0
    {
       "5. Text CCG. Composite expressions.",
       300,
       0,
       "KW/1,KW/2,KW/3,KW/4",
       "URL/1",
       "TATagId/1",
       "unit-test",
       "TACC/1,TACC/2",
       72*60*60,
       0,
       false
    }
   
  };

  /**
   * @class SelectedCreativesCheck
   * @brief Check trigger channels on client (user)
   */
  class SelectedCreativesCheck : public AutoTest::Checker
  {
  public:
    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param test.
     * @param expected ccid names.
     */
    SelectedCreativesCheck(AutoTest::AdClient& user,
                           BaseUnit* test,
                           const char* exp_ccids) :
      user_(user),
      test_(test),
      exp_ccids_(exp_ccids)
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
      std::list<std::string> ccids;
      String::StringManip::SplitComma tokenizer(exp_ccids_);
      String::SubString token;
      while (tokenizer.get_token(token))
      {
        String::StringManip::trim(token);
        ccids.push_back(test_->fetch_string(token.str()));
      }
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          ccids,
          AutoTest::SelectedCreativesCCID(user_)).check(),
        "selected creatives");
      return true;
    }
  private:

    AutoTest::AdClient& user_;  // user
    BaseUnit* test_;            // test
    std::string exp_ccids_;    // expected ccid names
  };

  double calc_cpm_revenue(double cpm,
                          double impressions,
                          double exchange_rate = 1.0)
  {
    return (cpm * impressions) / 1000.0 / exchange_rate;
  }

  double calc_cpa_revenue(double cpa,
                          double actions,
                          double exchange_rate = 1.0)
  {
    return cpa * actions / exchange_rate;
  }
}  


bool
ExpressionChannelPerformancesTest::run_test()
{
  ch1 = fetch_string("ChannelId/01");
  ch2 = fetch_string("ChannelId/02");
  ch3 = fetch_string("ChannelId/03");
  ch4 = fetch_string("ChannelId/04");
  ch5 = fetch_string("ChannelId/05");

  expr1 = fetch_string("ExpressionId/01");
  expr2 = fetch_string("ExpressionId/02");
  expr3 = fetch_string("ExpressionId/03");

  colo = fetch_int("Colocation");
  displaycc1 = fetch_int("DisplayCC/1");
  displaycc2 = fetch_int("DisplayCC/2");
  tacc1 = fetch_int("TACC/1");
  tacc2 = fetch_int("TACC/2");
  displayccg1 = fetch_int("DisplayCCG/1");
  displayccg2 = fetch_int("DisplayCCG/2");
  taccg1 = fetch_int("TACCG/1");
  taccg2 = fetch_int("TACCG/2");
  displaycpa1 = fetch_float("DisplayCPA/1");
  displaycpa2 = fetch_float("DisplayCPA/2");
  tacpm1 = fetch_float("TACPM/1");
  tacpm2 = fetch_float("TACPM/2");
  rate = fetch_float("ExchangeRate");

  base_scenario();
  latecomer_requests();

  return true;

}

void
ExpressionChannelPerformancesTest::base_scenario()
{
  // ExpressionPerformance rows
  ORM::StatsArray<ORM::ExpressionPerformanceStats, 8> exprs;
  // Display CCG with default currency
  exprs[0].key().
    expression("(" + expr2 + ")").
    colo_id(1).
    cc_id(displaycc1).
    sdate(currentDate);
  exprs[0].description("ExpressionPerformance. Expression#1. "
    "Display CCG with default currency.");
  // Display CCG with NOT default currency
  exprs[1].key().
    expression("((" + expr2 + " | " + ch3 + "))").
    colo_id(colo).
    cc_id(displaycc2).
    sdate(currentDate);
  exprs[1].description("ExpressionPerformance. Expression#1. "
    "Display CCG with NOT default currency.");
  // Text CCG. One triggers for 2 CCGs.
  exprs[2].key().
    expression(ch1).
    colo_id(1).
    cc_id(tacc1).
    sdate(currentDate + 24*60*60);
  exprs[2].description("ExpressionPerformance. Expression#2. "
    "Text CCG. One triggers for 2 CCGs.");
  exprs[3].key().
    expression("(" + ch1 + " & " + ch3 + ")").
    colo_id(1).
    cc_id(tacc2).
    sdate(currentDate + 24*60*60);
  exprs[3].description("ExpressionPerformance. Expression#3. "
    "Text CCG. One triggers for 2 CCGs.");
  // Text CCG. One triggers for each CCGs.
  exprs[4].key().
    expression(ch2).
    colo_id(1).
    cc_id(tacc1).
    sdate(currentDate + 48*60*60);
  exprs[4].description("ExpressionPerformance. Expression#2. "
    "Text CCG. One triggers for each CCGs.");
  exprs[5].key().
    expression(ch4).
    colo_id(1).
    cc_id(tacc2).
    sdate(currentDate + 48*60*60);
  exprs[5].description("ExpressionPerformance. Expression#3. "
    "Text CCG. One triggers for each CCGs.");
  // Text CCG. Composite expressions.
  exprs[6].key().
    expression("(" + ch1 + " | " + ch2 + ")").
    colo_id(1).
    cc_id(tacc1).
    sdate(currentDate + 72*60*60);
  exprs[6].description("ExpressionPerformance. Expression#2. "
    "Text CCG. Composite expressions.");
  exprs[7].key().
    expression("((" + ch1 + " & " + ch3 + ") | " + ch4 + ")").
    colo_id(1).
    cc_id(tacc2).
    sdate(currentDate + 72*60*60);
  exprs[7].description("ExpressionPerformance. Expression#3. "
    "Text CCG. Composite expressions.");  
  exprs.select(conn);


  // ChannelUsageStatsTotal rows 
  ORM::StatsArray<ORM::ChannelPerformance, 8> channels;
  // last_use = last request debug_time
  channels[0].key().
    channel_id(fetch_int("ChannelId/01")).
    last_use(currentDate + 72*60*60);
  channels[0].description("ChannelUsageStatsTotal. Channel#1.");
  channels[1].key().
    channel_id(fetch_int("ChannelId/02")).
    last_use(currentDate + 72*60*60);
  channels[1].description("ChannelUsageStatsTotal. Channel#2.");
  channels[2].key().
    channel_id(fetch_int("ChannelId/03")).
    last_use(currentDate + 72*60*60);
  channels[2].description("ChannelUsageStatsTotal. Channel#3.");
  channels[3].key().
    channel_id(fetch_int("ChannelId/04")).
    last_use(currentDate + 72*60*60);
  channels[3].description("ChannelUsageStatsTotal. Channel#4.");
  channels[4].key().
    channel_id(fetch_int("ChannelId/05")).
    last_use(currentDate + 72*60*60);
  channels[4].description("ChannelUsageStatsTotal. Channel#5.");
  channels[5].key().
    channel_id(fetch_int("ExpressionId/01")).
    last_use(currentDate);
  channels[5].description("ChannelUsageStatsTotal. Expression#1.");
  channels[6].key().
    channel_id(fetch_int("ExpressionId/02")).
    last_use(currentDate + 72*60*60);
  channels[6].description("ChannelUsageStatsTotal. Expression#2.");
  channels[7].key().
    channel_id(fetch_int("ExpressionId/03")).
    last_use(currentDate + 72*60*60);
  channels[7].description("ChannelUsageStatsTotal. Expression#3.");
  channels.select(conn);


  // ChannelUsageStatsHourly
  ORM::StatsArray<ORM::ChannelUsageStats, 24> usage;
  // Display CCG with default currency
  usage[0].key().
    channel_id(fetch_int("ChannelId/01")).
    ccg_id(displayccg1).
    colo_id(1).
    sdate(currentDate);
  usage[0].description("ChannelUsageStatsHourly. Channel#1. "
    "Display CCG with default currency.");
  usage[1].key().
    channel_id(fetch_int("ChannelId/02")).
    ccg_id(displayccg1).
    colo_id(1).
    sdate(currentDate);
  usage[1].description("ChannelUsageStatsHourly. Channel#2. "
    "Display CCG with default currency.");
  usage[2].key().
    channel_id(fetch_int("ExpressionId/01")).
    ccg_id(displayccg1).
    colo_id(1).
    sdate(currentDate);
  usage[2].description("ChannelUsageStatsHourly. Expression#1. "
    "Display CCG with default currency.");
  usage[3].key().
    channel_id(fetch_int("ExpressionId/02")).
    ccg_id(displayccg1).
    colo_id(1).
    sdate(currentDate);
  usage[3].description("ChannelUsageStatsHourly. Expression#2. "
    "Display CCG with default currency.");
  // Display CCG with NOT default currency
  usage[4].key().
    channel_id(fetch_int("ChannelId/02")).
    ccg_id(displayccg2).
    colo_id(colo).
    sdate(currentDate);
  usage[4].description("ChannelUsageStatsHourly. Channel#2. "
    "Display CCG with NOT default currency.");
  usage[5].key().
    channel_id(fetch_int("ChannelId/03")).
    ccg_id(displayccg2).
    colo_id(colo).
    sdate(currentDate);
  usage[5].description("ChannelUsageStatsHourly. Channel#3. "
    "Display CCG with NOT default currency.");
  usage[6].key().
    channel_id(fetch_int("ExpressionId/01")).
    ccg_id(displayccg2).
    colo_id(colo).
    sdate(currentDate);
  usage[6].description("ChannelUsageStatsHourly. Expression#1. "
    "Display CCG with NOT default currency.");
  usage[7].key().
    channel_id(fetch_int("ExpressionId/02")).
    ccg_id(displayccg2).
    colo_id(colo).
    sdate(currentDate);
  usage[7].description("ChannelUsageStatsHourly. Expression#2. "
    "Display CCG with NOT default currency.");
  // Text CCG. One triggers for 2 CCGs.
  usage[8].key().
    channel_id(fetch_int("ChannelId/01")).
    ccg_id(taccg1).
    colo_id(1).
    sdate(currentDate + 24*60*60);
  usage[8].description("ChannelUsageStatsHourly. Channel#1. "
    "Text CCG. One triggers for 2 CCGs.");
  usage[9].key().
    channel_id(fetch_int("ChannelId/01")).
    ccg_id(taccg2).
    colo_id(1).
    sdate(currentDate + 24*60*60);
  usage[9].description("ChannelUsageStatsHourly. Channel#1. "
    "Text CCG. One triggers for 2 CCGs.");
  usage[10].key().
    channel_id(fetch_int("ChannelId/03")).
    ccg_id(taccg2).
    colo_id(1).
    sdate(currentDate + 24*60*60);
  usage[10].description("ChannelUsageStatsHourly. Channel#3. "
    "Text CCG. One triggers for 2 CCGs.");
  usage[11].key().
    channel_id(fetch_int("ExpressionId/02")).
    ccg_id(taccg1).
    colo_id(1).
    sdate(currentDate + 24*60*60);
  usage[11].description("ChannelUsageStatsHourly. Expression#2. "
    "Text CCG. One triggers for 2 CCGs.");
  usage[12].key().
    channel_id(fetch_int("ExpressionId/03")).
    ccg_id(taccg2).
    colo_id(1).
    sdate(currentDate + 24*60*60);
  usage[12].description("ChannelUsageStatsHourly. Expression#3. "
    "Text CCG. One triggers for 2 CCGs.");
  // Text CCG. One triggers for each CCGs.
  usage[13].key().
    channel_id(fetch_int("ChannelId/02")).
    ccg_id(taccg1).
    colo_id(1).
    sdate(currentDate + 48*60*60);
  usage[13].description("ChannelUsageStatsHourly. Channel#2. "
    "Text CCG. One triggers for each CCGs.");
  usage[14].key().
    ccg_id(taccg2).
    channel_id(fetch_int("ChannelId/04")).
    colo_id(1).
    sdate(currentDate + 48*60*60);
  usage[14].description("ChannelUsageStatsHourly. Channel#4. "
    "Text CCG. One triggers for each CCGs.");
  usage[15].key().
    channel_id(fetch_int("ExpressionId/02")).
    ccg_id(taccg1).
    colo_id(1).
    sdate(currentDate + 48*60*60);
  usage[15].description("ChannelUsageStatsHourly. Expression#2. "
    "Text CCG. One triggers for each CCGs.");
  usage[16].key().
    channel_id(fetch_int("ExpressionId/03")).
    ccg_id(taccg2).    
    colo_id(1).
    sdate(currentDate +  48*60*60);
  usage[16].description("ChannelUsageStatsHourly. Expression#3. "
    "Text CCG. One triggers for each CCGs.");
  // Text CCG. Composite expressions.
  usage[17].key().
    channel_id(fetch_int("ChannelId/01")).
    ccg_id(taccg1).
    colo_id(1).
    sdate(currentDate + 72*60*60);
  usage[17].description("ChannelUsageStatsHourly. Channel#1. "
    "Text CCG. Composite expressions.");
  usage[18].key().
    channel_id(fetch_int("ChannelId/01")).
    ccg_id(taccg2).
    colo_id(1).
    sdate(currentDate + 72*60*60);
  usage[18].description("ChannelUsageStatsHourly. Channel#1. "
    "Text CCG. Composite expressions.");
  usage[19].key().
    channel_id(fetch_int("ChannelId/02")).
    ccg_id(taccg1).
    colo_id(1).
    sdate(currentDate + 72*60*60);
  usage[19].description("ChannelUsageStatsHourly. Channel#2. "
    "Text CCG. Composite expressions.");
  usage[20].key().
    channel_id(fetch_int("ChannelId/03")).
    ccg_id(taccg2).
    colo_id(1).
    sdate(currentDate + 72*60*60);
  usage[20].description("ChannelUsageStatsHourly. Channel#3. "
    "Text CCG. Composite expressions.");
  usage[21].key().
    channel_id(fetch_int("ChannelId/04")).
    ccg_id(taccg2).
    colo_id(1).
    sdate(currentDate + 72*60*60);
  usage[21].description("ChannelUsageStatsHourly. Channel#4. "
    "Text CCG. Composite expressions.");
  usage[22].key().
    channel_id(fetch_int("ExpressionId/02")).
    ccg_id(taccg1).
    colo_id(1).
    sdate(currentDate + 72*60*60);
  usage[22].description("ChannelUsageStatsHourly. Expression#2. "
    "Text CCG. Composite expressions.");
  usage[23].key().
    channel_id(fetch_int("ExpressionId/03")).
    ccg_id(taccg2).
    colo_id(1).
    sdate(currentDate + 72*60*60);
  usage[23].description("ChannelUsageStatsHourly. Expression#3. "
    "Text CCG. Composite expressions.");
  usage.select(conn);

  for (unsigned long i=0; i < countof(TestData); ++i)
  {
    test_case(TestData[i]);
  }

  {
    add_descr_phrase("Check ExpressionPerformance table");

    const ORM::ExpressionPerformanceStats::Diffs diffs[8] =
    {
      // Display CCG with default currency
      ORM::ExpressionPerformanceStats::Diffs().
        imps(TestData[0].iter_count / 2).
        clicks(TestData[0].iter_count / 2 / 3).
        actions(TestData[0].iter_count / 2 / 3 / 5),
      // Display CCG with NOT default currency
      ORM::ExpressionPerformanceStats::Diffs().
        imps(TestData[1].iter_count).
        clicks(TestData[1].iter_count / 3).
        actions(TestData[1].iter_count / 3 / 5),
      // Text CCG. One triggers for 2 CCGs.
      ORM::ExpressionPerformanceStats::Diffs().
        imps(TestData[2].iter_count).
        clicks(0).
        actions(0),
      ORM::ExpressionPerformanceStats::Diffs().
        imps(TestData[2].iter_count).
        clicks(0).
        actions(0),
      // Text CCG. One triggers for each CCGs.
      ORM::ExpressionPerformanceStats::Diffs().
        imps(TestData[3].iter_count).
        clicks(0).
        actions(0),
      ORM::ExpressionPerformanceStats::Diffs().
        imps(TestData[3].iter_count).
        clicks(0).
        actions(0),
      // Text CCG. Composite expressions.
      ORM::ExpressionPerformanceStats::Diffs().
          imps(TestData[4].iter_count).
          clicks(0).
          actions(0),
      ORM::ExpressionPerformanceStats::Diffs().
          imps(TestData[4].iter_count).
          clicks(0).
          actions(0)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(conn, diffs, exprs)).check(),
      "ExpressionPerformance check");

  }

  {
    add_descr_phrase("Check ChannelUsageStatsTotal table");

    // Total result on last request debug_time
    const ORM::ChannelPerformance::Diffs diffs[] =
    {
      // Channel#1 
      ORM::ChannelPerformance::Diffs().
        imps(
          TestData[0].iter_count / 2 +
          2*TestData[2].iter_count +
          2*TestData[4].iter_count).
        clicks(TestData[0].iter_count / 2 / 3).
        actions(TestData[0].iter_count / 2 / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa1,
              TestData[0].iter_count / 2 / 3 / 5) +
            calc_cpm_revenue(
              tacpm1,
              TestData[2].iter_count +
              TestData[4].iter_count) +
            calc_cpm_revenue(
              tacpm2,
              TestData[2].iter_count +
              TestData[4].iter_count),
            0.1)),

      // Channel#2
      ORM::ChannelPerformance::Diffs().
        imps(
          TestData[0].iter_count / 2 +
          TestData[1].iter_count +
          TestData[3].iter_count +
          TestData[4].iter_count).
        clicks(
          TestData[0].iter_count / 2 / 3 +
          TestData[1].iter_count / 3).
        actions(
          TestData[0].iter_count / 2 / 3 / 5 +
          TestData[1].iter_count / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa1,
              TestData[0].iter_count / 2 / 3 / 5) +
            calc_cpa_revenue(
              displaycpa2,
              TestData[1].iter_count / 3 / 5,
              rate) +
            calc_cpm_revenue(
              tacpm1,
              TestData[3].iter_count +
              TestData[4].iter_count),
            0.1)),

      // Channel#3
      ORM::ChannelPerformance::Diffs().
        imps(
          TestData[2].iter_count +
          TestData[1].iter_count +
          TestData[4].iter_count).
        clicks(TestData[1].iter_count / 3).
        actions(TestData[1].iter_count / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa2,
              TestData[1].iter_count / 3 / 5,
              rate) +
            calc_cpm_revenue(
              tacpm2,
              TestData[2].iter_count +
              TestData[4].iter_count),
            0.1)),

      // Channel#4
      ORM::ChannelPerformance::Diffs().
        imps(
          TestData[3].iter_count +
          TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm2,
              TestData[3].iter_count +
              TestData[4].iter_count),
            0.1)),

      // Channel#5
      ORM::ChannelPerformance::Diffs(0),

      // Expression#1
      ORM::ChannelPerformance::Diffs().
        imps(
          TestData[0].iter_count / 2 +
          TestData[1].iter_count).
        clicks(
          TestData[0].iter_count / 2 / 3 +
          TestData[1].iter_count / 3).
        actions(
          TestData[0].iter_count / 2 / 3 / 5 +
          TestData[1].iter_count / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa1,
              TestData[0].iter_count / 2 / 3 / 5) +
            calc_cpa_revenue(
              displaycpa2,
              TestData[1].iter_count / 3 / 5,
              rate),
            0.1)),

      // Expression#2
      ORM::ChannelPerformance::Diffs().
        imps(
          TestData[0].iter_count / 2 +
          TestData[1].iter_count +
          TestData[2].iter_count +
          TestData[3].iter_count +
          TestData[4].iter_count).
        clicks(
          TestData[0].iter_count / 2 / 3 +
          TestData[1].iter_count / 3).
        actions(
          TestData[0].iter_count / 2 / 3 / 5 +
          TestData[1].iter_count / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa1,
              TestData[0].iter_count / 2 / 3 / 5) +
            calc_cpa_revenue(
              displaycpa2,
              TestData[1].iter_count / 3 / 5,
              rate) +
            calc_cpm_revenue(
              tacpm1,
              TestData[2].iter_count +
              TestData[3].iter_count +
              TestData[4].iter_count),
            0.1)),

      // Expression#3
      ORM::ChannelPerformance::Diffs().
        imps(
          TestData[2].iter_count +
          TestData[3].iter_count +
          TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm2,
              TestData[2].iter_count +
              TestData[3].iter_count +
              TestData[4].iter_count),
            0.1))
    };
    
    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn, diffs, channels)).check(),
        "ChannelUsageStatsTotal check");

  }
  
  {
    add_descr_phrase("Check ChannelUsageStatsHourly table");
    const ORM::ChannelUsageStats::Diffs diffs[] =
    {
      // Display CCG with default currency
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[0].iter_count / 2).
        clicks(TestData[0].iter_count / 2 / 3).
        actions(TestData[0].iter_count / 2 / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa1,
              TestData[0].iter_count / 2 / 3 / 5),
            0.1)),
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[0].iter_count / 2).
        clicks(TestData[0].iter_count / 2 / 3).
        actions(TestData[0].iter_count / 2 / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa1,
              TestData[0].iter_count / 2 / 3 / 5),
            0.1)),
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[0].iter_count / 2).
        clicks(TestData[0].iter_count / 2 / 3).
        actions(TestData[0].iter_count / 2 / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa1,
              TestData[0].iter_count / 2 / 3 / 5),
            0.1)),
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[0].iter_count / 2).
        clicks(TestData[0].iter_count / 2 / 3).
        actions(TestData[0].iter_count / 2 / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa1,
              TestData[0].iter_count / 2 / 3 / 5),
            0.1)),
      // Display CCG with NOT default currency
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[1].iter_count).
        clicks(TestData[1].iter_count / 3).
        actions(TestData[1].iter_count / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa2,
              TestData[1].iter_count / 3 / 5,
              rate),
            0.1)),
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[1].iter_count).
        clicks(TestData[1].iter_count / 3).
        actions(TestData[1].iter_count / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa2,
              TestData[1].iter_count / 3 / 5,
              rate),
            0.1)),
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[1].iter_count).
        clicks(TestData[1].iter_count / 3).
        actions(TestData[1].iter_count / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa2,
              TestData[1].iter_count / 3 / 5,
              rate),
            0.1)),
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[1].iter_count).
        clicks(TestData[1].iter_count / 3).
        actions(TestData[1].iter_count / 3 / 5).
        revenue(
          ORM::stats_diff_type(
            calc_cpa_revenue(
              displaycpa2,
              TestData[1].iter_count / 3 / 5,
              rate),
            0.1)),
      // Text CCG. One triggers for 2 CCGs.
      //  Channel#1
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[2].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm1,
              TestData[2].iter_count),
            0.1)),
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[2].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm2,
              TestData[2].iter_count),
            0.1)),
      //  Channel#3
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[2].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm2,
              TestData[2].iter_count),
            0.1)),
      //  Expression#2
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[2].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm1,
              TestData[2].iter_count),
            0.1)),
      //  Expression#3
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[2].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm2,
              TestData[2].iter_count),
            0.1)),
      // Text CCG. One triggers for each CCGs.
      //  Channel#2
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[3].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm1,
              TestData[3].iter_count),
            0.1)),
      //  Channel#4
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[3].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm2,
              TestData[3].iter_count),
            0.1)),
      //  Expression#2
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[3].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm1,
              TestData[3].iter_count),
            0.1)),
      //  Expression#3
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[3].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
              tacpm2,
              TestData[3].iter_count),
            0.1)),
      // Text CCG. Composite expressions.
      //  Channel#1
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
            tacpm1,
            TestData[4].iter_count),
            0.1)),
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
            tacpm2,
            TestData[4].iter_count),
            0.1)),
      //  Channel#2
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
            tacpm1,
            TestData[4].iter_count),
            0.1)),
      //  Channel#3
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
            tacpm2,
            TestData[4].iter_count),
            0.1)),
      //  Channel#4
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
            tacpm2,
            TestData[4].iter_count),
            0.1)),
      //  Expression#2
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
            tacpm1,
            TestData[4].iter_count),
            0.1)),
      //  Expression#3
      ORM::ChannelUsageStats::Diffs().
        imps(TestData[4].iter_count).
        clicks(0).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            calc_cpm_revenue(
            tacpm2,
            TestData[4].iter_count),
            0.1))
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn, diffs, usage)).check(),
        "ChannelUsageStatsHourly check");
  }
}


void
ExpressionChannelPerformancesTest::latecomer_requests()
{

  add_descr_phrase("Latecomer impressions");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      TestData[0].iter_count / 2,
      impressions.size()).check(),
    "latecomer impressions count");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      TestData[0].iter_count / 2 -
      TestData[0].iter_count / 2 / 3,
      clicks.size()).check(),
    "latecomer clicks count");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      TestData[0].iter_count / 2 -
      TestData[0].iter_count / 2 / 5,
      actions.size()).check(),
    "latecomer actions count");

  ORM::ExpressionPerformanceStats exprs;
  exprs.key().
    expression("(" + expr2 + ")").
    colo_id(1).
    cc_id(displaycc1).
    sdate(currentDate);
  exprs.select(conn);

  ORM::StatsArray<ORM::ChannelPerformance, 4> channels;
  channels[0].key().
    channel_id(fetch_int("ChannelId/01")).
    last_use(currentDate + 72*60*60);
  channels[1].key().
    channel_id(fetch_int("ChannelId/02")).
    last_use(currentDate + 72*60*60);
  channels[2].key().
    channel_id(fetch_int("ExpressionId/01")).
    last_use(currentDate);
  channels[3].key().
    channel_id(fetch_int("ExpressionId/02")).
    last_use(currentDate + 72*60*60);
  channels.select(conn);

  ORM::StatsArray<ORM::ChannelUsageStats, 4> usage;
  usage[0].key().
    channel_id(fetch_int("ChannelId/01")).
    ccg_id(displayccg1).
    colo_id(1).
    sdate(currentDate);
  usage[1].key().
    channel_id(fetch_int("ChannelId/02")).
    ccg_id(displayccg1).
    colo_id(1).
    sdate(currentDate);
  usage[2].key().
    channel_id(fetch_int("ExpressionId/01")).
    ccg_id(displayccg1).
    colo_id(1).
    sdate(currentDate);
  usage[3].key().
    channel_id(fetch_int("ExpressionId/02")).
    ccg_id(displayccg1).
    colo_id(1).
    sdate(currentDate);
  usage.select(conn);

  // Send latecomer impressions
  for (RequestList::iterator req_pair =
       impressions.begin(); req_pair != impressions.end(); ++req_pair)
  {
    req_pair->first.process_request(req_pair->second, "Impression request");
  }

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn,
        ORM::ExpressionPerformanceStats::Diffs().
          imps(impressions.size()).
          clicks(impressions.size() / 3).
          actions(impressions.size() / 3 / 5),
        exprs)).check(),
    "ExpressionPerformance latecomer impressions check");


  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_each_diff_checker(
        conn,
        ORM::ChannelPerformance::Diffs().
          imps(impressions.size()).
          clicks(impressions.size() / 3).
          actions(impressions.size() / 3 / 5).
          revenue(
            ORM::stats_diff_type(
              calc_cpa_revenue(
                displaycpa1,
                impressions.size() / 3 / 5),
              0.1)),
        channels)).check(),
    "ChannelUsageStatsTotal latecomer impressions check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_each_diff_checker(
        conn,
        ORM::ChannelUsageStats::Diffs().
          imps(impressions.size()).
          clicks(impressions.size() / 3).
          actions(impressions.size() / 3 / 5).
          revenue(
            ORM::stats_diff_type(
              calc_cpa_revenue(
                displaycpa1,
                impressions.size() / 3 / 5),
              0.1)),
        usage)).check(),
    "ChannelUsageStatsHourly latecomer impressions check");


  add_descr_phrase("Latecomer clicks & actions");

  exprs.select(conn);
  channels.select(conn);
  usage.select(conn);

  // Send latecomer clicks
  for (RequestList::iterator req_pair =
        clicks.begin(); req_pair != clicks.end(); ++req_pair)
  {
    req_pair->first.process_request(req_pair->second, "Click request");
  }

  // Send latecomer actions
  for (RequestList::iterator req_pair =
        actions.begin(); req_pair != actions.end(); ++req_pair)
  {
    req_pair->first.process_request(req_pair->second, "Action request");
  }

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn,
        ORM::ExpressionPerformanceStats::Diffs().
          imps(0).
          clicks(clicks.size()).
          actions(
            actions.size() +
            /* confirmed actions from loop#1 */ 
            clicks.size() / 5),
        exprs)).check(),
    "ExpressionPerformance latecomer clicks & actions check");

    FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_each_diff_checker(
        conn,
        ORM::ChannelPerformance::Diffs().
          imps(0).
          clicks(clicks.size()).
          actions(
            actions.size() +
            /* confirmed actions from loop#1 */ 
            clicks.size() / 5).
          revenue(
            ORM::stats_diff_type(
              calc_cpa_revenue(
                displaycpa1,
                actions.size() +
                clicks.size() / 5),
              0.1)),
        channels)).check(),
    "ChannelUsageStatsTotal latecomer clicks & actions check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_each_diff_checker(
        conn,
        ORM::ChannelUsageStats::Diffs().
          imps(0).
          clicks(clicks.size()).
          actions(
            actions.size() +
            /* confirmed actions from loop#1 */ 
            clicks.size() / 5).
          revenue(
            ORM::stats_diff_type(
              calc_cpa_revenue(
                displaycpa1,
                actions.size() +
                clicks.size() / 5),
              0.1)),
        usage)).check(),
    "ChannelUsageStatsHourlyn latecomer clicks & actions check");

}


void
ExpressionChannelPerformancesTest::test_case(const TestDescription& test)
{
  add_descr_phrase(test.description);
  NSLookupRequest request;
  set_request_param(request.colo, test.colo);
  set_request_param(request.referer_kw, test.referer_kw);
  set_request_param(request.referer, test.referer);
  set_request_param(request.tid, test.tid);
  request.format = test.format;
  request.debug_time = currentDate + test.time_ofset;
  for (unsigned int i = 0; i < test.iter_count; ++i)
  {
    AdClient client(AdClient::create_user(this));
    
    client.process_request(request);


    std::string imp_url = client.debug_info.track_pixel_url;
    std::string click_url = client.debug_info.click_url;
    std::string action_url = client.debug_info.action_adv_url;
    
    FAIL_CONTEXT(
      SelectedCreativesCheck(
        client,
        this,
        test.ccids).check(),
      "Check creative selection#" + strof(i));


    if (test.check_url & CUE_Impression)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !imp_url.empty()),
        "must have valid track_pixel_url");
      if ( !test.store_urls || i % 2 == 0)
      {
        client.process_request(imp_url, "Impression");
      }
      else if (test.store_urls)
      {
        impressions.push_back(RequestPair(client, imp_url));
        
        if (i % 3 != 2)
        {
          clicks.push_back(RequestPair(client, click_url));          
        }

        if (i % 5 != 4)
        {
          actions.push_back(RequestPair(client, action_url));          
        }
      }
    }
    
    if (test.check_url & CUE_Click)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !click_url.empty()),
        "must have valid click_url");
         
      if (i % 3 == 2)
      {
        client.process_request(click_url, "Click");
      }
    }

    if (test.check_url & CUE_Action)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !action_url.empty()),
        "must have valid action_adv_url");
      
      if (i % 5 == 4)
      {
        client.process_request(action_url, "Action");
      }
    }
  }  
}

template <class ParamType>
void
ExpressionChannelPerformancesTest::set_request_param(
  ParamType& request_param,
  const char* param_name)
{
  if (param_name)
  {
    std::string p(param_name);
    std::string param_value;
    String::StringManip::SplitComma tokenizer(p);
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      String::StringManip::trim(token);
      std::string value = param_value.empty()?
          fetch_string(token.str()):
          "," +  fetch_string(token.str());
      param_value+=value;
    }
    request_param = param_value;
  }
}
