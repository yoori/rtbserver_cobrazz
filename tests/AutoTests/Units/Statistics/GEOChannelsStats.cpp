
#include "GEOChannelsStats.hpp"

REFLECT_UNIT(GEOChannelsStats) (
  "Statistics",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;

  namespace ORM = AutoTest::ORM;

  enum ActionEnum
  {
    AE_CLICK  = 1,
    AE_ACTION = 2
  };

  const double DISPLAY_CTR = 0.01;
  const double DISPLAY_AR = 0.01;
  const double TEXT_CTR = 0.1;
}

bool
GEOChannelsStats::run_test()
{
  add_descr_phrase("Initialize.");

  double display_cpm = fetch_float("DISPLAYCPM");
  double display_cpa = fetch_float("DISPLAYCPA");
  double text_cpc1 = fetch_float("TEXTCPC1");
  double text_cpc2 = fetch_float("TEXTCPC2");

  const char* GEO_CHANNELS[] =
  {
    "STATE1CH", "STATE2CH", "STATE3CH",
    "CITY1CH", "CITY2CH", "CITY3CH",
  };

  ORM::StatsList<ORM::ChannelInventoryEstimStats> inventory_estim;

  for (size_t i = 0; i < countof(GEO_CHANNELS); ++i)
  {
    ORM::ChannelInventoryEstimStats cie;
    cie.key().channel_id(fetch_int(GEO_CHANNELS[i]));
    cie.select(pq_conn_);
    inventory_estim.push_back(cie);
  }

  // ChannelInventory
  ORM::StatsArray<ORM::ChannelInventoryStats, 4> inventory;
  inventory[0].key().
    channel_id(fetch_int("STATE1CH")).
    sdate(testtime);
  inventory[0].description(
    "ChannelInventory. State 'London, City of'");
  inventory[1].key().
    channel_id(fetch_int("CITY1CH")).
    sdate(testtime);
  inventory[1].description(
    "ChannelInventory. City 'London'");
  inventory[2].key().
    channel_id(fetch_int("CITY2CH")).
    sdate(testtime);
  inventory[2].description(
    "ChannelInventory. City 'Middlesborough'");
  inventory[3].key().
    channel_id(fetch_int("CITY3CH")).
    sdate(testtime);
  inventory[3].description(
    "ChannelInventory. City 'Aberfoyle'");
  inventory.select(pq_conn_);

  // ChannelImpInventory
  ORM::StatsArray<ORM::ChannelImpInventory, 6> imp_inventory;
  imp_inventory[0].key().
    channel_id(fetch_int("STATE1CH")).
    ccg_type("D").
    colo_id(1).
    sdate(testtime);
  imp_inventory[0].description(
    "ChannelImpInventory. "
    "State 'London, City of', Display.");
  imp_inventory[1].key().
    channel_id(fetch_int("STATE1CH")).
    ccg_type("T").
    colo_id(1).
    sdate(testtime);
  imp_inventory[1].description(
    "ChannelImpInventory. "
    "State 'London, City of', Text.");
  imp_inventory[2].key().
    channel_id(fetch_int("CITY1CH")).
    ccg_type("D").
    colo_id(1).
    sdate(testtime);
  imp_inventory[2].description(
    "ChannelImpInventory. "
    "City 'London', Display.");
  imp_inventory[3].key().
    channel_id(fetch_int("CITY1CH")).
    ccg_type("T").
    colo_id(1).
    sdate(testtime);
  imp_inventory[3].description(
    "ChannelImpInventory. "
    "City 'London', Text.");
  imp_inventory[4].key().
    channel_id(fetch_int("CITY3CH")).
    ccg_type("D").
    colo_id(1).
    sdate(testtime);
  imp_inventory[4].description(
    "ChannelImpInventory. "
    "City 'Aberfoyle', Display.");
  imp_inventory[5].key().
    channel_id(fetch_int("CITY3CH")).
    ccg_type("T").
    colo_id(1).
    sdate(testtime);
  imp_inventory[5].description(
    "ChannelImpInventory. "
    "City 'Aberfoyle', Text.");
  imp_inventory.select(pq_conn_);


  // ChannelPerformance
  ORM::StatsArray<ORM::ChannelPerformance, 3> performance;
  performance[0].key().
    channel_id(fetch_int("STATE1CH")).
    last_use(testtime);
  performance[0].description(
    "ChannelPerformance. "
    "State 'London, City of'.");
  performance[1].key().
    channel_id(fetch_int("CITY1CH")).
    last_use(testtime);
  performance[1].description(
    "ChannelPerformance. "
    "City 'London '.");
  performance[2].key().
    channel_id(fetch_int("CITY3CH")).
    last_use(testtime);
  performance[2].description(
    "ChannelPerformance. "
    "City 'Aberfoyle'.");
  performance.select(pq_conn_);

  // ChannelUsage
  ORM::StatsArray<ORM::ChannelUsageStats, 3> channel_usage;
  channel_usage[0].key().
    channel_id(fetch_int("STATE1CH")).
    colo_id(1).
    sdate(testtime);
  channel_usage[0].description(
    "ChannelUsage. "
    "State 'London, City of'.");
  channel_usage[1].key().
    channel_id(fetch_int("CITY1CH")).
    colo_id(1).
    sdate(testtime);
  channel_usage[1].description(
    "ChannelUsage. "
    "City 'London '.");
  channel_usage[2].key().
    channel_id(fetch_int("CITY3CH")).
    colo_id(1).
    sdate(testtime);
  channel_usage[2].description(
    "ChannelUsage. "
    "City 'Aberfoyle'.");
  channel_usage.select(pq_conn_);

  // ChannelInventoryByCpm
  ORM::StatsArray<ORM::ChannelInventoryByCPMStats,  6> inventory_by_cpm;
  inventory_by_cpm[0].key().
    channel_id(fetch_int("STATE1CH"));
  inventory_by_cpm[0].description("ChannelInventoryByCpm. "
    "State 'London, City of', Display.");
  inventory_by_cpm[1].key().
    channel_id(fetch_int("STATE1CH"));
  inventory_by_cpm[1].description("ChannelInventoryByCpm. "
    "State 'London, City of', Text.");
  inventory_by_cpm[2].key().
    channel_id(fetch_int("CITY1CH"));
  inventory_by_cpm[2].description("ChannelInventoryByCpm. "
    "City 'London', Display.");
  inventory_by_cpm[3].key().
    channel_id(fetch_int("CITY1CH"));
  inventory_by_cpm[3].description("ChannelInventoryByCpm. "
    "City 'London', Text.");
  inventory_by_cpm[4].key().
    channel_id(fetch_int("CITY2CH"));
  inventory_by_cpm[4].description("ChannelInventoryByCpm. "
    "City 'Middlesborough', Display.");
  inventory_by_cpm[5].key().
    channel_id(fetch_int("CITY3CH"));
  inventory_by_cpm[5].description("ChannelInventoryByCpm. "
    "City 'Aberfoyle', Display.");
  inventory_by_cpm.select(pq_conn_);

  // ExpressionPerformance
  ORM::StatsArray<ORM::ExpressionPerformanceStats, 3> expr_perf;
  expr_perf[0].key().
    cc_id(fetch_int("DISPLAYCC-CPM")).
    expression(fetch_string("DISPLAYCHANNEL"));
  expr_perf[0].description(
    "ExpressionPerformance. "
    "CPM CC, City 'London' & display channel expression.");

  expr_perf[1].key().
      cc_id(fetch_int("DISPLAYCC-CPA")).
      expression(fetch_string("DISPLAYCHANNEL"));
  expr_perf[1].description(
    "ExpressionPerformance. "
    "CPA CC, State 'London, City Of' & display channel expression.");

  expr_perf[2].key().
    cc_id(fetch_int("DISPLAYCC-CPC")).
    expression(fetch_string("DISPLAYCHANNEL"));
  expr_perf[2].description(
    "ExpressionPerformance. "
    "CPC CC, Only display channel expression.");

  expr_perf.select(pq_conn_);
    
  // SiteChannelStats
  ORM::StatsArray<ORM::SiteChannelStats, 3> sitechannel;
  sitechannel[0].key().
    tag_id(fetch_int("DISPLAYTID-CPM")).
    channel_id(fetch_int("CITY1CH")).
    sdate(testtime);
  sitechannel[0].description(
    "SiteChannelStats. "
    "City 'London', Display CPM tag.");
  sitechannel[1].key().
    tag_id(fetch_int("TEXTTID")).
    channel_id(fetch_int("CITY1CH")).
    sdate(testtime);
  sitechannel[1].description(
    "SiteChannelStats. "
    "City 'London', Text tag.");
  sitechannel[2].key().
    tag_id(fetch_int("DISPLAYTID-CPA")).
    channel_id(fetch_int("STATE1CH")).
    sdate(testtime);
  sitechannel[2].description(
    "SiteChannelStats. "
    "State 'London, City of', Display CPA tag.");
  sitechannel.select(pq_conn_);

  make_requests();

  add_descr_phrase("Check statistics.");

  {
    typedef ORM::ChannelInventoryStats::Diffs Diffs;
    const Diffs diffs[4] =
    {
      Diffs().// 'London, City of' state
        active_users(2).
        total_users(2),
      Diffs().// 'London' city
        active_users(2).
        total_users(2),
      Diffs().// 'Middlesborough' city
        active_users(1).
        total_users(1),
      Diffs().// 'Aberfoyle' city
        active_users(1).
        total_users(1)
    };

    FAIL_CONTEXT(AutoTest::wait_checker(
        AutoTest::stats_diff_checker(pq_conn_, diffs, inventory)).check(),
      "ChannelInventory check");
  }

  {
    // ChannelImpInventory
    const ORM::ChannelImpInventory::Diffs diffs[6] =
    {
      // 'London, City of' state, Display
      ORM::ChannelImpInventory::Diffs().
        imps(1).
        clicks(1).
        actions(1).
        revenue(display_cpa). 
        impops_user_count(2).
        imps_user_count(1).
        imps_other(2).
        imps_other_user_count(2).
        imps_other_value((text_cpc1 + text_cpc2) * TEXT_CTR +
          display_cpm / 1000).
        impops_no_imp(0).
        impops_no_imp_user_count(0).
        impops_no_imp_value(0),
      // 'London, City of' state, Text
      ORM::ChannelImpInventory::Diffs().
        imps(0).
        clicks(0).
        actions(0).
        revenue(0). 
        impops_user_count(2).
        imps_user_count(0).
        imps_value(0).
        imps_other(6).
        imps_other_user_count(2).
        impops_no_imp(0).
        impops_no_imp_user_count(0).
        impops_no_imp_value(0),
      // 'London' city, Display
      ORM::ChannelImpInventory::Diffs().
        imps(1).
        clicks(1).
        actions(0).
        revenue(display_cpm / 1000). 
        impops_user_count(2).
        imps_user_count(1).
        imps_value(display_cpm / 1000).
        imps_other(1).
        imps_other_user_count(1).
        imps_other_value(
          (text_cpc1 + text_cpc2) * TEXT_CTR).
        impops_no_imp(0).
        impops_no_imp_user_count(0).
        impops_no_imp_value(0),
      // 'London' city, Text
      ORM::ChannelImpInventory::Diffs().
        imps(2).
        clicks(2).
        actions(0).
        revenue(text_cpc1 + text_cpc2). 
        impops_user_count(2).
        imps_user_count(1).
        imps_value((text_cpc1 + text_cpc2) * TEXT_CTR).
        imps_other(2).
        imps_other_user_count(1).
        imps_other_value(display_cpm / 1000).
        impops_no_imp(0).
        impops_no_imp_user_count(0).
        impops_no_imp_value(0),
      // 'Aberfoyle' city, Display
      ORM::ChannelImpInventory::Diffs().
        imps(0).
        clicks(0).
        actions(0).
        revenue(0). 
        impops_user_count(1).
        imps_user_count(0).
        imps_value(0).
        imps_other(0).
        imps_other_user_count(0).
        imps_other_value(0).
        impops_no_imp(1).
        impops_no_imp_user_count(1).
        impops_no_imp_value(0), // =0 after REQ-3174
      // 'Aberfoyle' city, Text
      ORM::ChannelImpInventory::Diffs().
        imps(0).
        clicks(0).
        actions(0).
        revenue(0). 
        impops_user_count(1).
        imps_user_count(0).
        imps_value(0).
        imps_other(0).
        imps_other_user_count(0).
        imps_other_value(0).
        impops_no_imp(2).
        impops_no_imp_user_count(1).
        impops_no_imp_value(0) // =0 after REQ-3174
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(pq_conn_, diffs, imp_inventory)).check(),
      "ChannelImpInventory check");
  }

  {
    // ChannelPerformance
    typedef ORM::ChannelPerformance::Diffs Diffs;

    const Diffs diffs[] =
    {
      // 'London, City of' state
      Diffs().
        imps(1).
        clicks(1).
        actions(1).
        revenue(
          ORM::stats_diff_type(
            display_cpa, 0.001)),
      // 'London' city
      Diffs().
        imps(3).
        clicks(3).
        actions(0).
        revenue(
          ORM::stats_diff_type(
            display_cpm / 1000 +
            text_cpc1 + text_cpc2, 0.001)),
      // 'Aberfoyle' city
      Diffs(0)        
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(pq_conn_, diffs, performance)).check(),
      "ChannelPerformance check");
  }

  {
    // ChannelUsage
    typedef ORM::ChannelUsageStats::Diffs Diffs;
    
    const Diffs diffs[] =
      {
        // 'London, City of' state
        Diffs().
          imps(1).
          clicks(1).
          actions(1).
          revenue(
            ORM::stats_diff_type(
              display_cpa, 0.001)),
        // 'London' city
        Diffs().
          imps(3).
          clicks(3).
          actions(0).
          revenue(
            ORM::stats_diff_type(
              display_cpm / 1000 +
              text_cpc1 + text_cpc2, 0.001)),
        // 'Aberfoyle' city
        Diffs(0)
      };

    FAIL_CONTEXT(AutoTest::wait_checker(
      AutoTest::stats_diff_checker(pq_conn_, diffs, channel_usage)).check(),
      "ChannelUsage check");
  }

  // ChannelInventoryByCpm
  FAIL_CONTEXT(AutoTest::wait_checker(
      AutoTest::stats_each_diff_checker(pq_conn_, 0, inventory_by_cpm)).check(),
    "ChannelInventoryByCPM check");

  {
    // ExpressionPerformance

    const ORM::ExpressionPerformanceStats::Diffs diffs[3] =
    {
      // CPM CC, City 'London' & display channel expression.
      ORM::ExpressionPerformanceStats::Diffs().
        imps(1).
        clicks(1).
        actions(0),
      // CPA CC, State 'London, City Of' & display channel expression.
      ORM::ExpressionPerformanceStats::Diffs().
        imps(1).
        clicks(1).
        actions(1),
      // CPC CC, Only display channel expression.
      ORM::ExpressionPerformanceStats::Diffs().
        imps(1).
        clicks(1).
        actions(0)
    };

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(pq_conn_, diffs, expr_perf)).check(),
      "ExpressionPerformance check");
  }

  {
    // SiteChannelStats
    typedef ORM::SiteChannelStats::Diffs Diffs;
    const Diffs diffs[] =
    {
      // 'London' city, display tag
      Diffs().
        imps(1).
        adv_revenue(
          ORM::stats_diff_type(
            display_cpm / 1000, 0.001)).
        pub_revenue(0),
      // 'London' city, text tag
      Diffs().
        imps(2).
        adv_revenue(
          ORM::stats_diff_type(
            text_cpc1 + text_cpc2, 0.001)).
        pub_revenue(0),
      // 'London, City of' state, display tag
      Diffs().
        imps(1).
        adv_revenue(
          ORM::stats_diff_type(
            display_cpa, 0.001)).
        pub_revenue(0)
    };
    
    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_, diffs, sitechannel)).check(),
      "SiteChannelStats check");
  }

  FAIL_CONTEXT(
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      0,
      inventory_estim).check(),
    "ChannelInventoryEstimStats check");

  return true;
}

void
GEOChannelsStats::make_requests()
{
  // Display
  {
    add_descr_phrase("Requests. Display part.");
    AdClient client(AdClient::create_user(this));

    // Match display channels
    NSLookupRequest request;
    request.referer_kw = fetch_string("DISPLAYKWD");
    request.loc_name.clear();
    request.debug_time = testtime;

    client.process_request(request);
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "DISPLAYCH",
        client.debug_info.trigger_channels).check(),
      "Display part. Expected trigger_channels");

    // City GEO channel
    make_location_request(
      client, "DISPLAYTID-CPM",
      "gb/" + fetch_string("STATE1") +
      "/" + fetch_string("CITY1"),
      "DISPLAYCC-CPM", AE_CLICK);

    // State GEO channel
    make_location_request(
      client, "DISPLAYTID-CPA",
      "gb/" + fetch_string("STATE1"),
      "DISPLAYCC-CPA", AE_CLICK | AE_ACTION);

    // Absent GEO channel
    make_location_request(
      client, "DISPLAYTID-CPC",
      "gb/" + fetch_string("STATE2") +
      "/" + fetch_string("CITY2"),
      "DISPLAYCC-CPC", AE_CLICK);

    // Absent GEO channel
    make_location_request(
      client, "DISPLAYTID-CPM",
      "gb/" + fetch_string("STATE3") +
      "/" + fetch_string("CITY3"), 0);

  }

  // Text
  {
    add_descr_phrase("Requests. Text part.");
    AdClient client(AdClient::create_user(this));

    // Match text channels
    NSLookupRequest request;
    request.loc_name.clear();
    request.referer_kw =
      fetch_string("TEXTKWD") + "," +
      fetch_string("CHANNELKWD");
    request.debug_time = testtime;

    client.process_request(request);
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "TEXTCH,CHANNELCH",
        client.debug_info.trigger_channels).check(),
      "Text part. Expected trigger_channels");

    // GEO text creative selection
    make_location_request(
      client, "TEXTTID",
      "gb/" + fetch_string("STATE1") +
      "/" + fetch_string("CITY1"),
      "CHANNELCC,TEXTCC", AE_CLICK);
  }
}

void GEOChannelsStats::make_location_request(
  AdClient& client,
  const char* tid,
  const std::string& location,
  const char* expected_ccids,
  unsigned short flags)
{
  NSLookupRequest request;
  request.loc_name = location;
  request.tid = fetch_string(tid);
  request.debug_time = testtime;


  if (expected_ccids)
  {
    AutoTest::CreativeList exp_ccids;
    AutoTest::ConsequenceActionList actions;

    if ( flags & AE_CLICK )
    {
      actions.push_back(AutoTest::CLICK);
    }

    if ( flags & AE_ACTION )
    {
      actions.push_back(AutoTest::ACTION);
    }

    String::SubString s(expected_ccids);
    String::StringManip::SplitComma tokenizer(s);
    String::SubString token;

    while (tokenizer.get_token(token))
    {
      exp_ccids.push_back(fetch_string(token.str()));
    }

    FAIL_CONTEXT(
      client.do_ad_requests(
        request,
        exp_ccids,
        actions));
  }
  else
  {
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "No creative expected");
  }

}
