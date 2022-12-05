
#include "CTRCalculationLogicTest.hpp"

REFLECT_UNIT(CTRCalculationLogicTest) (
  "CreativeSelection",
  AUTO_TEST_SLOW,
  AUTO_TEST_SERIALIZE
);

namespace DB  = ::AutoTest::DBC;
namespace ORM = ::AutoTest::ORM;

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::DebugInfo::SelectedCreativesList SelectedCreativesList;
  typedef AutoTest::DebugInfo::DebugInfo DebugInfo;
  typedef AutoTest::Money Money;
  typedef AutoTest::precisely_number precisely_number;
  typedef Sync::Policy::PosixThread SyncPolicy;

  const double DEFAULT_CTR = 0.01;
  const double DEFAULT_TOW = 1.0;
  const unsigned int CCG_KEYWORD_SIZE = 4;
  const unsigned int ONE_HOUR = 60 * 60;

  double calc_ctr(double clicks, double imps, double ctr)
  {
    return (clicks + (2000 * ctr)) / (2000 + imps);
  }

  double calc_kwd_ctr(double clicks, double imps,
                      double ccg_ctr, double kwd_ctr)
  {
    return (clicks + (ccg_ctr * 1000) +
            (kwd_ctr * 1000)) / (2000 + imps);
  }

  /**
   * @class CheckCTR
   * @brief Check keyword CTR.
   */
  class CheckCTR : public AutoTest::Checker
  {
  public:

    /**
     * @brief Constructor.
     *
     * @param test.
     * @param connection.
     * @param keyword.
     * @param expected CTR.
     */
    CheckCTR(
      BaseUnit* test,
      AutoTest::DBC::IConn& ora_conn,
      AutoTest::DBC::IConn& pg_conn,
      unsigned long keyword,
      Money expected_ctr) :
      test_(test),
      pq_conn_(ora_conn),
      pg_conn_(pg_conn),
      keyword_(keyword),
      expected_ctr_(expected_ctr)
    { }

    /**
     * @brief Destructor.
     */
    virtual ~CheckCTR() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_on_error = true)
      /*throw(eh::Exception)*/
    {
      AutoTest::ORM::calc_ctr(pg_conn_);

      return
        AutoTest::CCGKeywordChecker(
          test_, keyword_,
          AutoTest::ChannelManagerController,
          AutoTest::CCGKeywordChecker::Expected().
            ctr(expected_ctr_)).check(throw_on_error);
    }
    
  private:
    BaseUnit* test_;
    AutoTest::DBC::IConn& pq_conn_;
    AutoTest::DBC::IConn& pg_conn_;
    unsigned long keyword_;
    Money expected_ctr_;
  };

}



void
CTRCalculationLogicTest::set_up()
{
  add_descr_phrase("Setup");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CHANNEL_CONTROLLER)),
    "ChannelManagerController need for this test");
}

void
CTRCalculationLogicTest::tear_down()
{
  add_descr_phrase("Tear down");

  const unsigned int ccg_keywords[] = {
    static_cast<unsigned int>(fetch_int("CCGKeyword1")),
    static_cast<unsigned int>(fetch_int("CCGKeyword2")),
    static_cast<unsigned int>(fetch_int("CCGKeyword3")),
    static_cast<unsigned int>(fetch_int("CCGKeyword4"))
  };
  ORM::clear_stats(pq_conn_, "ccg_keyword_id", ccg_keywords);

  const unsigned int ccgs[] = {
    static_cast<unsigned int>(fetch_int("CCG1")),
    static_cast<unsigned int>(fetch_int("CCG2"))
  };
  ORM::clear_stats(pq_conn_, "ccg_id", ccgs);

  const unsigned int ccs[] = {
    static_cast<unsigned int>(fetch_int("CC1")),
    static_cast<unsigned int>(fetch_int("CC2"))
  };
  
  ORM::clear_stats(pq_conn_, "cc_id", ccs);

  ORM::clear_stats(pq_conn_, "country_code", fetch_string("COUNTRYCODE"));

  ORM::calc_ctr(pq_conn_);
}

bool 
CTRCalculationLogicTest::run()
{

  initial_case();
  base_case();

  return true;
}

void
CTRCalculationLogicTest::initial_case()
{
  add_descr_phrase("Initial case");

  for (unsigned int i = 0; i < CCG_KEYWORD_SIZE; ++i)
  {
    std::string keyword_name = "CCGKeyword" + strof(i+1);

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        CheckCTR(
          this, pq_conn_, pq_conn_,
          fetch_int(keyword_name),
          DEFAULT_CTR)).check(),
      keyword_name + " initial check");
  }

}

void
CTRCalculationLogicTest::base_case()
{
  add_descr_phrase("Base case");

  const unsigned int impressions1 = 200;
  const unsigned int impressions2 = 300;
  const unsigned int impressions3 = 400;
  unsigned int clicks2 = 0;
  unsigned int clicks3 = 0;
  
  // All requests make with past time to
  // simulate state "one hour late", it need
  // for making tow = 1.0 for country, campaign, etc
  // 2 hours, because calc.ctr in the beggining of the hour use
  // hour-1 row
  AutoTest::Time debug_time(
    (AutoTest::Time() - 2*ONE_HOUR).get_gm_time().format("%d-%m-%Y:%H-01-00"));

  // CCGKeyword1 impressions & clicks
  for (unsigned int i=0; i < impressions1; ++i)
  {
    AutoTest::NSLookupRequest request;
    request.tid = fetch_string("Tag1");
    request.referer_kw = fetch_string("Keyword1");
    request.loc_name = fetch_string("Country");
    request.debug_time = debug_time; 
    AutoTest::AdClient
      client(AdClient::create_user(this));
    client.process_request(request);
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("CC1"),
        SelectedCreativesCCID(client)).check(),
      "selected_creatives");
  }

  // CCGKeyword2 impressions & clicks
  for (unsigned int i=0; i < impressions2; ++i)
  {
    AutoTest::NSLookupRequest request;
    request.tid = fetch_string("Tag2");
    request.referer_kw = fetch_string("Keyword2");
    request.loc_name = fetch_string("Country");
    request.debug_time = debug_time;
    AutoTest::AdClient
      client(AdClient::create_user(this));
    client.process_request(request);
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("CC2"),
        SelectedCreativesCCID(client)).check(),
      "selected_creatives");
    
    if (i % 4 == 0)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !client.debug_info.selected_creatives.first().click_url.empty()),
        "server must return non empty click url");
      client.process_request(
        client.debug_info.selected_creatives.first().click_url,
        "CCGKeyword#2 click");
      clicks2++;
    }
  }

  // CCGKeyword4 impressions & clicks
  for (unsigned int i=0; i < impressions3; ++i)
  {
    AutoTest::NSLookupRequest request;
    request.tid = fetch_string("Tag2");
    request.referer_kw = fetch_string("Keyword3");
    request.loc_name = fetch_string("Country");
    request.debug_time = debug_time;
    AutoTest::AdClient
      client(AdClient::create_user(this));
    client.process_request(request);
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("CC2"),
        SelectedCreativesCCID(client)).check(),
      "selected_creatives");
    
    if (i % 2 == 0)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !client.debug_info.selected_creatives.first().click_url.empty()),
        "server must return non empty click url");
      client.process_request(
        client.debug_info.selected_creatives.first().click_url,
        "CCGKeyword#4 click");
      clicks3++;
    }
  }

  // Check CTRs
  // See https://confluence.ocslab.com/display/TDOCDRAFT/REQ-1929+CTR+Calculation+for+Auction
  // Section "Keyword Targeted Text Groups"

  // 2.0 * (200 + 300 + 400) = 1800
  double system_impressions = 2.0 * (impressions1 + impressions2 + impressions3);
  // 2.0 * (200 + 75) = 550
  double system_clicks = 2.0 * (clicks2 + clicks3);

  // CTRs calculation
  // (550 + 2000 * 0.01) / (200 + 1800) = 0.15
  double system_ctr = calc_ctr(system_clicks, system_impressions, DEFAULT_CTR);

  // clicks / imps = 0 / 200 = 0.0
  double ccg1_ctr = 0.0;

  // clicks / imps = (75 + 200) / (300 + 400) = 0.393
  double ccg2_ctr = 
    static_cast<double>(clicks2 + clicks3) /
      static_cast<double>(impressions2 + impressions3);

  // (2*0 +  2000 *  0.15) / (2000 + 400) = 0.125
  double kw1_ctr = calc_ctr(0.0, 2.0 * impressions1, system_ctr);
  // (2*75 + 2000 *  0.15) / (2000 + 600) = 0.173
  double kw2_ctr = calc_ctr(2.0 * clicks2, 2.0 * impressions2, system_ctr);
  // (2*200 + 2000 *  0.15) / (2000 + 800) = 0.25
  double kw3_ctr = calc_ctr(2.0 * clicks3, 2.0 * impressions3, system_ctr);

  // (2 * clicks1 + 1000 * ccg1_ctr + 1000 * kw1_ctr) / (2000 + 2 * impressions1) =
  // (2 * 0       + 1000 * 0.0      + 1000 * 0.125  ) / (2000 + 2 * 200) = 0.052 
  double ctr1 = calc_kwd_ctr(0.0, 2.0 * impressions1, ccg1_ctr, kw1_ctr);
  // (2 * clicks2 + 1000 * ccg2_ctr + 1000 * kw2_ctr) / (2000 + 2 * impressions2) =
  // (2 * 75      + 1000 * 0.393    + 1000 * 0.173  ) / (2000 + 2 * 300) = 0.275
  double ctr2 = calc_kwd_ctr(2.0 * clicks2, 2.0 * impressions2, ccg2_ctr, kw2_ctr);
  // (2 * clicks4 + 1000 * ccg2_ctr + 1000 * kw1_ctr) / (2000 + 2 * impressions4) =
  // (2 * 0       + 1000 * 0.393    + 1000 * 0.125  ) / (2000 + 2 * 0) = 0.259
  double ctr3 = calc_kwd_ctr(0.0, 0.0, ccg2_ctr, kw1_ctr);
  // (2 * clicks3 + 1000 * ccg2_ctr + 1000 * kw3_ctr) / (2000 + 2 * impressions3) =
  // (2 * 200     + 1000 * 0.393    + 1000 * 0.25  )  / (2000 + 2 * 400) = 0.372
  double ctr4 = calc_kwd_ctr(2.0 * clicks3, 2.0 * impressions3, ccg2_ctr, kw3_ctr);

  const Money expected_ctr[CCG_KEYWORD_SIZE] =
  {
    precisely_number(ctr1, 0.00001),
    precisely_number(ctr2, 0.00001),
    precisely_number(ctr3, 0.00001),
    precisely_number(ctr4, 0.00001)
  };

 
  for (unsigned int i = 0; i < CCG_KEYWORD_SIZE; ++i)
  {
    std::string keyword_name = "CCGKeyword" + strof(i+1);

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        CheckCTR(
          this, pq_conn_, pq_conn_,
          fetch_int(keyword_name),
          expected_ctr[i])).check(),
      keyword_name + " check chanded CTR#" + strof(i+1));
  }
  
}
