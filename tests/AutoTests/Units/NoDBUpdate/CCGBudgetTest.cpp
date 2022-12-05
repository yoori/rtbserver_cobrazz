
#include "CCGBudgetTest.hpp"
#include "SpentBudgetChecker.hpp"
 
REFLECT_UNIT(CCGBudgetTest) (
  "NoDBUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  const char ASPECT[] = "CCGBudgetTest";
  const unsigned long INFO = Logging::Logger::INFO;
  const unsigned long WARNING = Logging::Logger::WARNING;
  // Equal to tz offset for TZAdvertiser account ( for America/Sao_Paulo)
  const unsigned long TZ_SHIFT = 3;

  typedef AutoTest::SpentBudgetChecker SpentBudgetChecker;

  const int MINUTE = 60;
  const int HOUR = 60 * MINUTE;
  const int DAY = 24 * HOUR;

  namespace ORM = ::AutoTest::ORM;
};

const CCGBudgetTest::TestCase CCGBudgetTest::GMT_CASES[] =
{
  { "Net commission.",
    "NetAdvertiser",
    CCGBudgetTest::CF_BUDGET_NOT_SPENT | CCGBudgetTest::CF_CPC, 0, 1, 0, 0 },

  { "Fixed daily budget.",
    "FixedDailyBudget", 0, 0, 0, 0,
    &CCGBudgetTest::increase_budget<&CCG::daily_budget, 1> },

  // have sense only for dynamic case
  { "Dynamic daily budget (not spent budget)",
    "DynamicDailyBudget2",
    CCGBudgetTest::CF_BUDGET_DYNAMIC | CCGBudgetTest::CF_BUDGET_NOT_SPENT, 0, 1, 0,
    &CCGBudgetTest::prolong_lifetime },

  { "Dynamic daily budget (yesterday spent budget)",
    "DynamicDailyBudget3",
    CCGBudgetTest::CF_BUDGET_DYNAMIC | CCGBudgetTest::CF_BUDGET_NOT_SPENT | CCGBudgetTest::CF_SPEND_REST_BUDGET,
    // Two requests at yesterday date + one today
    90 * MINUTE, 3, -13 * HOUR, 0 },

  { "Dynamic daily budget.",
    "DynamicDailyBudget1",
    CCGBudgetTest::CF_BUDGET_DYNAMIC, 0, 0, 0,
    &CCGBudgetTest::increase_budget<&CCG::budget, 10> },
  
  { "CCG channel CPM != 0.",
    "ChannelRate", 0, 0, 0, 0, 0 },

  { "Gross commission.",
    "GrossAdvertiser", CCGBudgetTest::CF_CPC, 0, 1, 0, 0 },
};

const CCGBudgetTest::TestCase CCGBudgetTest::GMT_PLUS_3_CASES[] =
{
  { "Advertiser in positive TZ (spent budget yesterday in GMT, but today in TZ)",
    "DDBudgetPosTZ1",
    CCGBudgetTest::CF_BUDGET_DYNAMIC | CCGBudgetTest::CF_BUDGET_NOT_SPENT | CCGBudgetTest::CF_SPEND_REST_BUDGET,
    0, 0, -13 * HOUR, 0 },

  { "Advertiser in positive TZ (spent all budget)",
    "DDBudgetPosTZ2", CCGBudgetTest::CF_BUDGET_DYNAMIC, 0, 0, 0, 0 }
};

const CCGBudgetTest::TestCase CCGBudgetTest::GMT_MINUS_3_CASES[] =
{
  { "America/Sao_Paulo account.",
    "TZAdvertiser",
    CCGBudgetTest::CF_BUDGET_NOT_SPENT | CCGBudgetTest::CF_SPEND_REST_BUDGET,
    TZ_SHIFT * HOUR, 0, -11 * HOUR, 0 },

  { "Advertiser in negative TZ (not spent budget)",
    "DDBudgetNegTZ",
    CCGBudgetTest::CF_BUDGET_DYNAMIC | CCGBudgetTest::CF_BUDGET_NOT_SPENT | CCGBudgetTest::CF_SPEND_REST_BUDGET,
    0, 0, 0, 0 },

  { "Advertiser in negative TZ (spent all budget)",
    "DDBudgetNegTZMarker", CCGBudgetTest::CF_BUDGET_DYNAMIC, 0, 0, 0, 0 }
};

Generics::Time CCGBudgetTest::get_switch_date_time(
  const Generics::Time& time,
  const char* tzname)
{
  Generics::Time time_zone_offset =
    AutoTest::ORM::get_tz_ofset(this, tzname);

  return (time + time_zone_offset).get_gm_time().get_date() +
    Generics::Time::ONE_DAY - time_zone_offset;
}

Generics::Time CCGBudgetTest::get_time_in_tz(const char* tzname)
{
  Generics::Time time_zone_offset =
    AutoTest::ORM::get_tz_ofset(this, tzname);

  return Generics::Time::get_time_of_day() + time_zone_offset;
}

bool
CCGBudgetTest::checker_call(
  const std::string& description,
  AutoTest::Checker* checker) /*throw(eh::Exception)*/
{
  try
  {
    return checker->check();
  }
  catch (
    const AutoTest::TimeLessChecker::TimeLessCheckFailed&)
  {
    throw;
  }
  catch(const eh::Exception& e)
  {
    FAIL_CONTEXT({throw;}, description);
    return false;
  }                                                                 
}

void CCGBudgetTest::process_case(
  const TestCase& test,
  double& realized_budget,
  bool initial)
{
  add_descr_phrase(test.description);
  unsigned long ccgid = fetch_int(test.prefix + "/CCG");
  std::string ccid = fetch_string(test.prefix + "/CC");
  double req_revenue = fetch_float(test.prefix + "/Revenue");
  double budget = fetch_float(test.prefix + "/Budget");
  std::string time_zone = fetch_string(test.prefix + "/TIMEZONE");

  CampaignChecker ccg_checker(
    this,
    ccgid,
    CampaignChecker::Expected().
    status("A").
    eval_status("A"));

  Generics::Time start_time =
    Generics::Time(fetch_string(test.prefix + "/TZDATE") + ":12-00-00",
                   "%d-%m-%Y:%H-%M-%S");

  if (initial)
  {
    FAIL_CONTEXT(
      ccg_checker.check(),
      test.description + 
        " Initial check");    
  }
  else
  {
    FAIL_CONTEXT(
      AutoTest::wait_checker(ccg_checker).check(),
      test.description + 
        " Initial check");

    // Check that date switched
    if (start_time != Generics::Time(
          get_time_in_tz(time_zone.c_str()).get_gm_time().format("%d-%m-%Y")
            + ":12-00-00",
          "%d-%m-%Y:%H-%M-%S"))
    { start_time += Generics::Time::ONE_DAY; };
  }

  // Calculate number of requests, need for test
  unsigned long request_count = 0;
  int days_to_end = -1;
  if (test.flags & CF_BUDGET_DYNAMIC)
  {
    const Generics::Time date_end(
      fetch_string(test.prefix + "/DateEnd"),
      "%Y-%m-%d %H:%M:%S");

    days_to_end =
      ( date_end.get_gm_time().get_date() -
        start_time.get_gm_time().get_date() ).tv_sec /
        Generics::Time::ONE_DAY.tv_sec + 1; // current day also consider

    request_count = days_to_end > 0 ?
      static_cast<unsigned long>(
        ceil((budget - realized_budget) / (days_to_end * req_revenue))) : 0;
    if (test.flags & CF_BUDGET_NOT_SPENT)
    { --request_count; }
  }
  else
  {
    request_count =
      static_cast<unsigned long> (budget / req_revenue);
  }

  request_count = test.requests? test.requests: request_count;

  AutoTest::Logger::thlog().stream(INFO, ASPECT) <<
    "Case '" << test.description << "' environment: " <<
    " ccg_id='" << ccgid << "', " <<
    " budget='" << budget << "', " <<
    " realized_budget='" << realized_budget << "', " <<
    " request revenue='" << req_revenue << "', " <<
    " request_count='" << request_count << "', " <<
    " days to end='" << (days_to_end > 0? strof(days_to_end): "-") << "'.";

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      request_count > 0),
    test.description +
    " Invalid calculated request count."
    " Please, check test environment!");

  // Make budget reached
  NSLookupRequest request;
  request.referer_kw = fetch_string(test.prefix + "/KWD");
  request.tid = fetch_string("Tid1");

  Generics::Time debug_time;

  for (unsigned long i = 0; i < request_count; ++i)
  {
    AdClient client = AdClient::create_user(this);

    debug_time = start_time + test.debug_time_shift +
      (request_count > 1 ? i * test.interval / (request_count - 1) : 0);
    
    request.debug_time = debug_time;
    
    client.process_request(request);
    
    AutoTest::ConsequenceActionList actions;

    std::list<std::string> expected_ccs;

    expected_ccs.push_back(ccid);

    if (test.flags & CF_CPC)
    {
      actions.push_back(
        AutoTest::ConsequenceAction(
          AutoTest::CLICK, debug_time));
    }

    FAIL_CONTEXT(
      client.do_ad_requests(
        expected_ccs, actions),
      test.description +
        " Check request#" + strof(i));

    realized_budget += req_revenue;
  }

  // We can check eval_status only for current day (in TZ)
  if (test.debug_time_shift != 0)
  { request.debug_time = start_time; }

  add_checker(test.description,
    SpentBudgetChecker(
      this,
      request.tid(fetch_string("Tid2")),
      ccgid,
      test.flags & CF_BUDGET_NOT_SPENT? fetch_int(test.prefix + "/CC"): 0,
      get_switch_date_time(start_time, time_zone.c_str())));

  campaign_last_request[test.prefix] = debug_time;
}

template<size_t SIZE>
void
CCGBudgetTest::process_cases(const TestCase (&cases)[SIZE],
                             bool initial)
{
  for (unsigned long i = 0; i < SIZE; ++i)
  {
    if (initial)
    { budget_reached[cases[i].prefix] = 0; }
    AUTOTEST_CASE(
      process_case(cases[i], budget_reached[cases[i].prefix], initial),
      cases[i].description);
  }

  check();

  for (unsigned long i = 0; i < SIZE; ++i)
  {
    if (cases[i].flags & CF_SPEND_REST_BUDGET)
    {
      AUTOTEST_CASE(
        spend_rest_budget(cases[i].prefix),
        cases[i].description);
    }
  }

  check();
}

bool 
CCGBudgetTest::run()
{
  // GMT cases
  try
  {
    process_cases(GMT_CASES, true);
  }
  catch (const AutoTest::TimeLessChecker::TimeLessCheckFailed&)
  {
    add_descr_phrase("=== Day switched for GMT TZ: "
                     "run tests again for next day ===");
    process_cases(GMT_CASES, false);
  }

  // GMT +3:00 cases
  try
  {
    process_cases(GMT_PLUS_3_CASES, true);
  }
  catch (const AutoTest::TimeLessChecker::TimeLessCheckFailed&)
  {
    add_descr_phrase("=== Day switched for GMT+3:00 timezone: "
                     "run tests again for next day ===");
    process_cases(GMT_PLUS_3_CASES, false);
  }

  // GMT -3:00 cases
  try
  {
    process_cases(GMT_MINUS_3_CASES, true);
  }
  catch (const AutoTest::TimeLessChecker::TimeLessCheckFailed&)
  {
    add_descr_phrase("=== Day switched for GMT-3 timezone: "
                     "run tests again for next day ===");
    process_cases(GMT_MINUS_3_CASES, false);
  }

  return true;
}

void CCGBudgetTest::spend_rest_budget(const std::string& prefix)
{
  std::string description("Spent rest budget");
  add_descr_phrase(description);

  std::map<std::string, Generics::Time>::const_iterator pos =
    campaign_last_request.find(prefix);

  Generics::Time debug_time = pos != campaign_last_request.end()
    ? pos->second
    : Generics::Time(fetch_string(prefix + "/TZDATE") + ":12-00-00",
        "%d-%m-%Y:%H-%M-%S");

  Generics::Time switch_date_time = get_switch_date_time(debug_time,
        fetch_string(prefix + "/TIMEZONE").c_str());

  add_checker(description +
    " - campaign deactivation",
    SpentBudgetChecker(this,
      NSLookupRequest().referer_kw(fetch_string(prefix + "/KWD")).
                        tid(fetch_string("Tid3")).
                        debug_time(debug_time),
      fetch_int(prefix + "/CCG"), 0,
      switch_date_time));
}

template<CCGMember member, int coef>
void CCGBudgetTest::increase_budget(const std::string& prefix)
{
  std::string description("Increase budget of creative group");
  add_descr_phrase(description);

  std::map<std::string, Generics::Time>::const_iterator pos =
    campaign_last_request.find(prefix);

  Generics::Time debug_time = pos != campaign_last_request.end()
    ? pos->second
    : Generics::Time(fetch_string(prefix + "/TZDATE") + ":12-00-00",
        "%d-%m-%Y:%H-%M-%S");

  Generics::Time switch_date_time = get_switch_date_time(
    debug_time,
    fetch_string(prefix + "/TIMEZONE").c_str());

  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(
      fetch_int(prefix + "/CCG"));

  // Initial check
  try
  {
    CampaignChecker(this, ccg->id(),
      CampaignChecker::Expected().
        status("A").
        eval_status("I")).check();
  }
  catch (const AutoTest::CheckFailed& e)
  {
    if (AutoTest::TimeLessChecker(switch_date_time).check(false))
    {
      FAIL_CONTEXT({ throw; }, description + " - initial check");
    }
    AutoTest::Logger::thlog().stream(WARNING, ASPECT)
      << "Day switched at post_condition step: '"
      << description << "' case will be omitted";
    return;
  }

  ccg->*member =
    (ccg->*member).value() + coef * fetch_float(prefix + "/Revenue");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update()),
    "updating ccg.daily_budget");

  add_checker(description,
    SpentBudgetChecker(this,
      NSLookupRequest().referer_kw(fetch_string(prefix + "/KWD")).
                        tid(fetch_string("Tid3")).
                        debug_time(debug_time),
      ccg->id(),
      fetch_int(prefix + "/CC"),
      switch_date_time));
}

void CCGBudgetTest::prolong_lifetime(const std::string& prefix)
{
  std::string description("Prolong lifetime of creative group");
  add_descr_phrase(description);

  const Generics::Time date_end(
    fetch_string(prefix + "/DateEnd"),"%Y-%m-%d %H:%M:%S");

  std::map<std::string, Generics::Time>::const_iterator pos =
    campaign_last_request.find(prefix);

  Generics::Time debug_time = pos != campaign_last_request.end()
    ? pos->second
    : Generics::Time(fetch_string(prefix + "/TZDATE") + ":12-00-00",
        "%d-%m-%Y:%H-%M-%S");

  Generics::Time switch_date_time = get_switch_date_time(
    debug_time,
    fetch_string(prefix + "/TIMEZONE").c_str());

  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(
      fetch_int(prefix + "/CCG"));

  // Initial checks
  try
  {
    // Check that ccg has active status (regardless of some spent budget)
    CampaignChecker(this, ccg->id(),
      CampaignChecker::Expected().
        status("A").
        eval_status("A")).check();
  }
  catch(const AutoTest::CheckFailed& e)
  {
    if (AutoTest::TimeLessChecker(switch_date_time).check(false))
    {
      FAIL_CONTEXT({ throw; }, description + " - initial check");
    }
    AutoTest::Logger::thlog().stream(WARNING, ASPECT)
      << "Day switched at post_condition step: '"
      << description << "' case will be omitted";
    return;
  }

  // Change end date of ccg to make it inactive
  ccg->date_end = date_end + 7 * Generics::Time::ONE_DAY.tv_sec;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update()),
    "updating ccg.date_end");

  add_checker(description,
    SpentBudgetChecker(this,
      NSLookupRequest().referer_kw(fetch_string(prefix + "/KWD")).
                        tid(fetch_string("Tid3")).
                        debug_time(debug_time),
      ccg->id(), 0, switch_date_time));
}

template<size_t SIZE>
void
CCGBudgetTest::process_dynamic_cases(const TestCase (&cases)[SIZE])
{
  for (size_t i = 0; i < SIZE; ++i)
  {
    if (cases[i].dynamic_part)
    {
      AUTOTEST_CASE(
        (this->*(cases[i].dynamic_part))(cases[i].prefix),
        cases[i].description);
    }
  }
}

void CCGBudgetTest::post_condition()
{
  std::string description("Dynamic part");
  add_descr_phrase(description);

  process_dynamic_cases(GMT_CASES);
  process_dynamic_cases(GMT_PLUS_3_CASES);
  process_dynamic_cases(GMT_MINUS_3_CASES);

  try
  {
    check();
  }
  catch(const AutoTest::TimeLessChecker::TimeLessCheckFailed&)
  {
    AutoTest::Logger::thlog().stream(WARNING, ASPECT)
      << "Day switched at post_condition step: "
      << "dynamic cases checks will be omitted";
    return;
  }
}

void CCGBudgetTest::tear_down()
{}
