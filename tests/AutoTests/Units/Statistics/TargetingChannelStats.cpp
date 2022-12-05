
#include "TargetingChannelStats.hpp"

REFLECT_UNIT(TargetingChannelStats) (
  "Statistics",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  struct TestCase
  {
    const char* description;
    const char* prefix;
  };

  class UserLogger : public AutoTest::Checker
  {
  public:

    UserLogger(
      TargetingChannelStats* test,
      const std::list<std::string>& users) :
      test_(test),
      users_(users)
    { }

    virtual ~UserLogger() noexcept
    { }

    virtual
    bool
    check(bool throw_error = true)
      /*throw(AutoTest::CheckFailed, eh::Exception)*/
    {
      if (throw_error)
      {
        std::for_each(
          users_.begin(),
          users_.end(),
          std::bind1st(
            std::mem_fun(
              &TargetingChannelStats::log_profile), test_));
      }
      return true;
    }

  private:
    TargetingChannelStats* test_;
    std::list<std::string> users_;
  };

  unsigned int MIN_USERS_COUNT = 100;

}

void
TargetingChannelStats::log_profile(
  std::string uid)
{
  if (
    get_config().check_service(
      CTE_ALL, STE_EXPRESSION_MATCHER))
  {
    AutoTest::AdminsArray<AutoTest::InventoryProfileAdmin>
      admins;
    
    admins.initialize(this, CTE_ALL, STE_EXPRESSION_MATCHER, uid);
    
    admins.log(AutoTest::Logger::thlog());
  }
}

bool
TargetingChannelStats::check_channels_(
  const std::string& expected,
  AutoTest::DebugInfo::StringListValue& got)
{
  try
  {
    fetch_string(expected);
    return
      ChannelsCheck(
        this,
        expected.c_str(),
        got).check();
  }
  catch(const BaseUnit::InvalidArgument&)
  {
    return false;
  }
}

void
TargetingChannelStats::set_req_param_(
  NSLookupRequest& request,
  NSLookupRequest::Member member,
  const std::string& name)
{
  try
  {
    member(request, fetch_string(name));
  }
  catch(const BaseUnit::InvalidArgument&)
  {
    // pass
  }
}

template <typename Stat>
void
TargetingChannelStats::add_stats_(
  AutoTest::DBC::IConn& connection,
  const typename Stat::Key& key,
  ORM::StatsList<Stat>& stats)
{
  Stat stat(key);
  stat.select(connection);
  stats.push_back(stat);
}

void
TargetingChannelStats::initialize_stats_(
  StatCollection& stats,
  unsigned long channel_id,
  const std::string& expression,
  unsigned long cc_id,
  const ExpectedStats& expected,
  int text_size)
{
  add_stats_(
    pq_conn_,
    ORM::ChannelInventoryStats::Key().
      channel_id(channel_id).
      sdate(now_),
    stats.inv);

  ORM::ChannelImpInventory::Diffs imp_diff;
  imp_diff.
    imps(expected.imps).
    clicks(expected.clicks).
    actions(expected.actions).
    revenue(expected.revenue). 
    impops_user_count(1).
    imps_user_count(1).
    imps_other(expected.imp_imp_other).
    imps_other_user_count(1).
    imps_other_value(expected.imp_imp_other_value).
    impops_no_imp(expected.imp_no_imp).
    impops_no_imp_user_count(expected.imp_no_imp_users).
    impops_no_imp_value(0);

  ORM::ChannelImpInventory::Diffs no_imp_diff;
  no_imp_diff.
    imps(0).
    clicks(0).
    actions(0).
    revenue(0). 
    impops_user_count(1).
    imps_user_count(0).
    imps_value(0).
    imps_other(expected.no_imp_other).
    imps_other_user_count(1).
    impops_no_imp(expected.no_imp_no_imp).
    impops_no_imp_user_count(expected.no_imp_users).
    impops_no_imp_value(0);

  add_stats_(
    pq_conn_,
    ORM::ChannelImpInventory::Key().
      channel_id(channel_id).
      ccg_type("D").
      sdate(now_),
    stats.imp_inv_disp);

  stats.imp_inv_disp_diffs.push_back(
    text_size? no_imp_diff: imp_diff);

  add_stats_(
    pq_conn_,
    ORM::ChannelImpInventory::Key().
      channel_id(channel_id).
      ccg_type("T").
      sdate(now_),
    stats.imp_inv_text);

  stats.imp_inv_text_diffs.push_back(
    text_size? imp_diff: no_imp_diff);

  add_stats_(
    pq_conn_,
    ORM::ChannelInventoryByCPMStats::Key().
      channel_id(channel_id).
      sdate(now_),
    stats.inv_by_cpm);

  add_stats_(
    pq_conn_,
    ORM::ChannelPerformance::Key().
      channel_id(channel_id).
      last_use(now_),
    stats.perf);

  stats.perf_diffs.push_back(
    ORM::ChannelPerformance::Diffs().
      imps(expected.imps).
      clicks(expected.clicks).
      actions(expected.actions).
      revenue(expected.revenue));

  add_stats_(
    pq_conn_,
    ORM::ChannelUsageStats::Key().
      channel_id(channel_id).
      sdate(now_),
    stats.usage);

  stats.usage_diffs.push_back(
    ORM::ChannelUsageStats::Diffs().
      imps(expected.imps).
      clicks(expected.clicks).
      actions(expected.actions).
      revenue(expected.revenue));

  if (expression.empty())
  {
    add_stats_(
      pq_conn_,
      ORM::ExpressionPerformanceStats::Key().
        cc_id(cc_id),
      stats.expr_perf_zero);
  }
  else
  {
    add_stats_(
      pq_conn_,
      ORM::ExpressionPerformanceStats::Key().
        cc_id(cc_id).
        expression(expression).
        sdate(now_),
      stats.expr_perf);

    stats.expr_perf_diffs.push_back(
      ORM::ExpressionPerformanceStats::Diffs().
        imps(expected.imps).
        clicks(expected.clicks).
        actions(expected.actions));
  }

  add_stats_(
    pq_conn_,
    ORM::SiteChannelStats::Key().
      channel_id(channel_id).
      sdate(now_),
    stats.site_channel);

  stats.site_channel_diffs.push_back(
    ORM::SiteChannelStats::Diffs().
      imps(expected.imps).
      adv_revenue(expected.revenue).
      pub_revenue(0));

}

void
TargetingChannelStats::add_checkers_(
  StatCollection& stats)
{
  UserLogger log_checker(
    this, stats.users);
    
  ADD_WAIT_CHECKER(
    "ChannelInventory",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_each_diff_checker(
          pq_conn_,
          ORM::ChannelInventoryStats::Diffs().
            active_users(1).
            total_users(1),
          stats.inv)),
      log_checker));

  ADD_WAIT_CHECKER(
    "ChannelImpInventory (display)",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_,
          stats.imp_inv_disp_diffs,
          stats.imp_inv_disp)),
      log_checker));

  ADD_WAIT_CHECKER(
    "ChannelImpInventory (text)",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_,
          stats.imp_inv_text_diffs,
          stats.imp_inv_text)),
      log_checker));
    
  ADD_WAIT_CHECKER(
    "ChannelInventoryByCPM",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_each_diff_checker(
          pq_conn_,
          ORM::ChannelInventoryByCPMStats::Diffs().
            user_count(1),
            stats.inv_by_cpm)),
      log_checker));
    
  ADD_WAIT_CHECKER(
    "ChannelPerformance",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_,
          stats.perf_diffs,
          stats.perf)),
      log_checker));
    
    
  ADD_WAIT_CHECKER(
    "ChannelUsageStats",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_,
          stats.usage_diffs,
          stats.usage)),
      log_checker));
    
  ADD_WAIT_CHECKER(
    "ExpressionPerformance",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_,
          stats.expr_perf_diffs,
          stats.expr_perf)),
      log_checker));

  ADD_WAIT_CHECKER(
    "SiteChannelStats",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_,
          stats.site_channel_diffs,
          stats.site_channel)),
      log_checker));

  ADD_WAIT_CHECKER(
    "ExpressionPerformance (zero)",
    AutoTest::fail_checker(
      AutoTest::wait_checker(
        AutoTest::stats_each_diff_checker(
          pq_conn_,
          ORM::ExpressionPerformanceStats::Diffs(0),
          stats.expr_perf_zero)),
      log_checker));
}
  

void
TargetingChannelStats::process_case_(
  const std::string& prefix)
{
  AdClient client(AdClient::create_user(this));

  try
  {
    std::string channel_id = fetch_string(prefix + "/Audience");
    AutoTest::LocalAudienceChannel audience_channel(channel_id);
    audience_channel.fill_random(MIN_USERS_COUNT);
    audience_channel.add_user(client);
    audience_channel.commit();

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::BaseProfileChecker(
          this,
          AutoTest::prepare_uid(
            client.get_uid(),
            AutoTest::UUE_ADMIN_PARAMVALUE),
          false,
          AutoTest::UserInfoManagerController,
          AutoTest::BaseProfileChecker::Expected().
            audience_channels(
              "\\[ channel_id = " +
              channel_id +
              ", time = " +
              "\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2} \\]"))).check(),
      "Wait audience channel appearance in profile");
  }
  catch(const BaseUnit::InvalidArgument&)
  {/*Skip adding users to audience channel if it's not created for the case*/}

  // Initialize stats
  double cpm = fetch_float("Other/CPM") / 1000;
 
  ExpectedStats expected =
    {
      // Common
      1, 1, 1, static_cast<double>(fetch_float("CPA")),
      // No imp
      4, 2, 1,
      // Imp
      1, cpm, 1, 1 };

  StatCollection stats;
  
  initialize_stats_(
    stats,
    fetch_int(prefix + "/Targeting"),
    fetch_string(prefix + "/TargetingExpression"),
    fetch_int(prefix + "/CC"),
    expected);

  NSLookupRequest request;
  request.debug_time = now_;
  set_req_param_(request, &NSLookupRequest::referer_kw, prefix + "/KWD");
  set_req_param_(request, &NSLookupRequest::loc_name, prefix + "/Location");
  set_req_param_(request, &NSLookupRequest::user_agent, prefix + "/UserAgent");

  // no imps
  request.tid = fetch_int("NoImp/Tag");
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check CC (no imps)");

  // imps other
  request.tid = fetch_int("Other/Tag");
  if (!request.referer_kw.empty())
  {
    request.referer_kw = request.referer_kw.raw_str() + "," +
      fetch_string("Other/KWD");
  }
  else
  {
    request.referer_kw = fetch_string("Other/KWD");
  }
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Other/CC"),
      client.debug_info.ccid).check(),
    "Check CC (imps other)");
  
  // imps
  request.referer_kw.clear();
  set_req_param_(request, &NSLookupRequest::referer_kw, prefix + "/KWD");
  request.tid = fetch_int(prefix + "/Tag");

  client.process_request(request);

  FAIL_CONTEXT(
    check_channels_(
      prefix + "/Device",
      client.debug_info.device_channels),
    "Check device channels");

  FAIL_CONTEXT(
    check_channels_(
      prefix + "/GEO",
      client.debug_info.geo_channels),
    "Check GEO channels");

  FAIL_CONTEXT(
    check_channels_(
      prefix + "/Channel",
      client.debug_info.history_channels),
    "Check history");

  FAIL_CONTEXT(
    check_channels_(
      prefix + "/Audience",
      client.debug_info.history_channels),
    "Check history");

  stats.users.push_back("\\" + client.debug_info.uid.value());

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::CLICK);
  actions.push_back(AutoTest::ACTION);

  std::list<std::string> expected_ccs;
  expected_ccs.push_back(
    fetch_string(prefix + "/CC"));
  
  FAIL_CONTEXT(
    client.do_ad_requests(
      expected_ccs, actions));

  add_checkers_(stats);

}

void
TargetingChannelStats::set_up()
{ }

bool
TargetingChannelStats::run()
{
  const TestCase TEST_CASES[] =
  {
    // {"Channel, geo and device targeting.", "ALL"},
    {"Channel and geo targeting.", "CHANNEL-GEO"},
    {"Only geo targeting.", "GEO-ONLY"},
    {"Geo and device targeting.", "GEO-DEVICE"},
    {"Geo and audience targeting", "GEO-AUDIENCE"},
    {"Campaign Excluded channel targeting", "GEO-EXCLUDED"}
  };

  AUTOTEST_CASE(
    case_all_(),
    "Channel, geo and device targeting.");

  for (size_t i = 0; i < countof(TEST_CASES); ++i)
  {
    AUTOTEST_CASE(
      process_case_(
        TEST_CASES[i].prefix),
      TEST_CASES[i].description);
  }

  return true;
}

void
TargetingChannelStats::tear_down()
{ }

void
TargetingChannelStats::case_all_()
{
  double cpm_other = fetch_float("ALL/Other/CPM") / 1000;
  double cpa1 = fetch_float("ALL/CPA/1");
  double cpm2 = fetch_float("ALL/CPM/2") / 1000;
  double cpm3 = fetch_float("ALL/CPM/3") / 1000;

  StatCollection stats;

  const ExpectedStats EXPECTED[] =
  {
    {
      // Common
      1, 1, 1, cpa1,
      // No imp
      2, 1, 1,
      // Imp
      5, cpm_other + cpm2 + cpm3, 3, 1
    },
    {
      // Common
      1, 1, 0, cpm2,
      // No imp
      1, 0, 0,
      // Imp
      2, ORM::any_stats_diff, 0, 0
    },
    {
      // Common
      1, 1, 0, cpm3,
      // No imp
      1, 0, 0,
      // Imp
      2, ORM::any_stats_diff, 0, 0
    }
  };

  const std::string EXPRESSIONS[] =
  {
    fetch_string("ALL/TargetingExpression1"),
    fetch_string("ALL/TargetingExpression2"),
    "(" + fetch_string("ALL/ChannelB2") + " & " +
      fetch_string("ALL/ChannelB4") + ")"
  };

  size_t text_size = countof(EXPECTED);
  
  for (size_t i = 0; i < text_size; ++i)
  {
    std::string suffix  = strof(i+1);
    initialize_stats_(
      stats,
      fetch_int("ALL/Targeting" + suffix),
      EXPRESSIONS[i],
      fetch_int("ALL/CC/"  + suffix),
      EXPECTED[i],
      text_size);
  }
  
  AdClient client(AdClient::create_user(this));
  NSLookupRequest request;
  request.debug_time = now_;
  request.referer_kw = fetch_string("ALL/KWDB1");
  set_req_param_(request, &NSLookupRequest::loc_name, "ALL/Location");
  set_req_param_(request, &NSLookupRequest::user_agent, "ALL/UserAgent");

  // no imps
  request.tid = fetch_int("ALL/NoImp/Tag");
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check CC (no imps)");

  // imps other
  request.tid = fetch_int("ALL/Other/Tag");
  request.referer_kw = map_objects("ALL/Other/KWD,ALL/KWDB1");

  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("ALL/Other/CC"),
      client.debug_info.ccid).check(),
    "Check CC (imps other)");

  request.referer_kw =
    map_objects("ALL/KWDB1,ALL/KWDB2,ALL/KWDB3,ALL/KWDB4");
  request.tid = fetch_int("ALL/Tag");

  client.process_request(request);

  FAIL_CONTEXT(
    check_channels_(
      "ALL/Device",
      client.debug_info.device_channels),
    "Check device channels");

  FAIL_CONTEXT(
    check_channels_(
      "ALL/GEO",
      client.debug_info.geo_channels),
    "Check GEO channels");

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "ALL/ChannelB1,ALL/ChannelB2,ALL/ChannelB3,ALL/ChannelB4",
      client.debug_info.history_channels).check(),
    "Check history");

  stats.users.push_back("\\" + client.debug_info.uid.value());

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::CLICK);
  actions.push_back(AutoTest::NON_EMPTY_ACTION);

  std::list<std::string> expected_ccs;
  fetch_objects(
    std::inserter(expected_ccs, expected_ccs.begin()),
    "ALL/CC/1,ALL/CC/2,ALL/CC/3");
  
  FAIL_CONTEXT(
    client.do_ad_requests(
      expected_ccs, actions));

  add_checkers_(stats);
  
}

