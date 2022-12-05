
#include "DisputingInvoice.hpp"

REFLECT_UNIT(DisputingInvoice) (
  "CreativeSelection",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CampaignChecker CampaignChecker;
  typedef AutoTest::AccountChecker AccountChecker;
  typedef AutoTest::StatAccountChecker StatAccountChecker;
}

void
DisputingInvoice::set_up()
{
  add_descr_phrase("Setup.");

  ccgid_ = fetch_int("CCG");
  ccid_ = fetch_int("CC");
  tid_ = fetch_int("TAG");
  keyword_ = fetch_string("KWD");
  account_ = fetch_int("ACCOUNT");

  clear_stats_();

  stat.key().adv_account_id(account_);
  
  stat.select(pq_conn_);
}

void
DisputingInvoice::tear_down()
{
  add_descr_phrase("Tear down.");

  // Clear stats
  clear_stats_();
}


bool
DisputingInvoice::run()
{

  ORM::ORMRestorer<ORM::PQ::Accountfinancialdata>* acc_data =
    create<ORM::PQ::Accountfinancialdata>(fetch_int("ACCOUNT"));

  create_invoice_(acc_data);
  edit_invoice_(acc_data);

  return true;
}
  

void
DisputingInvoice::create_invoice_(
  ORM::PQ::Accountfinancialdata* acc_data)
{
  std::string description("Create invoice.");
  add_descr_phrase(description);

  FAIL_CONTEXT(
    AccountChecker(
      this,
      account_,
      AccountChecker::Expected().
        eval_status("A").
        status("A")).check(),
    description +
      " Initial check");
  
  for (size_t i = 0; i < 3; ++i)
  {
    AdClient client(AdClient::create_user(this));
    client.process_request(
      NSLookupRequest().
        tid(tid_).
        referer_kw(keyword_));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(ccid_),
      client.debug_info.ccid).check(),
    description + " Check ccid#" + strof(i+1));
  }

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AccountChecker(
        this,
        account_,
        AccountChecker::Expected().
          status("I"))).check(),
    description +
      " Account budget reached");

  acc_data->not_invoiced =
    *acc_data->not_invoiced + 3;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      acc_data->update(false)),
    "Update account financial data");

}

void
DisputingInvoice::edit_invoice_(
  ORM::PQ::Accountfinancialdata* acc_data)
{
  std::string description("Edit invoice.");
  add_descr_phrase(description);

  acc_data->select();

  acc_data->not_invoiced =
    *acc_data->not_invoiced - 3;
  acc_data->invoiced_outstanding =
    *acc_data->invoiced_outstanding + 1;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      acc_data->update(false)),
    "Update account financial data");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AccountChecker(
        this,
        account_,
        AccountChecker::Expected().
          status("A"))).check(),
    description +
      " Invoice generated");

  FAIL_CONTEXT(
    CampaignChecker(
      this,
      ccgid_,
      CampaignChecker::Expected().
        budget(100000).
        eval_status("A").
        status("A")).check(),
    description +
      " CCG initial");

  
  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(ccgid_);

  AutoTest::UpdateStats::execute(this);
  ccg->budget = *ccg->budget + 1;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update(false)),
    "Update CCG budget");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CampaignChecker(
        this,
        ccgid_,
        CampaignChecker::Expected().
          budget(100001).
          eval_status("A").
          status("A"))).check(),
    description +
      " CCG after budget update");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AccountChecker(
        this,
        account_,
        AccountChecker::Expected().
          eval_status("A").
          status("A"))).check(),
    description +
      " Account after CCG budget update");

  AdClient client(AdClient::create_user(this));
  client.process_request(
    NSLookupRequest().
    tid(tid_).
    referer_kw(keyword_));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(ccid_),
      client.debug_info.ccid).check(),
    description + " Check ccid");

  // Wait stats in DB
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        ORM::HourlyStats::Diffs().imps(4),
        stat)).check(),
    "RequestStatsHourly check");
}

void
DisputingInvoice::clear_stats_()
{
  ORM::clear_stats(pq_conn_, "cc_id", ccid_);
  ORM::clear_stats(pq_conn_, "adv_account_id", account_);
  ORM::clear_stats(pq_conn_, "account_id", account_);

  ORM::update_display_status(
    this, "account", account_);

  AutoTest::UpdateStats::execute(this);  
}
