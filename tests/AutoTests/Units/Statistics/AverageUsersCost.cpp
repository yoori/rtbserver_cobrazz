#include "AverageUsersCost.hpp"

REFLECT_UNIT(AverageUsersCost) (
  "Statistics",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;

  namespace ORM = AutoTest::ORM;

  enum CaseOption
  {
    CO_CHECK_IMP_COUNT = 0x1
  };

  enum UserActions
  {
    UA_TRACK = 0x1,
    UA_CLICK = 0x2,
    UA_ACTION = 0x4,
  };

  const AverageUsersCost::UserRequest CPM_CPC[] = {
    {"DEFCUR/CPM/TAG", "DEFCUR/CPM/CCID", "DEFCUR/CPM/REVENUE", true, UA_TRACK | UA_CLICK},
    {"DEFCUR/CPC/TAG", "DEFCUR/CPC/CCID", "DEFCUR/CPC/REVENUE", true, UA_TRACK | UA_CLICK}
  };

  const AverageUsersCost::UserRequest CPM_CPA[] = {
    {"DEFCUR/CPM/TAG", "DEFCUR/CPM/CCID", "DEFCUR/CPM/REVENUE", true, UA_TRACK | UA_CLICK},
    {"DEFCUR/CPA/TAG", "DEFCUR/CPA/CCID", "DEFCUR/CPA/REVENUE", true, UA_TRACK | UA_CLICK | UA_ACTION}
  };

  const AverageUsersCost::UserRequest CPM_ZERO[] = {
    {"DEFCUR/CPM/TAG", "DEFCUR/CPM/CCID", "DEFCUR/CPM/REVENUE", false, UA_CLICK},
    {"DEFCUR/CPM/TAG", "DEFCUR/CPM/CCID", NULL, true, 0}
  };

  const AverageUsersCost::UserRequest NON_DEFAULT_CURRENCY[] = {
    {"NONDEFCUR/CPM/TAG", "NONDEFCUR/CPM/CCID", "NONDEFCUR/CPM/REVENUE", true, UA_TRACK | UA_CLICK},
    {"NONDEFCUR/CPC/TAG", "NONDEFCUR/CPC/CCID", "NONDEFCUR/CPC/REVENUE", true, UA_TRACK | UA_CLICK},
    {"NONDEFCUR/CPA/TAG", "NONDEFCUR/CPA/CCID", "NONDEFCUR/CPA/REVENUE", true, UA_TRACK | UA_CLICK | UA_ACTION}
  };

  const AverageUsersCost::UserRequest TEXT_CC[] = {
    {"TEXT/TAG", "TEXT/CCID#1,TEXT/CCID#4,TEXT/CCID#5", "TEXT/REVENUE", false, UA_CLICK}
  };

}

void
AverageUsersCost::log_profile(
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

template<size_t RequestsCount>
double
AverageUsersCost::test_case(
  const std::string& description,
  unsigned long test_channel,
  const char* test_keyword,
  const UserRequest(&requests) [RequestsCount],
  unsigned long flags)
{
  add_descr_phrase(description);

  std::string uid;

  ORM::StatsArray<ORM::ChannelPerformance, 2> stats;

  stats[0].key().
    channel_id(request_channel_).
    last_use(today_);
  stats[1].key().
    channel_id(request_k_channel_).
    last_use(today_);

  stats.select(pq_conn_);
  
  size_t d_imps = 0,
         t_imps = 0,
         d_clicks = 0,
         t_clicks = 0,
         d_actions = 0,
         additional_reqs = 0; // in case of text ad
  double d_revenue = 0,
         t_revenue = 0;
  AdClient client(AdClient::create_user(this));
  for (size_t i = 0; i < RequestsCount; ++i)
  {
    const char* format = requests[i].track_imp ? "unit-test-imp" : "unit-test";

    AutoTest::CreativeList exp_ccid;
    String::SubString s(requests[i].ccid);
    String::StringManip::SplitComma tokenizer(s);
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      exp_ccid.push_back(fetch_string(token.str()));
    }

    if (!requests[i].track_imp)
    {
      if (exp_ccid.size() < 2) {
        d_imps++; }
      else {
        t_imps += exp_ccid.size(); }
    }

    AutoTest::ConsequenceActionList consequence_actions;
    if (requests[i].actions & UA_TRACK)
    {
      consequence_actions.push_back(
        AutoTest::ConsequenceAction(
          AutoTest::TRACK, today_));
      if (exp_ccid.size() < 2) {
        d_imps++; }
      else {
        t_imps += exp_ccid.size(); };
    }
    if (requests[i].actions & UA_CLICK)
    {
      consequence_actions.push_back(
        AutoTest::ConsequenceAction(
          AutoTest::CLICK, today_));
      if (exp_ccid.size() < 2) {
        d_clicks++; }
      else {
        t_clicks += exp_ccid.size(); };
    }
    if (requests[i].actions & UA_ACTION)
    {
      d_actions += exp_ccid.size();
      consequence_actions.push_back(
        AutoTest::ConsequenceAction(
          AutoTest::ACTION, today_));
    }

    // if expected more than one creative then this creatives are text and
    // it is required to send 'making keyword context request'
    if (exp_ccid.size() > 1)
    {
      // Bag illustration (Yuri Kuznecov)
      // sum_ecpm must depend on requsts count, not imps.
      // But in case of text ad it depends on imps count
      additional_reqs += exp_ccid.size() - 1;
      client.process_request(
        NSLookupRequest().
          referer_kw(request_keyword_).
           debug_time(today_));
    }

    client.process_request(
        NSLookupRequest().
          referer_kw(request_keyword_).
           tid(fetch_string(requests[i].tag)).
             format(format).
               debug_time(today_));

    if (uid.empty() && !client.debug_info.uid.value().empty())
    {
      uid = "\\" + client.debug_info.uid.value();
    }

    FAIL_CONTEXT(
      client.do_ad_requests(
        exp_ccid,
        consequence_actions));

    if (requests[i].revenue && strlen(requests[i].revenue))
    {
      if (exp_ccid.size() < 2) {
        d_revenue += fetch_float(requests[i].revenue); }
      else {
        t_revenue += fetch_float(requests[i].revenue); };
    }
  }

  if (flags & CO_CHECK_IMP_COUNT)
  {
    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::InventoryProfileChecker(
          this,
          uid,
          AutoTest::InventoryProfileChecker::Expected().
            imp_count(RequestsCount))).check(),
      description +
        "Check user inventory profile");
  }

  const ORM::ChannelPerformance::Diffs diffs[] =
  {
    ORM::ChannelPerformance::Diffs().
      imps(d_imps).
      clicks(d_clicks).
      actions(d_actions).
      revenue(d_revenue),
    ORM::ChannelPerformance::Diffs().
      imps(t_imps).
      clicks(t_clicks).
      actions(0).
      revenue(t_revenue)
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats)).check(),
    description +
      "ChannelPerformance check");

  log_profile(uid);

  uids_.push_back(uid);

  client.process_request(
    NSLookupRequest().referer_kw(test_keyword).debug_time(today_));

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(test_channel) + "P",
      client.debug_info.trigger_channels).check(),
    "must get expected channel in trigger_channels debug_info header");

  return (d_revenue + t_revenue) * 1000 / (RequestsCount + additional_reqs);
}

bool
AverageUsersCost::run_test()
{
  request_keyword_ = fetch_string("REQUEST/KEYWORD");
  request_channel_ = fetch_int("REQUEST/CHANNEL");
  request_k_channel_ = fetch_int("REQUEST/K-CHANNEL");
  default_colo_ = fetch_int("DEFAULT/COLO");

  unsigned long test_channel1 = fetch_int("TEST1/CHANNEL");
  unsigned long test_channel2 = fetch_int("TEST2/CHANNEL");
  unsigned long test_channel3 = fetch_int("TEST3/CHANNEL");
  unsigned long test_channel4 = fetch_int("TEST4/CHANNEL");
  unsigned long test_channel5 = fetch_int("TEST5/CHANNEL");

  ORM::StatsArray<ORM::ChannelInventoryStats, 5> stats;
  stats[0].key(ORM::ChannelInventoryStats::Key().
    channel_id(test_channel1).sdate(today_));
  stats[1].key(ORM::ChannelInventoryStats::Key().
    channel_id(test_channel2).sdate(today_));
  stats[2].key(ORM::ChannelInventoryStats::Key().
    channel_id(test_channel3).sdate(today_));
  stats[3].key(ORM::ChannelInventoryStats::Key().
    channel_id(test_channel4).sdate(today_));
  stats[4].key(ORM::ChannelInventoryStats::Key().
    channel_id(test_channel5).sdate(today_));
  stats.select(pq_conn_);

  typedef ORM::ChannelInventoryStats::Diffs Diffs;
  
  const Diffs diffs[5] =
  {
    Diffs().
      active_users(1).
      total_users(1).
      sum_ecpm(
        test_case(
          "Combined channel inventory. Part 1 - CPM + CPC.",
          test_channel1,
          fetch_string("TEST1/KEYWORD").c_str(),
          CPM_CPC)),
    Diffs().
      active_users(1).
      total_users(1).
      sum_ecpm(
        test_case(
          "Combined channel inventory. Part 2 - CPM + CPA.",
          test_channel2,
          fetch_string("TEST2/KEYWORD").c_str(),
          CPM_CPA)),
    Diffs().
      active_users(1).
      total_users(1).
      sum_ecpm(
        test_case(
          "Unverified impressions.",
          test_channel3,
          fetch_string("TEST3/KEYWORD").c_str(),
          CPM_ZERO,
          CO_CHECK_IMP_COUNT)),
    Diffs().
      active_users(1).
      total_users(1).
      sum_ecpm(
        test_case(
          "Currency converting.",
          test_channel4,
          fetch_string("TEST4/KEYWORD").c_str(),
          NON_DEFAULT_CURRENCY)),
    Diffs().
      active_users(1).
      total_users(1).
      sum_ecpm(
        ORM::stats_diff_type(
          test_case(
            "Text creatives in one request.",
            test_channel5,
            fetch_string("TEST5/KEYWORD").c_str(),
            TEXT_CC),
          0.01))
  };

  try
  {
    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(pq_conn_, diffs, stats)).check(),
      "ChannelInventory users stats check");
  }
  catch (const eh::Exception&)
  {
    std::for_each(
      uids_.begin(),
      uids_.end(),
      std::bind1st(
        std::mem_fun(
          &AverageUsersCost::log_profile), this));
    throw;
  }

  return true;
}

