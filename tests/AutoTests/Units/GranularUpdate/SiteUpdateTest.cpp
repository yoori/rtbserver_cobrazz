/* $Id$
* @file SiteUpdateTest.cpp
* Test that server dynamically update Site.
* For more info see https://confluence.ocslab.com/display/ADS/SiteUpdateTest
*/
#include "SiteUpdateTest.hpp"

REFLECT_UNIT(SiteUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::SiteChecker SiteChecker;
  typedef AutoTest::WaitChecker<SiteChecker> SiteWaitChecker;
  typedef AutoTest::FreqCapChecker FreqCapChecker;
  typedef AutoTest::WaitChecker<FreqCapChecker> FreqCapWaitChecker;
  typedef AutoTest::SelectedCreativeChecker SelectedCreativeChecker;
  typedef AutoTest::NSLookupRequest NSLRequest;
  typedef AutoTest::AdClient Client;

  namespace ORM = ::AutoTest::ORM;
}

void SiteUpdateTest::set_up()
{
  add_descr_phrase("Setup");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
      "CampaignManager must set in the XML configuration file");
}

void SiteUpdateTest::create_site_()
{
  unsigned long freq_cap_id = fetch_int("InsertSite/FC");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      FreqCapChecker(
        this,
        freq_cap_id,
        FreqCapChecker::Expected(),
        AutoTest::AEC_NOT_EXISTS)).check(),
    "FreqCap initial");

  unsigned int account_id = fetch_int("Global/ACCOUNT");

  ORM::ORMRestorer<ORM::PQ::Site>* site =
    create<ORM::PQ::Site>();
  site->name = fetch_string("InsertSite/Site/NAME");
  site->account = account_id;
  site->qa_status = "A";
  site->site_url = "www.unittest.com";
  site->status = "A";
  site->freq_cap = freq_cap_id;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      site->insert()),
    "inserting site");

  ADD_WAIT_CHECKER(
    "Site check",
    SiteChecker(
      this,
      site->id(),
      SiteChecker::Expected().
        site_id(site->id()).
        status("A").
        approved_creative_categories("").
        rejected_creative_categories("").
        approved_creatives("").
        account_id(strof(account_id)).
        noads_timeout("0") ));

  ADD_WAIT_CHECKER(
    "FreqCap check",
    FreqCapWaitChecker(
      FreqCapChecker(
        this,
        freq_cap_id,
        FreqCapChecker::Expected().
          window_time(
            fetch_string("InsertSite/FC/WindowLength")) )));
}

void SiteUpdateTest::update_site_campaign_approval_()
{

  std::string creative_category = fetch_string("Global/VCAT");
  unsigned int site_id = fetch_int("UpdateSiteCreativeApproval/Publisher/SITE_ID");
  unsigned int tag_id = fetch_int("UpdateSiteCreativeApproval/Publisher/TAG_ID");

  // Initial check
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      SiteChecker(this, site_id,
        SiteChecker::Expected().
          site_id(site_id).
          status("A").
          approved_creative_categories(creative_category).
          rejected_creative_categories("").
          approved_creatives(fetch_string("Global/CREATIVEID")) )).check(),
    "Initial");

  Client client(Client::create_user(this));

  NSLRequest request;
  request.referer_kw = fetch_string("Global/KEYWORD");
  request.tid = tag_id;

  client.process_request(request, "request for creative");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Global/CCID"),
      client.debug_info.ccid).check(),
    "server must return expected creative");

  ORM::ORMRestorer<ORM::PQ::SiteCreativeApproval>* sca =
    create<ORM::PQ::SiteCreativeApproval>(
      ORM::PQ::SiteCreativeApproval(pq_conn_, fetch_int("Global/CREATIVEID"), site_id));
  sca->approval = "R";
  sca->approval_date.set_now();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      sca->update()),
    "updating SiteCreativeApproval");

  ADD_WAIT_CHECKER(
    "Site check",
    SiteChecker(
      this,
      site_id,
      SiteChecker::Expected().
        site_id(site_id).
        status("A").
        approved_creative_categories(creative_category).
        rejected_creative_categories("").
        approved_creatives("") ));

  ADD_WAIT_CHECKER(
      "Request for no creative",
      SelectedCreativeChecker(client, request, 0));
}

void SiteUpdateTest::update_noads_timeout_()
{
  std::string cc_id = fetch_string("Global/CCID");
  unsigned int site_id = fetch_int("UpdateNoAdsTimeout/Publisher/SITE_ID");
  unsigned int tag_id = fetch_int("UpdateNoAdsTimeout/Publisher/TAG_ID");

  // Initial check
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      SiteChecker(this, site_id,
        SiteChecker::Expected().
          site_id(strof(site_id)).
          status("A").
          noads_timeout("40") )).check(),
    "Initial");

  Client client(Client::create_user(this));

  NSLRequest request;
  request.referer_kw = fetch_string("Global/KEYWORD");
  request.tid = tag_id;
  request.debug_time = base_time_;

  client.process_request(request, "request for no creative");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "server must return expected creative");

  request.debug_time = base_time_ + 20;
  client.process_request(request, "request for no creative");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "server must return expected creative");

  request.debug_time = base_time_ + 40;
  client.process_request(request, "request for creative");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_id,
      client.debug_info.ccid).check(),
    "server must return expected creative");

  ORM::ORMRestorer<ORM::PQ::Site>* site = create<ORM::PQ::Site>(site_id);
  site->no_ads_timeout = 50;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(site->update()),
    "updatind no_ads_timeout");

  ADD_WAIT_CHECKER(
    "Site check",
    SiteChecker(
      this,
      site_id,
      SiteChecker::Expected().
        site_id(strof(site_id)).
        status("A").
      noads_timeout("50") ));

  request.debug_time = base_time_;
  ADD_WAIT_CHECKER(
    "Request for no creative at base_time",
    SelectedCreativeChecker(client, request, 0));

  request.debug_time = base_time_ + 20;
  ADD_WAIT_CHECKER(
    "Request for no creative at base_time + 20",
    SelectedCreativeChecker(client, request, 0));

  request.debug_time = base_time_ + 40;
  ADD_WAIT_CHECKER("Request for no creative at base_time + 40",
    SelectedCreativeChecker(client, request, 0));

  request.debug_time = base_time_ + 49;
  ADD_WAIT_CHECKER("Request for no creative at base_time + 49",
    SelectedCreativeChecker(client, request, 0));

  request.debug_time = base_time_ + 50;
  ADD_WAIT_CHECKER("Request for no creative at base_time + 50",
    SelectedCreativeChecker(client, request, cc_id));
}

void SiteUpdateTest::update_creative_exclusion_()
{
  std::string ccg_id = fetch_string("Global/CCGID");
  std::string creative_id = fetch_string("Global/CREATIVEID");
  unsigned int creative_category = fetch_int("Global/VCAT");
  unsigned int site_id = fetch_int("UpdateCreativeExclusion/Publisher/SITE_ID");
  unsigned int tag_id = fetch_int("UpdateCreativeExclusion/Publisher/TAG_ID");

  // Initial
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      SiteChecker(this, site_id,
        SiteChecker::Expected().
          site_id(strof(site_id)).
          status("A").
          approved_creative_categories(strof(creative_category)).
          rejected_creative_categories("").
          approved_creatives(creative_id) )).check(),
    "Initial");

  Client client(Client::create_user(this));

  NSLRequest request;
  request.referer_kw = fetch_string("Global/KEYWORD");
  request.tid = tag_id;

  client.process_request(request, "request for creative");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Global/CCID"),
      client.debug_info.ccid).check(),
    "musr return expected ccid");

  ORM::ORMRestorer<ORM::PQ::SiteCreativeCategoryExclusion>* scce =
    create<ORM::PQ::SiteCreativeCategoryExclusion>(
      ORM::PQ::SiteCreativeCategoryExclusion(pq_conn_, creative_category, site_id));
  scce->approval = "R";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      scce->update()),
    "updating creative category exclusion");

  ADD_WAIT_CHECKER(
    "Site checker",
    SiteChecker(
      this,
      site_id,
      SiteChecker::Expected().
        site_id(strof(site_id)).
        status("A").
        approved_creative_categories("").
        rejected_creative_categories(strof(creative_category)).
        approved_creatives(creative_id) ));

  ADD_WAIT_CHECKER(
    "CCID checker",
    SelectedCreativeChecker(
      client, request, 0));
}

void SiteUpdateTest::delete_site_()
{
  unsigned int site_id = fetch_int("DeleteSite/Publisher/SITE_ID");
  unsigned int freq_cap_id = fetch_int("DeleteSite/Publisher/FC");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      FreqCapChecker(
        this,
        freq_cap_id,
        FreqCapChecker::Expected().
          window_time(
            fetch_string(
              "DeleteSite/FC/WindowLength")) )).check(),
    "FreqCap initial");

  // Initial
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      SiteChecker(this, site_id,
        SiteChecker::Expected().
          site_id(strof(site_id)).
          status("A") )).check(),
    "Site initial");

  ORM::ORMRestorer<ORM::PQ::Site>* site =
    create<ORM::PQ::Site>(site_id);

  //TODO: remove after ADSC-6857 resolving
  site->display_status_id = 6;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      site->update()),
    "updating site display status");
  // end of TODO
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      site->del()),
    "deleting site");

  ADD_WAIT_CHECKER(
    "Site check",
    SiteChecker(
      this,
      site_id,
      SiteChecker::Expected().
        site_id(strof(site_id)),
      AutoTest::AEC_NOT_EXISTS ));

  ADD_WAIT_CHECKER(
    "FreqCap check",
    FreqCapChecker(
      this,
      freq_cap_id,
      FreqCapChecker::Expected(),
      AutoTest::AEC_NOT_EXISTS ));

}

void SiteUpdateTest::update_site_freq_caps_()
{
  unsigned long windows_length =
    fetch_int("UpdateFC/FC/WindowLength");

  ORM::ORMRestorer<ORM::PQ::FreqCap>* freq_cap =
    create<ORM::PQ::FreqCap>(
      fetch_int("UpdateFC/Publisher/FC"));  

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      FreqCapChecker(
        this,
        freq_cap->id(),
        FreqCapChecker::Expected().
          window_time(strof(windows_length)) )).check(),
    "FreqCap initial check");
  
  freq_cap->window_length = windows_length + 100;
    
  FAIL_CONTEXT(
    freq_cap->update(),
    "Can't update fcap");  

  ADD_WAIT_CHECKER(
    "FreqCap check",
    FreqCapChecker(
      this,
      freq_cap->id(),
      FreqCapChecker::Expected().
        window_time(strof(windows_length + 100)) ));
  
}

void SiteUpdateTest::tear_down()
{
  add_descr_phrase("Tear down");
}

bool 
SiteUpdateTest::run()
{
  AUTOTEST_CASE(
    create_site_(),
    "Create site");

  AUTOTEST_CASE(
    update_site_campaign_approval_(),
    "Update SiteCreativeApproval");

 AUTOTEST_CASE(
   update_noads_timeout_(),
   "Update noads timeout");

  AUTOTEST_CASE(
    update_creative_exclusion_(),
    "Update creative exclusion");
    
  AUTOTEST_CASE(
    delete_site_(),
    "Delete site");

  AUTOTEST_CASE(
    update_site_freq_caps_(),
    "Update site freq caps");

  return true;
}

