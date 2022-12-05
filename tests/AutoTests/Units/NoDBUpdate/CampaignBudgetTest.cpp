
#include "CampaignBudgetTest.hpp"
#include "SpentBudgetChecker.hpp"
 
REFLECT_UNIT(CampaignBudgetTest) (
  "NoDBUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CampaignChecker CampaignChecker;
  typedef AutoTest::SpentBudgetChecker SpentBudgetChecker;

  const char ASPECT[] = "CampaignBudgetTest";
  const char FIXED_CASE[] = "Fixed daily budget";
  const char DYNAMIC_CASE[] = "Dynamic daily budget";
  const unsigned long WARNING = Logging::Logger::WARNING;

  namespace ORM = AutoTest::ORM;

  class TimeoutCampaignChecker: public CampaignChecker
  {
    typedef CampaignChecker Base;
  public:
    TimeoutCampaignChecker(
      BaseUnit* test,
      unsigned long ccg_id,
      const Expected& expected = Expected(),
      const Generics::Time& deadline = Generics::Time::ZERO):
      Base(test, ccg_id, expected),
      deadline_(deadline)
    {};

    ~TimeoutCampaignChecker() noexcept
    {};

    bool check(bool throw_error = true) /*throw(
        eh::Exception,
        AutoTest::CheckFailed,
        AutoTest::TimeLessChecker::TimeLessCheckFailed)*/
    {
      // throws exception in case of fail
      bool result = AutoTest::TimeLessChecker(deadline_).check();
      try
      {
        result |= Base::check(throw_error);
      }
      catch(const AutoTest::CheckFailed&)
      {
        AutoTest::TimeLessChecker(deadline_).check();
        throw;
      }
      if (!result)
      {
        result |= AutoTest::TimeLessChecker(deadline_).check();
      }
      return result;
    }

  private:
    Generics::Time deadline_;
  };
};

void
CampaignBudgetTest::fixed_daily_budget(bool initial)
{
  std::string description("Fixed daily budget of campaign");
  if (!initial)
  {description += " (rerun for next day because of day switching)";}
  add_descr_phrase(description);

  unsigned long ccg_cpm = fetch_int("FixedCPM/CCG");
  unsigned long ccg_cpc = fetch_int("FixedCPC/CCG");
  unsigned long never_matched_ccg = fetch_int("NeverMatched/CCG");

  unsigned long cc_cpm = fetch_int("FixedCPM/CC");
  unsigned long cc_cpc = fetch_int("FixedCPC/CC");

  std::string kwd_cpm = fetch_string("FixedCPM/Keyword");
  std::string kwd_cpc = fetch_string("FixedCPC/Keyword");

  Generics::Time next_date =
    (base_time_ + Generics::Time::ONE_DAY).get_gm_time().get_date();

  AutoTest::AndChecker initial_checker =
    AutoTest::and_checker(
      CampaignChecker(this, ccg_cpm,
        CampaignChecker::Expected().eval_status("A")),
      CampaignChecker(this, ccg_cpc,
        CampaignChecker::Expected().eval_status("A")),
      CampaignChecker(this, never_matched_ccg,
        CampaignChecker::Expected().eval_status("A")));

  // initial check
  if (initial)
  {
    FAIL_CONTEXT(initial_checker.check(), description + " - initial check");
  }
  else
  {
    FAIL_CONTEXT(AutoTest::wait_checker(initial_checker).check(),
      description + " - wait initial state again");
  };

  NSLookupRequest request;
  request.tid(fetch_string("Tag"));
  request.debug_time(base_time_);

  // FixedCPM creative group matching
  FAIL_CONTEXT(
    AutoTest::SelectedCreativeChecker(
      AdClient::create_user(this),
      request.referer_kw(kwd_cpm),
      cc_cpm).check(),
    "server must return expected ccid (FixedCPM CCG)");

  // FixedCPC creative group matching
  AutoTest::SelectedCreativeChecker checker(
      AdClient::create_user(this),
      request.referer_kw(kwd_cpc),
      cc_cpc);

  FAIL_CONTEXT(
      checker.check(),
    "server must return expected ccid (FixedCPC CCG)");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !checker.client().debug_info.click_url.empty()),
    "server must return not null click_url for returned creative");

  checker.client().process_request(checker.client().debug_info.click_url);

  add_checker("Checking spent daily budget (FixedCPM)",
    SpentBudgetChecker(
      this, request.referer_kw(kwd_cpm), ccg_cpm, 0, next_date));

  add_checker("Checking spent daily budget (FixedCPC)",
    SpentBudgetChecker(
      this, request.referer_kw(kwd_cpc), ccg_cpc, 0, next_date));

  add_wait_checker("Checking spent daily budget (never matched CCG)",
    TimeoutCampaignChecker(this,
      never_matched_ccg,
      CampaignChecker::Expected().eval_status("I"),
      next_date));
}

void
CampaignBudgetTest::dynamic_daily_budget(bool initial)
{
  std::string description("Dynamic daily budget of campaign");
  if (!initial)
  {description += " (rerun for next day because of day switching)";}
  add_descr_phrase(description);

  unsigned long ccg1 = fetch_int("Dynamic1/CCG");
  unsigned long ccg2 = fetch_int("Dynamic2/CCG");

  Generics::Time next_date =
    (base_time_ + Generics::Time::ONE_DAY).get_gm_time().get_date();

  AutoTest::AndChecker initial_checker = 
    AutoTest::and_checker(
      CampaignChecker(this, ccg1,
        CampaignChecker::Expected().eval_status("A")),
      CampaignChecker(this, ccg2,
        CampaignChecker::Expected().eval_status("A")));

  if (initial)
  {
    FAIL_CONTEXT(initial_checker.check(),
      description + " - initial check");
    realized_budget.clear();
  }
  else
  {
    FAIL_CONTEXT(AutoTest::wait_checker(initial_checker).check(),
      description + " - wait initial state again");
  };

  const std::string cases[] = { "Dynamic1", "Dynamic2" };

  NSLookupRequest request;
  request.tid(fetch_string("Tag"));
  request.debug_time(base_time_);

  for (size_t i = 0; i < countof(cases); ++i)
  {
    int days_to_end = (
      Generics::Time(fetch_string(cases[i] + "/DateEnd"), "%Y-%m-%d %H:%M:%S").
        get_gm_time().get_date() -
      base_time_.get_gm_time().get_date()).tv_sec /
      Generics::Time::ONE_DAY.tv_sec;
    days_to_end++; // current day also considered

    double imp_revenue = fetch_float(cases[i] + "/ImpRevenue");
    unsigned long max_repeat_count = days_to_end > 0
      ? static_cast<unsigned long>(
          (fetch_float(cases[i] + "/Budget") - realized_budget[cases[i]]) /
          (days_to_end * imp_revenue))
      : 0;

    if (cases[i] == "Dynamic2")
    {--max_repeat_count;}

    if (max_repeat_count <= 0)
    {
      AutoTest::Logger::thlog().stream(WARNING, ASPECT)
        << "Can't test dynamic daily budget, because CCG.date_end expired";
    }

    NSLookupRequest request;
    request.referer_kw(fetch_string(cases[i] + "/Keyword"));
    request.tid(fetch_string("Tag"));
    request.debug_time(base_time_);
    for (size_t j = 0; j < max_repeat_count; ++j)
    {
      FAIL_CONTEXT(
        AutoTest::SelectedCreativeChecker(
          AdClient::create_user(this),
          request.referer_kw(fetch_string(cases[i] + "/Keyword")),
          fetch_int(cases[i] + "/CC")).check(),
        "server must return expected ccid");

      realized_budget[cases[i]] += imp_revenue;
    }
  }

  add_checker("Checking spent dynamic daily budget",
    SpentBudgetChecker(
      this,
      request.referer_kw(fetch_string("Dynamic1/Keyword")),
      ccg1,
      0,
      next_date));

  add_wait_checker("Checking not spent dynamic daily budget",
    TimeoutCampaignChecker(
      this,
      ccg2,
      CampaignChecker::Expected().eval_status("A"),
      next_date));
}

bool 
CampaignBudgetTest::run()
{
  base_time_ = Generics::Time(fetch_string("TODAY"),
    "%Y-%m-%d %H:%M:%S");

  AUTOTEST_CASE(fixed_daily_budget(true), FIXED_CASE);
  AUTOTEST_CASE(dynamic_daily_budget(true), DYNAMIC_CASE);

  try
  {
    check();
  }
  catch (const AutoTest::TimeLessChecker::TimeLessCheckFailed&)
  {
    add_descr_phrase("Day switched: run test again for next day");
    base_time_ += Generics::Time::ONE_DAY;
    AUTOTEST_CASE(fixed_daily_budget(false), FIXED_CASE);
    AUTOTEST_CASE(dynamic_daily_budget(false), DYNAMIC_CASE);
  }

  return true;
}

void
CampaignBudgetTest::fixed_daily_budget_update()
{
  std::string description("Increase daily_budget of campaign");
  add_descr_phrase(description);

  Generics::Time next_date =
    (base_time_ + Generics::Time::ONE_DAY).get_gm_time().get_date();

  unsigned long ccg_cpm = fetch_int("FixedCPM/CCG");
  unsigned long ccg_cpc = fetch_int("FixedCPC/CCG");
  unsigned long ccg_never_matched = fetch_int("NeverMatched/CCG");

  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>(fetch_int("FixedCPM/Campaign"));

  // Initial checks
  try
  {
    AutoTest::and_checker(
      CampaignChecker(
        this,
        ccg_cpm,
        CampaignChecker::Expected().eval_status("I")),
      CampaignChecker(
        this,
        ccg_cpc,
        CampaignChecker::Expected().eval_status("I")),
      CampaignChecker(
        this,
        ccg_never_matched,
        CampaignChecker::Expected().eval_status("I"))).check();
  }
  catch(const AutoTest::CheckFailed& e)
  {
    if (AutoTest::TimeLessChecker(next_date).check(false))
    {
      FAIL_CONTEXT({ throw; }, description + " - initial check");
    }
    AutoTest::Logger::thlog().stream(WARNING, ASPECT)
      << "Day switched at post_condition step: '"
      << description << "' case will be omitted";
    return;
  }

  // Changes
  campaign->daily_budget = campaign->daily_budget.value() + 1;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->update()),
    "updating campaign.daily_budget");

  // Check new statuses
  NSLookupRequest request;
  request.debug_time(base_time_);
  request.tid(fetch_int("Tag"));

  add_checker("Check daily budget was increased (FixedCPM)",
    SpentBudgetChecker(
      this,
      request.referer_kw(fetch_string("FixedCPM/Keyword")),
      ccg_cpm,
      fetch_int("FixedCPM/CC"),
      next_date));

  add_checker("Check daily budget was increased (FixedCPC)",
    SpentBudgetChecker(
      this,
      request.referer_kw(fetch_string("FixedCPC/Keyword")),
      ccg_cpc,
      fetch_int("FixedCPC/CC"),
      next_date));

  add_wait_checker("Check daily budget was increased (NeverMatchedCCG)",
    TimeoutCampaignChecker(
      this,
      ccg_never_matched,
      CampaignChecker::Expected().eval_status("A"),
      next_date));
}

void
CampaignBudgetTest::dynamic_daily_budget_update()
{
  std::string description("Increase budget of campaign");
  add_descr_phrase(description);

  Generics::Time next_date =
    (base_time_ + Generics::Time::ONE_DAY).get_gm_time().get_date();

  unsigned long ccg1 = fetch_int("Dynamic1/CCG");
  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign1 =
    create<ORM::PQ::Campaign>(fetch_int("Dynamic1/Campaign"));

  unsigned long ccg2 = fetch_int("Dynamic2/CCG");
  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign2 =
    create<ORM::PQ::Campaign>(fetch_int("Dynamic2/Campaign"));

  // Initial checks
  try
  {
    AutoTest::and_checker(
      CampaignChecker(
        this,
        ccg1,
        CampaignChecker::Expected().eval_status("I")),
      CampaignChecker(
        this,
        ccg2,
        CampaignChecker::Expected().eval_status("A"))).check();
  }
  catch(const AutoTest::CheckFailed& e)
  {
    if (AutoTest::TimeLessChecker(next_date).check(false))
    {
      FAIL_CONTEXT({ throw; }, description + " - initial check");
    }
    AutoTest::Logger::thlog().stream(WARNING, ASPECT)
      << "Day switched at post_condition step: '"
      << description << "' case will be omitted";
    return;
  }

  // Changes
  campaign1->budget = campaign1->budget.value() + 1;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign1->update()),
    "updating campaign.budget");

  campaign2->date_end =
    Generics::Time(fetch_string("Dynamic2/DateEnd"), "%Y-%m-%d %H:%M:%S") +
    12 * Generics::Time::ONE_DAY.tv_sec;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign2->update()),
    "updating campaign.budget");

  NSLookupRequest request;
  request.debug_time(base_time_);
  request.tid(fetch_int("Tag"));

  // Check new statuses
  add_checker("Check campaign budget was increased",
    SpentBudgetChecker(
      this,
      request.referer_kw(fetch_string("Dynamic1/Keyword")),
      ccg1,
      fetch_int("Dynamic1/CC"),
      next_date));

  add_checker("Check date_end of campaign was prolonged",
    SpentBudgetChecker(
      this,
      request.referer_kw(fetch_string("Dynamic2/Keyword")),
      ccg2,
      0,
      next_date));
}

bool
CampaignBudgetTest::checker_call(
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

void
CampaignBudgetTest::post_condition()
{
  std::string description("Dynamic part.");
  add_descr_phrase(description);

  AUTOTEST_CASE(fixed_daily_budget_update(), FIXED_CASE);
  AUTOTEST_CASE(dynamic_daily_budget_update(), DYNAMIC_CASE);

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

void
CampaignBudgetTest::tear_down()
{}
