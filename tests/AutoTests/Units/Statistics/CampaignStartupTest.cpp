#include "CampaignStartupTest.hpp"
#include <math.h>

REFLECT_UNIT(CampaignStartupTest) (
  "Statistics",
  AUTO_TEST_SLOW,
  AUTO_TEST_SERIALIZE
);

namespace DB = ::AutoTest::DBC;
namespace ORM = ::AutoTest::ORM;

namespace
{
  const double DEFAULT_DISPLAY_CTR = 0.01;
  const int req_count = 500;

  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::ORM::HourlyStats HourlyStats;
  typedef AutoTest::Money Money;
  typedef Sync::Policy::PosixThread SyncPolicy;

  /**
   * @class CheckEcpm
   * @brief Check campaign eCPM.
   */
  class CheckEcpm : public AutoTest::Checker
  {
   public:

    /**
     * @brief Constructor.
     *
     * @param test.
     * @param connection.
     * @param CCG ID.
     * @param expected eCPM.
     */
    CheckEcpm(
      BaseUnit* test,
      DB::IConn& conn,
      unsigned long ccg_id,
      const Money& expected_ecpm) :
      test_(test),
      conn_(conn),
      ccg_id_(ccg_id),
      expected_ecpm_(expected_ecpm)
    { }

    /**
     * @brief Destructor.
     */
    virtual ~CheckEcpm() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_on_error = true)
      /*throw(eh::Exception)*/
    {

      AutoTest::ORM::calc_ctr(conn_);

      return
        AutoTest::CampaignChecker(
          test_,
          ccg_id_,
          AutoTest::CampaignChecker::Expected().
            status("A").
            eval_status("A").
            country("ad").
            ecpm(expected_ecpm_)).check(throw_on_error);
    }
    
  private:
    BaseUnit* test_;
    DB::IConn& conn_;
    unsigned long ccg_id_;
    Money expected_ecpm_;
  };

}

void CampaignStartupTest::set_up()
{
  // Activate campaign
  add_descr_phrase("Setup");
  const int ccg_id = fetch_int("CCGID");

  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(ccg_id);
  FAIL_CONTEXT(
    AutoTest::predicate_checker(ccg->select()),
    "should found CCG");

  ccg->status = "A";
  ccg->country_code = fetch_string("COUNTRYCODE");
  ccg->set_display_status(AutoTest::ORM::DS_LIVE);

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update()),
    "should activate CCG");
}

void CampaignStartupTest::initial_check()
{
  add_descr_phrase("Initial check started");
  const int ccg_id = fetch_int("CCGID");
  //const int cpc = fetch_int("CPC");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CheckEcpm(
        this,
        pq_conn_,
        ccg_id,
        0.0)).check(),
    "Campaign ecpm initial state");
}

void CampaignStartupTest::base_scenario()
{
  add_descr_phrase("Base scenario started");
  const int ccg_id = fetch_int("CCGID");
  const int cc_id = fetch_int("CCID");
  const int cpc = fetch_int("CPC");

  Generics::Time now = Generics::Time::get_time_of_day();
  HourlyStats stats;
  stats.key().cc_id(cc_id).num_shown(1).stimestamp(now);
  stats.select(pq_conn_);

  // Make 500 advertising request for campaign 
  // and test that server return campaign creative.
  request.referer_kw = fetch_string("KEYWORD");
  request.tid = fetch_string("TID");
  request.format = "unit-test";
  request.loc_name = fetch_string("Country");
  request.debug_time = now;

  StrVector exp_ccids;
  exp_ccids.push_back(strof(cc_id));

  for (int i = 0; i < req_count; ++i)
  {
    AdClient client = AdClient::create_user(this);

    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "unexpected creatives");
  }

  {
    HourlyStats::Diffs diff;
    diff
      .imps(req_count)
      .requests(req_count);

    // See section "Display CPC Creative Groups" for CTR calculation
    // https://confluence.ocslab.com/display/TDOCDRAFT/REQ-3122+Random+Adserving
    // CTR = 0! No clicks!
    FAIL_CONTEXT(
      AutoTest::and_checker(
        AutoTest::wait_checker(
          AutoTest::stats_diff_checker(
            pq_conn_, diff, stats)),
        AutoTest::wait_checker(
          CheckEcpm(
            this,
            pq_conn_,
            ccg_id,
            0.0))).check(),
      "must got expected stats and ecpm");

  }

  // Make 500 advertising request for campaign 
  // and test that server return creative.
  stats.key().cc_id(cc_id).num_shown(1).stimestamp(now);
  stats.select(pq_conn_);
  for (int i = 0; i < req_count; ++i)
  {
    AdClient client = AdClient::create_user(this);

    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "unexpected creatives");

    FAIL_CONTEXT(client.process_request(
      client.debug_info.selected_creatives.first().click_url,
      "campaign click"));
  };

  {
    // See section "Display CPC Creative Groups" for CTR calculation
    // https://confluence.ocslab.com/display/TDOCDRAFT/REQ-3122+Random+Adserving
    // History CTR (<CG>) used

    double ctr = 0.5; // imps = req_count * 2, clicks = req_count

    double exp_ecpm = ctr * cpc * 100 * 1000;

    HourlyStats::Diffs diff;
    diff
      .imps(req_count)
      .requests(req_count)
      .clicks(req_count);

    FAIL_CONTEXT(
      AutoTest::and_checker(
        AutoTest::wait_checker(
          AutoTest::stats_diff_checker(
            pq_conn_, diff, stats)),
      AutoTest::wait_checker(
        CheckEcpm(
          this,
          pq_conn_,
          ccg_id,
          exp_ecpm))).check(),
      "must got expected stats and ecpm");

  }
}

void CampaignStartupTest::tear_down()
{
  add_descr_phrase("Tear down");

  ORM::clear_stats(pq_conn_, "ccg_id", static_cast<int>(fetch_int("CCGID")));
  ORM::clear_stats(pq_conn_, "country_code", fetch_string("COUNTRYCODE"));
  ORM::calc_ctr(pq_conn_);
}


bool 
CampaignStartupTest::run()
{
  initial_check();
  base_scenario();

  return true;
}
