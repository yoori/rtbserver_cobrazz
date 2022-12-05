
#include "ColocationGranularUpdateTest.hpp"

REFLECT_UNIT(ColocationGranularUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::ColocationChecker ColocationChecker;
}

void 
ColocationGranularUpdateTest::set_up()
{
  add_descr_phrase("SetUp");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager must set in the XML configuration file");
}

void 
ColocationGranularUpdateTest::tear_down()
{
  add_descr_phrase("tearDown");
}

bool 
ColocationGranularUpdateTest::run()
{
  add_colocation();
  add_colo_rate();
  update_colo_account();
  deactivate_colo();
  return true;
}

void ColocationGranularUpdateTest::add_colocation()
{
  std::string description("Add colocation.");
  add_descr_phrase(description);

  colo_->account = fetch_int("Account1");
  colo_->name = fetch_string("ColocationName");
  colo_->status = "A";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      colo_->insert()),
    description + 
      " Cann't insert colocation");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ColocationChecker(
        this,
        colo_->id(),
        ColocationChecker::Expected().
          colo_id(colo_->id()).
          colo_rate_id("0").
          account_id(fetch_string("Account1")).
          revenue_share("0.0"))).check(),
    description);
}

void ColocationGranularUpdateTest::add_colo_rate()
{
  std::string description("Add colo rate.");
  add_descr_phrase(description);

  colo_->rate.revenue_share = 0.75;
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      colo_->insert_rate()),
    description + 
      " Cann't insert colocation rate");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ColocationChecker(
        this,
        colo_->id(),
        ColocationChecker::Expected().
          colo_rate_id(colo_->rate.id()).
          revenue_share("0.75"))).check(),
      description);
}

void ColocationGranularUpdateTest::update_colo_account()
{
  std::string description("Colocation account update.");
  add_descr_phrase(description);  
  colo_->account =  fetch_int("Account2");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      colo_->update()),
    description + 
      " Cann't update colocation");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ColocationChecker(
        this,
        colo_->id(),
        ColocationChecker::Expected().
          account_id(fetch_string("Account2")))).check(),
    description);
}

void ColocationGranularUpdateTest::deactivate_colo()
{
  std::string description("Deactivate colocation.");
   add_descr_phrase(description);
   colo_->status = "D";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      colo_->update()),
     description + 
       " Cann't update colocation");

   FAIL_CONTEXT(
     AutoTest::wait_checker(
       ColocationChecker(
         this,
         colo_->id(),
         ColocationChecker::Expected(),
         AutoTest::AEC_NOT_EXISTS)).check(),
     description);
}

