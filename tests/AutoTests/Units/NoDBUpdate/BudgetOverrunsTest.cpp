
#include "BudgetOverrunsTest.hpp"
 
REFLECT_UNIT(BudgetOverrunsTest) (
  "NoDBUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CampaignChecker CampaignChecker;

  namespace ORM = AutoTest::ORM;

  const char* CASE_NAME[] = {
    "Spent budget less than total budget of text CCG",
    "Spent budget less than total budget of campaign",
    "Not spent budget of display net campaign",
    "Spent budget for display cpm campaign",
    "Spent budget more than total budget of display cpc CCG",
    "Spent budget equal to total budget of channel targeted text CCG",
    "Spent budget more than total budget of campaign",
    "Spent budget equal to total budget of campaign",
    "Campaign with huge rate",
    "Display campaign linked with rated channel",
    "Text campaign linked with rated channel",
    "Spent budget of display gross campaign",
    "Spent budget of text gross campaign",
    "Spent budget of text net campaign",
    "Blank budget for channel targeted text CCG",
    "Blank budget for keyword targeted text CCG",
    "Campaign with zero budget"
  };

  class SpentBudgetChecker: public AutoTest::Checker
  {
  public:

    SpentBudgetChecker(BaseUnit* test,
                       const NSLookupRequest& request,
                       unsigned long ccg_id,
                       const std::string& cc_id):
      test_(test),
      request_(request),
      ccg_id_(ccg_id),
      cc_id_(cc_id)
    {}

    ~SpentBudgetChecker() noexcept
    {}

    bool check(bool throw_error = true) /*throw(eh::Exception)*/
    {
      if (AutoTest::wait_checker(CampaignChecker(test_, ccg_id_,
            CampaignChecker::Expected().eval_status(cc_id_ == "0" ? "I" : "A"))).check(throw_error))
      {
        AdClient client(AdClient::create_user(test_));
        client.process_request(request_);
        if (AutoTest::equal(cc_id_, client.debug_info.ccid))
        {
          return true;
        }
        else if (throw_error)
        {
          Stream::Error error;
          error
            << "Got unexpected ccid - " << cc_id_ << "!= "
            << client.debug_info.ccid << " (expected != got)";
          throw AutoTest::CheckFailed(error);
        }
      }
      return false;
    }
  private:
    BaseUnit* test_;
    NSLookupRequest request_;
    unsigned long ccg_id_;
    std::string cc_id_;
  };

  std::string case_name(size_t i)
  {
    return countof(CASE_NAME) > i ? CASE_NAME[i] : "Case #" + strof(i);
  }
}

const BudgetOverrunsTest::TestCase BudgetOverrunsTest::TEST_CASES[] = {
  { "TextCPC",              BudgetOverrunsTest::TCF_CPC_RATE | BudgetOverrunsTest::TCF_NOT_SPENT, 0, &BudgetOverrunsTest::decrease_budget_of_ccg },
  { "CampaignMoreBudget",   BudgetOverrunsTest::TCF_CPC_RATE | BudgetOverrunsTest::TCF_NOT_SPENT, 0, &BudgetOverrunsTest::decrease_budget_of_campaign },
  { "DisplayNet",           BudgetOverrunsTest::TCF_NOT_SPENT,                                    0, &BudgetOverrunsTest::spent_the_rest_budget },
  { "DisplayCPM",           0,                                                                    0, &BudgetOverrunsTest::increase_budget_of_ccg },
  { "DisplayCPC",           BudgetOverrunsTest::TCF_CPC_RATE,                                     0, 0 },
  { "ChannelTargetedCPC",   BudgetOverrunsTest::TCF_CPC_RATE,                                     0, 0 },
  { "CampaignLessBudget",   BudgetOverrunsTest::TCF_CPC_RATE,                                     0, &BudgetOverrunsTest::increase_budget_of_campaign },
  { "CampaignEqualBudget",  BudgetOverrunsTest::TCF_CPC_RATE,                                     0, 0 },
  { "CampaignUnlimBudget",  BudgetOverrunsTest::TCF_CPC_RATE,                                     0, &BudgetOverrunsTest::set_unlim_budget_for_campaign },
  { "DisplayChannelRate",   0,                                                                    0, 0 },
  { "TextChannelRate",      0,                                                                    0, 0 },
  { "DisplayGross",         0,                                                                    0, 0 },
  { "TextGross",            BudgetOverrunsTest::TCF_CPC_RATE,                                     0, 0 },
  { "TextNet",              BudgetOverrunsTest::TCF_CPC_RATE,                                     0, 0 },
  { "CTTextBlankBudget",    BudgetOverrunsTest::TCF_CPC_RATE,                                     0, &BudgetOverrunsTest::set_unlim_budget_for_ccg },
  { "KTTextBlankBudget",    BudgetOverrunsTest::TCF_CPC_RATE,                                     0, &BudgetOverrunsTest::set_unlim_budget_for_ccg }
};

void BudgetOverrunsTest::process_case(size_t i)
{
  if (countof(TEST_CASES) <= i)
  { return; }

  const unsigned int ccg = fetch_int(TEST_CASES[i].prefix + "/CCG");
  const std::string cc = fetch_string(TEST_CASES[i].prefix + "/CC");
  const double budget = fetch_float(TEST_CASES[i].prefix + "/Budget");
  const double request_revenue = fetch_float(TEST_CASES[i].prefix + "/Revenue");

  FAIL_CONTEXT(CampaignChecker(this, ccg,
      CampaignChecker::Expected().status("A").eval_status("A")).check(),
    "Initial check");

  unsigned int requests_count = TEST_CASES[i].requests_count
    ? TEST_CASES[i].requests_count
    : static_cast<unsigned int> (ceil(budget / request_revenue));

  if (TEST_CASES[i].flags & TCF_NOT_SPENT)
  { requests_count--; }

  NSLookupRequest request;
  request.tid = fetch_int("Tag");
  request.referer_kw = fetch_string(TEST_CASES[i].prefix + "/Keyword");

  for (size_t j = 0; j < requests_count; ++j)
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(request, "request for creative");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc,
        client.debug_info.ccid).check(),
      "server must return expected creative");
    
    if (TEST_CASES[i].flags & TCF_CPC_RATE)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !client.debug_info.click_url.empty()),
        "server must return non empty click url");
      client.process_request(client.debug_info.click_url, "click on creative");
    }
  }

  add_checker("Check budget",
    SpentBudgetChecker(
      this,
      request,
      ccg,
      TEST_CASES[i].flags & TCF_NOT_SPENT ? cc : "0"));
}

void
BudgetOverrunsTest::campaign_zero_budget()
{
  add_checker("Check campaign budget",
    SpentBudgetChecker(this,
      NSLookupRequest().
        referer_kw(fetch_string("CampaignZeroBudget/Keyword")).
        tid(fetch_int("Tag")),
      fetch_int("CampaignZeroBudget/CCG"), "0"));
}

bool 
BudgetOverrunsTest::run()
{
  size_t cases_count = countof(TEST_CASES);

  for (size_t i = 0; i < cases_count; ++i)
  {
    AUTOTEST_CASE(process_case(i), case_name(i));
  }

  AUTOTEST_CASE(campaign_zero_budget(), "Campaign with zero budget");

  check();

  return true;
}

void
BudgetOverrunsTest::increase_budget_of_ccg(const std::string& prefix)
{
  std::string description("Activate creative group.");
  add_descr_phrase(description);

  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(
      fetch_int(prefix + "/CCG"));

  // Initial check
  FAIL_CONTEXT(CampaignChecker(this, ccg->id(),
      CampaignChecker::Expected().eval_status("I")).check(),
    description + " Initial check");

  ccg->budget = ccg->budget.value() + fetch_float(prefix + "/Budget");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update()),
    description + " Updating ccg.budget");

  add_checker(description,
    SpentBudgetChecker(
      this,
      NSLookupRequest().
        referer_kw(fetch_string(prefix + "/Keyword")).
        tid(fetch_int("Tag")),
      ccg->id(),
      fetch_string(prefix + "/CC")));
}

void
BudgetOverrunsTest::decrease_budget_of_ccg(const std::string& prefix)
{
  std::string description("Deactivate creative group.");
  add_descr_phrase(description);

  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(
      fetch_int(prefix + "/CCG"));

  // Initial check
  FAIL_CONTEXT(CampaignChecker(this, ccg->id(),
      CampaignChecker::Expected().eval_status("A")).check(),
    description + "Initial check");

  ccg->budget = 1;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update()),
    description + "updating ccg.budget");

  add_checker(
    description,
    SpentBudgetChecker(
      this,
      NSLookupRequest().
        referer_kw(fetch_string(prefix + "/Keyword")).
        tid(fetch_int("Tag")),
      ccg->id(),
      "0"));
}

void
BudgetOverrunsTest::increase_budget_of_campaign(const std::string& prefix)
{
  std::string description("Activate campaign.");
  add_descr_phrase(description);

  unsigned long ccg = fetch_int(prefix + "/CCG");

  // Initial check
  FAIL_CONTEXT(CampaignChecker(this, ccg,
      CampaignChecker::Expected().eval_status("I")).check(),
    description + "Initial check");

  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>(
      fetch_int(prefix + "/Campaign"));

  campaign->budget = campaign->budget.value() + fetch_float(prefix + "/Budget");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->update()),
    description + "updating campaign.budget");

  add_checker(
    description,
    SpentBudgetChecker(
      this,
      NSLookupRequest().
        referer_kw(fetch_string(prefix + "/Keyword")).
        tid(fetch_int("Tag")),
      ccg,
      fetch_string(prefix + "/CC")));
}

void
BudgetOverrunsTest::decrease_budget_of_campaign(const std::string& prefix)
{
  std::string description("Deactivate campaign.");
  add_descr_phrase(description);

  unsigned long ccg = fetch_int(prefix + "/CCG");

  // Initial check
  FAIL_CONTEXT(CampaignChecker(this, ccg,
      CampaignChecker::Expected().eval_status("A")).check(),
    description + "Initial check");

  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>(
      fetch_int(prefix + "/Campaign"));

  campaign->budget = 1;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->update()),
    description + "updating campaign.budget");

  add_checker(
    description,
    SpentBudgetChecker(
      this,
      NSLookupRequest().
        referer_kw(fetch_string(prefix + "/Keyword")).
        tid(fetch_int("Tag")),
      ccg,
      "0"));
}

void
BudgetOverrunsTest::set_unlim_budget_for_campaign(const std::string& prefix)
{
  std::string description("Set unlim budget for campaign.");
  add_descr_phrase(description);

  unsigned long ccg = fetch_int(prefix + "/CCG");

  // Initial check
  FAIL_CONTEXT(CampaignChecker(this, ccg,
      CampaignChecker::Expected().eval_status("I")).check(),
    description + "Initial check");

  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>(
      fetch_int(prefix + "/Campaign"));

  campaign->budget.null();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->update()),
    description + "updating campaign.budget");

  add_checker(
    description,
    SpentBudgetChecker(
      this,
      NSLookupRequest().
        referer_kw(fetch_string(prefix + "/Keyword")).
        tid(fetch_int("Tag")),
      ccg,
      fetch_string(prefix + "/CC")));
}

void
BudgetOverrunsTest::set_unlim_budget_for_ccg(const std::string& prefix)
{
  std::string description("Set unlim budget for creative group.");
  add_descr_phrase(description);

  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(
      fetch_int(prefix + "/CCG"));

  // Initial check
  FAIL_CONTEXT(CampaignChecker(this, ccg->id(),
      CampaignChecker::Expected().eval_status("I")).check(),
    description + "Initial check");

  ccg->budget.null();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update()),
    description + "updating ccg.budget");

  add_checker(
    description,
    SpentBudgetChecker(
      this,
      NSLookupRequest().
        referer_kw(fetch_string(prefix + "/Keyword")).
        tid(fetch_int("Tag")),
      ccg->id(),
      fetch_string(prefix + "/CC")));
}

void
BudgetOverrunsTest::spent_the_rest_budget(const std::string& prefix)
{
  std::string description("Spent the rest budget of campaign.");
  add_descr_phrase(description);

  add_checker(
    description,
    SpentBudgetChecker(
      this,
      NSLookupRequest().
        referer_kw(fetch_string(prefix + "/Keyword")).
        tid(fetch_int("Tag")),
      fetch_int(prefix + "/CCG"),
      "0"));
}

void
BudgetOverrunsTest::post_condition()
{
  std::string description("Dynamic.");
  add_descr_phrase(description);

  for (size_t i = 0; i < countof(TEST_CASES); ++i)
  {
    if (TEST_CASES[i].dynamic_part)
    { 
      AUTOTEST_CASE(
        (this->*(TEST_CASES[i].dynamic_part))(TEST_CASES[i].prefix),
        case_name(i));
    }
  }

  // check(); - it will be called implicitly
}

void
BudgetOverrunsTest::tear_down()
{}
