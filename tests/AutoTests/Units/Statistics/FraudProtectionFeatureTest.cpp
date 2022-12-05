
#include "FraudProtectionFeatureTest.hpp"
 
REFLECT_UNIT(FraudProtectionFeatureTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{

  const int IMP_FRAUD_COUNT = 15;
  const int CLICK_FRAUD_COUNT = 10;
  
  namespace ORM = ::AutoTest::ORM;
  typedef GlobalConfig::ServiceList ServiceList;
  typedef ServiceList::const_iterator Service_iterator;
  typedef AutoTest::FraudProfileChecker RIAUserFraud;
  typedef AutoTest::FraudProfileEmptyChecker RIAUserNoFraud;
  typedef AutoTest::RequestProfileChecker RIARequestFraud;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ActionRequest ActionRequest;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;

  enum FraudEnum
  {
    FE_FRAUD,
    FE_GENUINE
  };

  enum TestRequestFlags
  {
    TRF_IMP              = 1,   // User send impression 
    TRF_CLICK            = 2,   // User send click 
    TRF_ACTION           = 4,   // User send adv_action
    TRF_FRAUD            = 8,   // Request sequence is a fraud
    TRF_NOFRAUD_PARAM    = 16,  // Send debug.no_fraud parameter of the nslookup request
    TRF_WAIT_FRAUD       = 32,  // Wait until server detected fraud
    TRF_SPECIFY_VERSION  = 64,  // Use specific version ('v' parameter of nslookup request)
    TRF_ACTIONS_NOW      = 128, // Use current time in action requests (imps, clicks, actions)
    TRF_SKIP_RIA_CHECKS  = 256  // Skip RequestInfoAdmin checks
  };

  /**
   * @class FraudStats
   * @brief Expected fraud stats.
   */
  class FraudStats
  {
  public:

    // Revenue calculation logic
    enum RevenueTypeEnum
    {
      RTE_CPM,
      RTE_CPC,
      RTE_CPA
    };
    
    
  public:
    /**
     * @brief Constructor.
     *
     * @param fraud sign.
     * @param requests count.
     * @param impressions count.
     * @param clicks count.
     * @param actions count.
     * @param revenue for one impression/click/action.
     */
    FraudStats(
      FraudEnum fraud,
      int requests,
      int imps = 0,
      int clicks = 0,
      int actions = 0,
      double revenue = ORM::any_stats_diff,
      RevenueTypeEnum revenue_type = RTE_CPM) :
      fraud_(fraud),
      requests_(requests),
      imps_(imps),
      clicks_(clicks),
      actions_(actions),
      revenue_(revenue),
      revenue_type_(revenue_type)
    { }

    /**
     * @brief Get requests count.
     *
     * @param fraud sign.
     * @return requests count.
     */
    int requests(FraudEnum fraud) const
    {
      return fraud == FE_GENUINE? requests_:
        fraud_ == FE_FRAUD? -requests_: 0;
    }

    /**
     * @brief Get impressions count.
     *
     * @param fraud sign.
     * @return impressions count.
     */
    int imps(FraudEnum fraud) const
    {
      return fraud == FE_GENUINE? imps_:
        fraud_ == FE_FRAUD? -imps_: 0;
    }

    /**
     * @brief Get clicks count.
     *
     * @param fraud sign.
     * @return clicks count.
     */
    int clicks(FraudEnum fraud) const
    {
      return fraud == FE_GENUINE? clicks_:
        fraud_ == FE_FRAUD? -clicks_: 0;
    }

    /**
     * @brief Get actions count.
     *
     * @param fraud sign.
     * @return actions count.
     */
    int actions(FraudEnum fraud) const
    {
      return fraud == FE_GENUINE? actions_: 0;
    }

    /**
     * @brief Get total revenue.
     *
     * @param fraud sign.
     * @return total revenue.
     */
    ORM::stats_diff_type revenue(FraudEnum fraud) const
    {
      return revenue_ == ORM::any_stats_diff?
        revenue_: fraud == FE_GENUINE || fraud_ == FE_FRAUD?
        calc_revenue(fraud): 0;
    }

  private:
    ORM::stats_diff_type calc_revenue(FraudEnum fraud) const
    {
      switch(revenue_type_)
      {
      case RTE_CPM: return revenue_ * imps(fraud);
      case RTE_CPC: return revenue_ * clicks(fraud);
      case RTE_CPA: return revenue_ * actions(fraud);
      }
      return 0;
    }

  private:
    FraudEnum fraud_;
    int requests_;
    int imps_;
    int clicks_;
    int actions_;
    double revenue_;
    RevenueTypeEnum revenue_type_; 
  };


  /**
   * @class StatsHourlyFraudCheck
   * @brief Check RequestStatsHourly fraud pair.
   */
  class StatsHourlyFraudCheck : public AutoTest::Checker
  {
    typedef ORM::HourlyStats StatsHourly;
    typedef ORM::StatsArray<StatsHourly, 2> StatsHourlyFraud;

  public:

    /**
     * @brief Constructor.
     *
     * @param test.
     * @param connection.
     * @param RequestStatsHourly#sdate.
     * @param RequestStatsHourly#ccid.
     * @param expected stats.
     * @param RequestStatsHourly#num_shown.
     * @param use RequestStatsHourlyTest instead RequestStatsHourly.
     */
    StatsHourlyFraudCheck(
      BaseUnit* test,
      AutoTest::DBC::Conn& conn,
      const AutoTest::Time& sdate,
      const char* cc,
      const FraudStats& stats,
      unsigned long num_shown = 1,
      bool test_table = false) :
      conn_(conn),
      stats_(stats)
    {
      unsigned long ccid = test->fetch_int(cc);
      fraud_[0].
        table(test_table?
          StatsHourly::RequestStatsHourlyTest: StatsHourly::RequestStatsHourly).
        key().
        cc_id(ccid).
        num_shown(num_shown).
        fraud_correction(false).
        stimestamp(sdate);
      fraud_[1].
        table(test_table?
          StatsHourly::RequestStatsHourlyTest: StatsHourly::RequestStatsHourly).
        key().
        cc_id(ccid).
        num_shown(num_shown).
        fraud_correction(true).
        stimestamp(sdate);

      fraud_.select(conn_);
    }

    /**
     * @brief Destructor.
     */
    ~StatsHourlyFraudCheck() noexcept
    { }

    /**
     * @brief Set expected stats.
     *
     * @param expected stats.     
     */
    void set_stats(
      const FraudStats& stats)
    {
      stats_ = stats;
      fraud_.select(conn_);
    }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_error = true) /*throw(eh::Exception)*/
    {
      ORM::HourlyStats::Diffs diffs[2];
      init_diff(diffs[0], FE_GENUINE);
      init_diff(diffs[1], FE_FRAUD);
      return AutoTest::stats_diff_checker(
        conn_, diffs, fraud_).check(throw_error);
    }

  private:
    /**
     * @brief RequestStatsHourly diff initialization.
     * @param RequestStatsHourly#diff.
     * @param fraud sign.
     */
    void init_diff(
      ORM::HourlyStats::Diffs& diff,
      FraudEnum fraud)
    {
      diff
        .imps(stats_.imps(fraud))
        .requests(stats_.requests(fraud))
        .clicks(stats_.clicks(fraud))
        .actions(stats_.actions(fraud))
        .adv_amount(stats_.revenue(fraud));
    }

  private:
    AutoTest::DBC::Conn& conn_;
    FraudStats stats_;
    StatsHourlyFraud fraud_;
  };

  struct StatsHourlyChecker
  {
    std::string description;
    StatsHourlyFraudCheck checker;
  };

   /**
   * @class CmpStatsFraudCheck
   * @brief Check CmpStatsHourly fraud pair.
   */
  class CmpStatsFraudCheck : public AutoTest::Checker
  {
    typedef ORM::CMPRequestStats CMPStats;
    typedef ORM::StatsArray<CMPStats, 2> CMPStatsFraud;

  public:

    /**
     * @brief Constructor.
     *
     * @param test.
     * @param connection.
     * @param CmpStatsHourly#sdate.
     * @param CmpStatsHourly#channels.
     * @param expected stats.
     */
    CmpStatsFraudCheck(
      BaseUnit* test,
      AutoTest::DBC::Conn& conn,
      const AutoTest::Time& sdate,
      const char* channel,
      const FraudStats& stats) :
      conn_(conn),
      stats_(stats)
    {
      unsigned long channelid = test->fetch_int(channel);
      
      fraud_[0].key(
        CMPStats::Key().
        channel_id(channelid).
        fraud_correction(false). //
        sdate(sdate));
      fraud_[1].key(
        CMPStats::Key().
        channel_id(channelid).
        fraud_correction(true). //
        sdate(sdate));

      fraud_.select(conn_);
    }

    /**
     * @brief Destructor.
     */
    ~CmpStatsFraudCheck() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_error = true) /*throw(eh::Exception)*/
    {
      CMPStats::Diffs diffs[] =
      {
        CMPStats::Diffs().
          imps(stats_.imps(FE_GENUINE)).
          clicks(stats_.clicks(FE_GENUINE)).
          adv_amount_cmp(stats_.revenue(FE_GENUINE)).
          cmp_amount(stats_.revenue(FE_GENUINE)).
          cmp_amount_global(stats_.revenue(FE_GENUINE)),
        CMPStats::Diffs().
          imps(stats_.imps(FE_FRAUD)).
          clicks(stats_.clicks(FE_FRAUD)).
          adv_amount_cmp(stats_.revenue(FE_FRAUD)).
          cmp_amount(stats_.revenue(FE_FRAUD)).
          cmp_amount_global(stats_.revenue(FE_FRAUD))
      };

      return AutoTest::stats_diff_checker(
        conn_, diffs, fraud_).check(throw_error);
    }

  private:
    AutoTest::DBC::Conn& conn_;
    FraudStats stats_;
    CMPStatsFraud fraud_;
  };

  struct CmpStatsChecker
  {
    std::string description;
    CmpStatsFraudCheck checker;
  };

  /**
   * @class ClientFraudChecker
   * @brief Use for waiting adserver fraud detection.
   */
  class ClientFraudChecker : public AutoTest::Checker
  {
  public:

    /**
     * @brief Constructor.
     *
     * @param client.
     * @param referer_kw.
     */
    ClientFraudChecker(
      AutoTest::AdClient& client,
      const NSLookupRequest& request) :
      client_(client),
      request_(request)
    { }

    /**
     * @brief Destructor.
     */
    ~ClientFraudChecker() noexcept
    { }

     /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_error = true) /*throw(eh::Exception)*/
    {
      client_.process_request(request_);

      if (AutoTest::equal(
            "true",
            client_.debug_info.fraud_request))
          return true;
      if (throw_error)
      {
        throw AutoTest::CheckFailed(
          "Fraud expected!");
      }
      return false;
    }

  private:
    AutoTest::AdClient& client_;
    NSLookupRequest request_;
  };
}

template<size_t Count>
void
FraudProtectionFeatureTest::process_request(
  AdClient& client,
  const TestRequest (&data)[Count],
  const AutoTest::Time& dtime)
{
  std::set<std::string> users;
  
  for (size_t i = 0; i < Count; ++i)
  {
    NSLookupRequest request;
    request.format      = data[i].format;  
    request.tid         = fetch_string(data[i].tid);
    if (data[i].referer_kw)
    {
      request.referer_kw = map_objects(data[i].referer_kw);
    }
    
    if (data[i].flags & TRF_NOFRAUD_PARAM)
    {
      request.debug_nofraud = 1;
    }

    if (data[i].flags & TRF_SPECIFY_VERSION)
    {
      request.version  = fetch_string("ClientVersion");
    }
    
    AutoTest::Time debug_time(
      dtime == Generics::Time::ZERO? today: dtime);

    debug_time += data[i].time_ofset;
    
    request.debug_time  = debug_time;

    for (unsigned long j = 0; j < data[i].count; ++j)
    {
      client.process_request(request);

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          data[i].expected_channels,
          client.debug_info.history_channels).check(),
        "Expected history#" + strof(i));

      // Creatives was returned
      if (data[i].expected_ccs)
      {
        std::list<std::string> expected_ccs;
        fetch_objects(
          std::inserter(expected_ccs, expected_ccs.begin()),
          data[i].expected_ccs);
        
        if (!(data[i].flags & TRF_SKIP_RIA_CHECKS))
        {
          ADD_WAIT_CHECKER(
            "Fraud request check",
            RIARequestFraud(
              this,
              "\\" +
              client.debug_info.creative_request_id.value(),
              AutoTest::RequestInfoManager,
              RIARequestFraud::Expected().
              fraud(data[i].flags & TRF_FRAUD? "1": "0")));
        }
        
        AutoTest::ConsequenceActionList actions;

        AutoTest::Time action_time(
          data[i].flags & TRF_ACTIONS_NOW?
            AutoTest::Time(): debug_time);
     
        // Send impression requests
        if (data[i].flags & TRF_IMP)
        {
          actions.push_back(
            AutoTest::ConsequenceAction(
              AutoTest::TRACK,
              action_time));
        }
      
        // Send click requests
        if (data[i].flags & TRF_CLICK)
        {
          actions.push_back(
            AutoTest::ConsequenceAction(
              AutoTest::CLICK,
              action_time));
        }
        
        // Send action requests
        if (data[i].flags & TRF_ACTION)
        {
          actions.push_back(
            AutoTest::ConsequenceAction(
              AutoTest::ACTION,
              action_time));
        }

        FAIL_CONTEXT(
          client.do_ad_requests(
            expected_ccs, actions));

      }
      // No creatives returned
      else
      {
        FAIL_CONTEXT(
          AutoTest::equal_checker(
            "0",
            client.debug_info.ccid).check(),
          "Unexpected ccid#" + strof(i));
      }
    }

    if (data[i].flags & TRF_WAIT_FRAUD)
    {
      request.tid.clear();
      FAIL_CONTEXT(
        AutoTest::wait_checker(
          ClientFraudChecker(
            client, request),
          AutoTest::GlobalSettings::instance().
            wait_timeout(),
          10).check(),
        "No fraud detected");
    }

    if (!(data[i].flags & TRF_SKIP_RIA_CHECKS))
    {
      auto uid = users.insert(
        AutoTest::prepare_uid(client.get_uid()));
        
      if (uid.second)
      {
        if (data[i].flags & TRF_FRAUD)
        {
          ADD_WAIT_CHECKER(
            "Fraudster check",
            AutoTest::and_checker(
              RIAUserNoFraud(
                this,
                *uid.first,
                AutoTest::RequestInfoManager,
                "Profile not found.",
                AutoTest::AEC_NOT_EXISTS),
              RIAUserFraud(
                this,
                *uid.first,
                AutoTest::RequestInfoManager,
                RIAUserFraud::Expected().
                  fraud_time(
                    Generics::Time::ZERO.
                      get_gm_time().
                      format("%Y-%m-%d %H:%M:%S")),
                AutoTest::AEC_NOT_EXISTS)));
        }
        else
        {
          ADD_WAIT_CHECKER(
            "Genuine check",
            AutoTest::or_checker(
              RIAUserNoFraud(
                this,
                *uid.first,
                AutoTest::RequestInfoManager,
                "Profile not found."),
              RIAUserFraud(
                this,
                *uid.first,
                AutoTest::RequestInfoManager,
                RIAUserFraud::Expected().
                  fraud_time(
                    Generics::Time::ZERO.
                      get_gm_time().
                      format("%Y-%m-%d %H:%M:%S")))));
        }
      }
    }
  }
}

void 
FraudProtectionFeatureTest::set_up()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_REQUEST_INFO_MANAGER)),
    "RequestInfoManager must set in the XML configuration file");
}

bool 
FraudProtectionFeatureTest::run()
{
  
  AUTOTEST_CASE(
    impression_fraud_("IMPFRAUD", false),
    "Impression fraud (no wait)");

  AUTOTEST_CASE(
    impression_fraud_("IMPWAITFRAUD", true),
    "Impression fraud (wait)");

  AUTOTEST_CASE(
    click_fraud_(),
    "Click fraud");

  AUTOTEST_CASE(
    genuine_user_(),
    "Genuine user");

  AUTOTEST_CASE(
    mixed_(),
    "Mixed display and text");

  AdClient cpa_client(AdClient::create_user(this));
  
  AUTOTEST_CASE(
    cpa_part_1_(cpa_client),
    "Clicks condition, actions corrections");

  AUTOTEST_CASE(
    no_fraud_(),
    "No fraud parameter");

  AUTOTEST_CASE(
    ta_no_fraud_(),
    "Channel targeted text no fraud");

  AUTOTEST_CASE(
    ta_channel_fraud_(),
    "Channel targeted text fraud");

  AUTOTEST_CASE(
    unconfirmed_imps_(),
    "Unconfirmed impressions");

  AUTOTEST_CASE(
    merging_(),
    "Merging");

  AUTOTEST_CASE(
    fraud_override_(),
    "Fraud override");

  AUTOTEST_CASE(
    delayed_clicks_(),
    "Delayed clicks");

  AdClient reversed_order_fraud_client(
    AdClient::create_user(this));

  AUTOTEST_CASE(
    reversed_order_fraud_part_1_(
      reversed_order_fraud_client),
    "Reversed order fraud");

  check();

  AUTOTEST_CASE(
    cpa_part_2_(cpa_client),
    "Clicks condition, actions corrections");

  AUTOTEST_CASE(
    reversed_order_fraud_part_2_(
      reversed_order_fraud_client),
    "Reversed order fraud");

  return true;
}

void 
FraudProtectionFeatureTest::tear_down()
{ }

void FraudProtectionFeatureTest::click_fraud_()
{
  StatsHourlyFraudCheck rsh_checker(
    this, pq_conn_, today, "CLICKFRAUD/CC",
    FraudStats(
      FE_FRAUD,
      CLICK_FRAUD_COUNT,
      CLICK_FRAUD_COUNT,
      CLICK_FRAUD_COUNT, 0,
      fetch_float("CLICKFRAUD/CCCPM")));

  CmpStatsFraudCheck cmp_checker(
    this, pq_conn_, today, "CLICKFRAUD/CHANNEL",
    FraudStats(
      FE_FRAUD,
      CLICK_FRAUD_COUNT,
      CLICK_FRAUD_COUNT,
      CLICK_FRAUD_COUNT, 0,
      fetch_float("CLICKFRAUD/CHANNELCPM")));

  ORM::UserPropertiesStats stats_user(
    ORM::UserPropertiesStats::Key().
      name("ClientVersion").
      value(fetch_string("ClientVersion")).
      colo_id(1).
      user_status("I"));
    
  stats_user.select(pq_conn_);

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    rsh_checker);

  ADD_WAIT_CHECKER(
    "CMPStats check",
    cmp_checker);

  ADD_WAIT_CHECKER(
    "UserProperties",
    AutoTest::stats_diff_checker(
      pq_conn_,
      ORM::UserPropertiesStats::Diffs().
        requests(CLICK_FRAUD_COUNT).
        imps(CLICK_FRAUD_COUNT).
        clicks(CLICK_FRAUD_COUNT).
        imps_unverified(CLICK_FRAUD_COUNT),
      stats_user));

  const FraudProtectionFeatureTest::TestRequest CLICK_FRAUD[] =
  {
    {
      "CLICKFRAUD/KWD",
      "CLICKFRAUD/TID",
      "unit-test-imp",
      0,
      "CLICKFRAUD/CHANNEL",
      "CLICKFRAUD/CC",
      CLICK_FRAUD_COUNT,
      TRF_IMP | TRF_CLICK | TRF_FRAUD | TRF_SPECIFY_VERSION
    }
  };

  AdClient client(AdClient::create_user(this));
  
  process_request(client, CLICK_FRAUD);
}

void FraudProtectionFeatureTest::genuine_user_()
{
  StatsHourlyFraudCheck rsh_checker(
    this, pq_conn_, today, "GENUINE/CC",
    FraudStats(
      FE_GENUINE,
      IMP_FRAUD_COUNT - 1,
      IMP_FRAUD_COUNT - 1,
      CLICK_FRAUD_COUNT - 1, 0,
      fetch_float("GENUINE/CCCPM")));

  CmpStatsFraudCheck cmp_checker(
    this, pq_conn_, today, "GENUINE/CHANNEL",
    FraudStats(
      FE_GENUINE,
      IMP_FRAUD_COUNT - 1,
      IMP_FRAUD_COUNT - 1,
      CLICK_FRAUD_COUNT - 1, 0,
      fetch_float("GENUINE/CHANNELCPM")));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    rsh_checker);

  ADD_WAIT_CHECKER(
    "CMPStats check",
    cmp_checker);

  const FraudProtectionFeatureTest::TestRequest GENUINE_USER[] =
  {
    {
      "GENUINE/KWD",
      "GENUINE/TID",
      "unit-test-imp",
      0,
      "GENUINE/CHANNEL",
      "GENUINE/CC",
      CLICK_FRAUD_COUNT - 1, TRF_IMP | TRF_CLICK,
    },
    {
      "GENUINE/KWD",
      "GENUINE/TID",
      "unit-test-imp",
      0,
      "GENUINE/CHANNEL",
      "GENUINE/CC",
      IMP_FRAUD_COUNT - CLICK_FRAUD_COUNT, TRF_IMP,
    }    
  };

  AdClient client(AdClient::create_user(this));
  
  process_request(client, GENUINE_USER);
}

void FraudProtectionFeatureTest::mixed_()
{
  StatsHourlyChecker RSH_STATS[] =
  {
    {
      "'CMP' stats.",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "CPM/CC",
        FraudStats(
          FE_FRAUD, 6, 6, 0, 0,
          fetch_float("CPM/CCCPM")))
    },
    {
      "Text#1 stats.",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "CHANNELTEXT/CC1",
        FraudStats(
          FE_FRAUD, 9, 9, 0, 0,
          fetch_float("CHANNELTEXT/CCCPM1")), 2)
    },
    {
      "Text#2 stats.",
       StatsHourlyFraudCheck(
        this, pq_conn_, today, "CHANNELTEXT/CC2",
        FraudStats(FE_FRAUD, 9, 9, 0, 0,
          fetch_float("CHANNELTEXT/CCCPM2")), 2)
    }
  };

  CmpStatsChecker CMP_STATS[] =
  {
    {
      "'CMP' channel stats.",
      CmpStatsFraudCheck(
        this, pq_conn_, today, "CPM/CHANNEL",
        FraudStats(FE_FRAUD, 6, 6, 0, 0,
          fetch_float("CPM/CHANNELCPM")))
    },
    {
      "Text channel#1 stats.",
      CmpStatsFraudCheck(
        this, pq_conn_, today, "CHANNELTEXT/CHANNEL1",
        FraudStats(
          FE_FRAUD, 9, 9, 0, 0,
          fetch_float("CHANNELTEXT/CHANNELCPM1")))
    },
    {
      "Text channel#2 stats.",
      CmpStatsFraudCheck(
        this, pq_conn_, today, "CHANNELTEXT/CHANNEL2",
        FraudStats(FE_FRAUD, 9, 9, 0, 0,
          fetch_float("CHANNELTEXT/CHANNELCPM2")))
    }
  };

  for (size_t i = 0; i < countof(RSH_STATS); ++i)
  {
    ADD_WAIT_CHECKER(
      RSH_STATS[i].description,
      RSH_STATS[i].checker);
  }

  for (size_t i = 0; i < countof(CMP_STATS); ++i)
  {
    ADD_WAIT_CHECKER(
      CMP_STATS[i].description,
      CMP_STATS[i].checker);
  }

  const FraudProtectionFeatureTest::TestRequest MIXED[] =
  {
    {
      "CPM/KWD",
      "CPM/TID",
      "unit-test",
      0,
      "CPM/CHANNEL",
      "CPM/CC",
      6, TRF_FRAUD
    },
    {
      "CHANNELTEXT/KWD1,CHANNELTEXT/KWD2",
      "CHANNELTEXT/TID",
      "unit-test",
      0,
      "CHANNELTEXT/CHANNEL1,CHANNELTEXT/CHANNEL2",
      "CHANNELTEXT/CC1,CHANNELTEXT/CC2",
      9, TRF_FRAUD
    }
  };

  AdClient client(AdClient::create_user(this));
  
  process_request(client, MIXED);
}

void FraudProtectionFeatureTest::unconfirmed_imps_()
{
  StatsHourlyFraudCheck rsh_checker(
    this, pq_conn_, today, "UNCONFIRMEDIMPS/CC",
    FraudStats(
      FE_GENUINE, IMP_FRAUD_COUNT + 5, 0, 0, 0,
      fetch_float("UNCONFIRMEDIMPS/CCCPM")));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    rsh_checker);

  const FraudProtectionFeatureTest::TestRequest UNCONFIRMED_IMPS[] =
  {
    {
      "UNCONFIRMEDIMPS/KWD",
      "UNCONFIRMEDIMPS/TID",
      "unit-test-imp",
      0,
      "UNCONFIRMEDIMPS/CHANNEL",
      "UNCONFIRMEDIMPS/CC",
      IMP_FRAUD_COUNT + 5, 0
    }
  };

  AdClient client(AdClient::create_user(this));
  
  process_request(client, UNCONFIRMED_IMPS);
}

void FraudProtectionFeatureTest::cpa_part_1_(
  AdClient& client)
{
  StatsHourlyFraudCheck rsh_checker(
    this, pq_conn_, today, "CPA/CC",
    FraudStats(
      FE_FRAUD,
      CLICK_FRAUD_COUNT + 1,
      CLICK_FRAUD_COUNT + 1,
      CLICK_FRAUD_COUNT + 1,
      CLICK_FRAUD_COUNT + 1,
      fetch_float("CPA/CCCPA"),
      FraudStats:: RTE_CPA));

  CmpStatsFraudCheck cmp_checker(
    this, pq_conn_, today, "CPA/CHANNEL",
    FraudStats(
      FE_FRAUD,
      CLICK_FRAUD_COUNT + 1,
      CLICK_FRAUD_COUNT + 1,
      CLICK_FRAUD_COUNT + 1,
      CLICK_FRAUD_COUNT + 1,
      fetch_float("CPA/CHANNELCPM")));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    rsh_checker);

  ADD_WAIT_CHECKER(
    "CMPStats check",
    cmp_checker);

  const FraudProtectionFeatureTest::TestRequest CPA[] =
  {
    {
      "CPA/KWD",
      "CPA/TID",
      "unit-test",
      0,
      "CPA/CHANNEL",
      "CPA/CC",
      CLICK_FRAUD_COUNT + 1, TRF_CLICK | TRF_ACTION | TRF_FRAUD 
    }
  };

  process_request(client, CPA);
}

void FraudProtectionFeatureTest::cpa_part_2_(
  AdClient& client)
{
  StatsHourlyFraudCheck rsh_checker(
    this, pq_conn_, today, "CPA/CC",
    FraudStats(
      FE_GENUINE, 0, 0, 0, 1,
      fetch_float("CPA/CCCPA"),
      FraudStats:: RTE_CPA));
    
  client.process_request(
    NSLookupRequest().
    referer_kw(fetch_string("CPA/KWD")).
    tid(fetch_string("CPA/TID")).
    debug_time(today));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    " Clicks condition, actions corrections."
    " Unexpected ccid (Post action).");

  client.process_request(
    ActionRequest().
    cid(fetch_string("CPA/CCG")).
    debug_time(today));

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    rsh_checker);
}

void FraudProtectionFeatureTest::no_fraud_()
{
  StatsHourlyFraudCheck rsh_checker(
    this, pq_conn_, today, "NOFRAUD/CC",
    FraudStats(
      FE_GENUINE,
      IMP_FRAUD_COUNT + 5,
      IMP_FRAUD_COUNT + 5,
      IMP_FRAUD_COUNT + 5,
      IMP_FRAUD_COUNT + 5,
      fetch_float("NOFRAUD/CCCPA"),
      FraudStats:: RTE_CPA), 1, true);

  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    rsh_checker);

  const FraudProtectionFeatureTest::TestRequest NO_FRAUD[] =
  {
    {
      "NOFRAUD/KWD",
      "NOFRAUD/TID",
      "unit-test-imp",
      0,
      "NOFRAUD/CHANNEL",
      "NOFRAUD/CC",
      IMP_FRAUD_COUNT + 5, TRF_IMP | TRF_CLICK | TRF_ACTION | TRF_NOFRAUD_PARAM
    }
  };

  AdClient client(AdClient::create_user(this));

  process_request(client, NO_FRAUD);
}

void FraudProtectionFeatureTest::ta_no_fraud_()
{
  StatsHourlyChecker RSH_STATS[] =
  {
    {
      "Creative#1",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "TANOFRAUD/CC1",
        FraudStats(
          FE_GENUINE,
          IMP_FRAUD_COUNT - 5,
          IMP_FRAUD_COUNT - 5, 0, 0,
          fetch_float("TANOFRAUD/CCCPM1")), 2)
    },
    {
      "Creative#2",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "TANOFRAUD/CC2",
        FraudStats(
          FE_GENUINE,
          IMP_FRAUD_COUNT - 5,
          IMP_FRAUD_COUNT - 5, 0, 0,
          fetch_float("TANOFRAUD/CCCPM2")), 2)
    }
  };

  CmpStatsChecker CMP_STATS[] =
  {
    {
      "Channel#1",
      CmpStatsFraudCheck(
        this, pq_conn_, today, "TANOFRAUD/CHANNEL1",
        FraudStats(
          FE_GENUINE,
          CLICK_FRAUD_COUNT,
          CLICK_FRAUD_COUNT, 0, 0,
          fetch_float("TANOFRAUD/CHANNELCPM1")))
    },
    {
      "Channel#2",
      CmpStatsFraudCheck(
        this, pq_conn_, today, "TANOFRAUD/CHANNEL2",
        FraudStats(FE_GENUINE,
          CLICK_FRAUD_COUNT,
          CLICK_FRAUD_COUNT, 0, 0,
          fetch_float("TANOFRAUD/CHANNELCPM2")))
    }
  };

  for (size_t i = 0; i < countof(RSH_STATS); ++i)
  {
    ADD_WAIT_CHECKER(
      RSH_STATS[i].description,
      RSH_STATS[i].checker);
  }

  for (size_t i = 0; i < countof(CMP_STATS); ++i)
  {
    ADD_WAIT_CHECKER(
      CMP_STATS[i].description,
      CMP_STATS[i].checker);
  }

  const FraudProtectionFeatureTest::TestRequest TA_NO_FRAUD[] =
  {
    {
      "TANOFRAUD/KWD1,TANOFRAUD/KWD2",
      "TANOFRAUD/TID",
      "unit-test",
      0,
      "TANOFRAUD/CHANNEL1,TANOFRAUD/CHANNEL2",
      "TANOFRAUD/CC1,TANOFRAUD/CC2",
      IMP_FRAUD_COUNT - 5, 0
    }
  };

  AdClient client(AdClient::create_user(this));
  
  process_request(client, TA_NO_FRAUD);
}


void FraudProtectionFeatureTest::ta_channel_fraud_()
{
  StatsHourlyChecker RSH_STATS[] =
  {
    {
      "Text advertising fraud. (Creative#1)",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "TAFRAUD/CC1",
        FraudStats(
          FE_FRAUD,
          IMP_FRAUD_COUNT + 5,
          IMP_FRAUD_COUNT + 5, 1, 0,
          fetch_float("TAFRAUD/CCCPC1"),
          FraudStats:: RTE_CPC), 4)
    },
    {
      "Text advertising fraud. (Creative#2)",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "TAFRAUD/CC2",
        FraudStats(
          FE_FRAUD,
          IMP_FRAUD_COUNT + 5,
          IMP_FRAUD_COUNT + 5, 1, 0,
          fetch_float("TAFRAUD/CCCPC2"),
          FraudStats:: RTE_CPC), 4)
    },
    {
      "Text advertising fraud. (Creative#3)",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "TAFRAUD/CC3",
        FraudStats(
          FE_FRAUD,
          IMP_FRAUD_COUNT + 5,
          IMP_FRAUD_COUNT + 5, 1, 0,
          fetch_float("TAFRAUD/CCCPC3"),
          FraudStats:: RTE_CPC), 4)
    },
    {
      "Text advertising fraud. (Creative#4)",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "TAFRAUD/CC4",
        FraudStats(FE_FRAUD,
          IMP_FRAUD_COUNT + 5,
          IMP_FRAUD_COUNT + 5, 1, 0,
          fetch_float("TAFRAUD/CCCPC4"),
          FraudStats:: RTE_CPC), 4)
    }
  };

  CmpStatsFraudCheck cmp_checker(
    this, pq_conn_, today, "TAFRAUD/CHANNEL",
    FraudStats(FE_FRAUD,
      (IMP_FRAUD_COUNT + 5) * 4,
      (IMP_FRAUD_COUNT + 5) * 4, 4, 0,
      fetch_float("TAFRAUD/CHANNELCPM")));

  for (size_t i = 0; i < countof(RSH_STATS); ++i)
  {
    ADD_WAIT_CHECKER(
      RSH_STATS[i].description,
      RSH_STATS[i].checker);
  }

  ADD_WAIT_CHECKER(
    "CMPStats check",
    cmp_checker);

  const FraudProtectionFeatureTest::TestRequest TA_FRAUD[] =
  {
    {
      "TAFRAUD/KWD",
      "TAFRAUD/TID",
      "unit-test",
      0,
      "TAFRAUD/CHANNEL",
      "TAFRAUD/CC1,TAFRAUD/CC2,TAFRAUD/CC3,TAFRAUD/CC4",
      1, TRF_FRAUD | TRF_CLICK
    },
    {
      "TAFRAUD/KWD",
      "TAFRAUD/TID",
      "unit-test",
      0,
      "TAFRAUD/CHANNEL",
      "TAFRAUD/CC1,TAFRAUD/CC2,TAFRAUD/CC3,TAFRAUD/CC4",
      IMP_FRAUD_COUNT + 4, TRF_FRAUD
    }
  };

  AdClient client(AdClient::create_user(this));
  
  process_request(client, TA_FRAUD);
}

void FraudProtectionFeatureTest::ta_text_fraud_()
{
  StatsHourlyChecker RSH_STATS[] =
  {
    {
      "Text fraud. (Creative#1)",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "FRAUDTEXT/CC1",
        FraudStats(FE_FRAUD,
          IMP_FRAUD_COUNT,
          IMP_FRAUD_COUNT, 5, 0), 2)
    },
    {
      "Text fraud. (Creative#2)",
      StatsHourlyFraudCheck(
        this, pq_conn_, today, "FRAUDTEXT/CC2",
        FraudStats(FE_FRAUD,
          IMP_FRAUD_COUNT,
          IMP_FRAUD_COUNT, 5, 0), 2)
    }
  };

  for (size_t i = 0; i < countof(RSH_STATS); ++i)
  {
    ADD_WAIT_CHECKER(
      RSH_STATS[i].description,
      RSH_STATS[i].checker);
  }

  const FraudProtectionFeatureTest::TestRequest TEXT_FRAUD[] =
  {
    {
      "FRAUDTEXT/KWD",
      "FRAUDTEXT/TID",
      "unit-test",
      0,
      "FRAUDTEXT/CHANNEL",
      "FRAUDTEXT/CC1,FRAUDTEXT/CC2",
      5, TRF_FRAUD | TRF_CLICK
    },
    {
      "FRAUDTEXT/KWD",
      "FRAUDTEXT/TID",
      "unit-test",
      0,
      "FRAUDTEXT/CHANNEL",
      "FRAUDTEXT/CC1,FRAUDTEXT/CC2",
      IMP_FRAUD_COUNT - 5, TRF_FRAUD
    }
  };

  AdClient client(AdClient::create_user(this));

  process_request(client, TEXT_FRAUD);
}

void FraudProtectionFeatureTest::merging_()
{

  // Impression fraud.
  const FraudProtectionFeatureTest::TestRequest IMPRESSION_FRAUD[] =
  {
    {
      "IMPFRAUD/KWD",
      "IMPFRAUD/TID",
      "unit-test-imp",
      0,
      "IMPFRAUD/CHANNEL",
      "IMPFRAUD/CC",
      IMP_FRAUD_COUNT, TRF_IMP | TRF_FRAUD
    }
  };

  AdClient fraudster(AdClient::create_user(this));
  process_request(fraudster, IMPRESSION_FRAUD);
  AdClient genuine(AdClient::create_user(this));
  
  TemporaryAdClient temporary1(TemporaryAdClient::create_user(this));
  TemporaryAdClient temporary2(TemporaryAdClient::create_user(this));

  NSLookupRequest request;
  request.referer_kw = fetch_string("IMPFRAUD/KWD");
  request.debug_time = today;

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ClientFraudChecker(
        fraudster, request),
      AutoTest::GlobalSettings::instance().
        wait_timeout(),
      10).check());

  fraudster.merge(temporary1);
  genuine.merge(temporary2);

  request.tid = fetch_string("IMPFRAUD/TID");

  genuine.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("IMPFRAUD/CC"),
      genuine.debug_info.ccid).check(),
    "Merging. Genuine user expected ccid");

  fraudster.process_request(request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      fraudster.debug_info.ccid).check(),
    "Merging. Fraudster unexpected ccid");

}

void FraudProtectionFeatureTest::impression_fraud_(
  const std::string& suffix,
  bool wait_fraud)
{
  Generics::Time midday(
    (today.get_gm_time().format("%d-%m-%Y") +
       ":" + "12-00-01"), "%d-%m-%Y:%H-%M-%S");

  double cc_cpm = fetch_float(suffix + "/CCCPM");
  double channel_cpm = fetch_float(suffix + "/CHANNELCPM");
  unsigned long channel_id = fetch_int(suffix + "/CHANNEL");
  unsigned long cc_id = fetch_int(suffix + "/CC");
  
  unsigned long inactivity_period = fetch_int("UserInactivityPeriod");
  unsigned long fraud_period_1 = 60;


  ORM::StatsArray<ORM::HourlyStats, 2> rsh_stats;
  rsh_stats[0].key().
    cc_id(cc_id).num_shown(1).fraud_correction(false).stimestamp(midday);
  rsh_stats[1].key().
    cc_id(cc_id).num_shown(1).fraud_correction(true).stimestamp(midday);
  rsh_stats.select(pq_conn_);

  ORM::StatsArray<ORM::CMPRequestStats, 2> cmp_stats;
  cmp_stats[0].key(
    ORM::CMPRequestStats::Key().
      channel_id(channel_id).
      fraud_correction(false).
      sdate(midday));
  cmp_stats[1].key(
      ORM::CMPRequestStats::Key().
      channel_id(channel_id).
      fraud_correction(true).
      sdate(midday));
  cmp_stats.select(pq_conn_);

  std::string kwd(suffix + "/KWD");
  std::string tid(suffix + "/TID");
  std::string channel(suffix + "/CHANNEL");
  std::string cc(suffix + "/CC");

  int impressions =
    wait_fraud? IMP_FRAUD_COUNT + 1:
      IMP_FRAUD_COUNT + 2;

  {
    ORM::HourlyStats::Diffs diffs[2] = 
    {
      ORM::HourlyStats::Diffs()
      .imps(impressions)
      .clicks(0)
      .actions(0)
      .requests(impressions)
      .adv_amount(cc_cpm * impressions),
      ORM::HourlyStats::Diffs()
      .imps(-impressions + 1)
      .clicks(0)
      .actions(0)
      .requests(-impressions + 1)
      .adv_amount(cc_cpm * (-impressions + 1))
    };

    ADD_WAIT_CHECKER(
      "RequestStatsHourly check",
      AutoTest::stats_diff_checker(
        pq_conn_, diffs, rsh_stats));
  }

  {
    // CMPRequestStatsHourly
    ORM::CMPRequestStats::Diffs diffs[] =
    {
      ORM::CMPRequestStats::Diffs().
        imps(impressions).
        clicks(0).
        adv_amount_cmp(channel_cpm * impressions).
        cmp_amount(channel_cpm * impressions).
        cmp_amount_global(channel_cpm * impressions),
      ORM::CMPRequestStats::Diffs().
        imps(-impressions + 1).
        clicks(0).
        adv_amount_cmp(channel_cpm * (-impressions + 1)).
        cmp_amount(channel_cpm * (-impressions + 1)).
        cmp_amount_global(channel_cpm * (-impressions + 1))
    };

    ADD_WAIT_CHECKER(
      "CMPRequestStatsHourly check",
      AutoTest::stats_diff_checker(
        pq_conn_, diffs, cmp_stats));
  }

  // Fraud state transition.
  const FraudProtectionFeatureTest::TestRequest IMPRESSION_FRAUD[] =
  {
    {
      kwd.c_str(),
      tid.c_str(),
      "unit-test-imp",
      0,
      channel.c_str(),
      cc.c_str(),
      IMP_FRAUD_COUNT,
      static_cast<unsigned long>(TRF_IMP | TRF_FRAUD |
        (wait_fraud? TRF_WAIT_FRAUD: 0))
    },
    {
      kwd.c_str(),
      tid.c_str(),
      "unit-test-imp",
      inactivity_period + fraud_period_1 - 1,
      channel.c_str(),
      wait_fraud? 0: cc.c_str(),
      1,  static_cast<unsigned long>((wait_fraud? 0: TRF_FRAUD | TRF_IMP))
    },
    {
      kwd.c_str(),
      tid.c_str(),
      "unit-test-imp",
      inactivity_period + fraud_period_1 + 1,
      channel.c_str(),
      cc.c_str(),
      1, static_cast<unsigned long>(TRF_IMP)
    }
  };

  AdClient client(AdClient::create_user(this));
  
  process_request(
    client, IMPRESSION_FRAUD, midday);
  
}

void FraudProtectionFeatureTest::fraud_override_()
{
  // Fix midday time to avoid request hour switching
  Generics::Time midday(
    (today.get_gm_time().format("%d-%m-%Y") +
       ":" + "12-00-01"), "%d-%m-%Y:%H-%M-%S");
  
  double cc_cpm = fetch_float("FRAUDOVERRIDE/CCCPM");
  double channel_cpm = fetch_float("FRAUDOVERRIDE/CHANNELCPM");
  unsigned long channel_id = fetch_int("FRAUDOVERRIDE/CHANNEL");
  unsigned long cc_id = fetch_int("FRAUDOVERRIDE/CC");
  unsigned long inactivity_period = fetch_int("UserInactivityPeriod");
  unsigned long fraud_period_2 = 3*inactivity_period;


  ORM::StatsArray<ORM::HourlyStats, 2> rsh_stats;
  rsh_stats[0].key().
    cc_id(cc_id).num_shown(1).fraud_correction(false).stimestamp(midday);
  rsh_stats[1].key().
    cc_id(cc_id).num_shown(1).fraud_correction(true).stimestamp(midday);
  rsh_stats.select(pq_conn_);

  ORM::StatsArray<ORM::CMPRequestStats, 2> cmp_stats;
  cmp_stats[0].key(
    ORM::CMPRequestStats::Key().
      channel_id(channel_id).
      fraud_correction(false).
      sdate(midday));
  cmp_stats[1].key(
      ORM::CMPRequestStats::Key().
      channel_id(channel_id).
      fraud_correction(true).
      sdate(midday));
  cmp_stats.select(pq_conn_);

  {
    // RequestStatsHourly
    ORM::HourlyStats::Diffs diffs[2] = 
    {
      ORM::HourlyStats::Diffs()
      .imps(41)
      .clicks(40)
      .actions(0)
      .requests(41)
      .adv_amount(cc_cpm * 41),
      ORM::HourlyStats::Diffs()
      .imps(-40)
      .clicks(-40)
      .actions(0)
      .requests(-40)
      .adv_amount(cc_cpm * -40)
    };

    ADD_WAIT_CHECKER(
      "RequestStatsHourly check",
      AutoTest::stats_diff_checker(
        pq_conn_, diffs, rsh_stats));
  }

  {
    ORM::CMPRequestStats::Diffs diffs[] =
    {
      ORM::CMPRequestStats::Diffs().
        imps(41).
        clicks(40).
        adv_amount_cmp(channel_cpm * 41).
        cmp_amount(channel_cpm * 41).
        cmp_amount_global(channel_cpm * 41),
      ORM::CMPRequestStats::Diffs().
        imps(-40).
        clicks(-40).
        adv_amount_cmp(channel_cpm * -40).
        cmp_amount(channel_cpm * -40).
        cmp_amount_global(channel_cpm * -40)
    };

    ADD_WAIT_CHECKER(
      "CMPRequestStatsHourly check",
      AutoTest::stats_diff_checker(
          pq_conn_, diffs, cmp_stats));
  }
  
  const FraudProtectionFeatureTest::TestRequest FRAUD_OVERRIDE[] =
  {
    {
      "FRAUDOVERRIDE/KWD",
      "FRAUDOVERRIDE/TID",
      "unit-test",
      0,
      "FRAUDOVERRIDE/CHANNEL",
      "FRAUDOVERRIDE/CC",
      CLICK_FRAUD_COUNT, TRF_CLICK | TRF_FRAUD
    },
    {
      "FRAUDOVERRIDE/KWD",
      "FRAUDOVERRIDE/TID",
      "unit-test",
      inactivity_period - 1,
      "FRAUDOVERRIDE/CHANNEL",
      "FRAUDOVERRIDE/CC",
      3, TRF_CLICK | TRF_FRAUD
    },
    {
      "FRAUDOVERRIDE/KWD",
      "FRAUDOVERRIDE/TID",
      "unit-test",
      inactivity_period + 1,
      "FRAUDOVERRIDE/CHANNEL",
      "FRAUDOVERRIDE/CC",
      9, TRF_CLICK | TRF_FRAUD
    },
    {
      "FRAUDOVERRIDE/KWD",
      "FRAUDOVERRIDE/TID",
      "unit-test",
      2*inactivity_period,
      "FRAUDOVERRIDE/CHANNEL",
      "FRAUDOVERRIDE/CC",
      9, TRF_CLICK | TRF_FRAUD
    },
    {
      "FRAUDOVERRIDE/KWD",
      "FRAUDOVERRIDE/TID",
      "unit-test",
      fraud_period_2 - 1,
      "FRAUDOVERRIDE/CHANNEL",
      "FRAUDOVERRIDE/CC",
      9, TRF_CLICK | TRF_FRAUD  | TRF_WAIT_FRAUD
    },
    {
      "FRAUDOVERRIDE/KWD",
      "FRAUDOVERRIDE/TID",
      "unit-test",
      fraud_period_2 + inactivity_period - 1,
      "FRAUDOVERRIDE/CHANNEL",
      0,
      1, TRF_FRAUD
    },
    {
      "FRAUDOVERRIDE/KWD",
      "FRAUDOVERRIDE/TID",
      "unit-test",
      fraud_period_2 + inactivity_period + 1,
      "FRAUDOVERRIDE/CHANNEL",
      "FRAUDOVERRIDE/CC",
      1, 0
    }
  };

  AdClient client(AdClient::create_user(this));
  
  process_request(
    client, FRAUD_OVERRIDE, midday);

}

void FraudProtectionFeatureTest::reversed_order_fraud_part_1_(
  AdClient& client)
{
  Generics::Time time(
    (today.get_gm_time().format("%d-%m-%Y") +
      ":" + "10-01-00"), "%d-%m-%Y:%H-%M-%S");

  unsigned long cc = fetch_int("REVERSEDORDER/CC");
  
  ORM::StatsArray<ORM::HourlyStats, 2> stats;
  stats[0].key().
    cc_id(cc).num_shown(1).fraud_correction(false).stimestamp(time);
  stats[1].key().
    cc_id(cc).num_shown(1).fraud_correction(true).stimestamp(time);
  stats.select(pq_conn_);

  // Wait all request processing
  {
    ORM::HourlyStats::Diffs diffs[2] = 
    {
      ORM::HourlyStats::Diffs()
      .imps(14)
      .clicks(0)
      .actions(0)
      .requests(14),
      ORM::HourlyStats::Diffs()
      .imps(0)
      .clicks(0)
      .actions(0)
      .requests(0)
    };

    ADD_WAIT_CHECKER(
      "RequestStatsHourly check",
      AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats));
  }

  const FraudProtectionFeatureTest::TestRequest REVERSED_ORDER[] =
  {
    {
      "REVERSEDORDER/KWD",
      "REVERSEDORDER/TID",
      "unit-test",
      0,
      "REVERSEDORDER/CHANNEL",
      "REVERSEDORDER/CC",
      14,
      TRF_FRAUD | TRF_SKIP_RIA_CHECKS
    }
  };

  process_request(
    client, REVERSED_ORDER, time);
}


void FraudProtectionFeatureTest::reversed_order_fraud_part_2_(
  AdClient& client)
{
  Generics::Time time(
    (today.get_gm_time().format("%d-%m-%Y") +
      ":" + "10-01-00"), "%d-%m-%Y:%H-%M-%S");
  
  unsigned long inactivity_period = fetch_int("UserInactivityPeriod");
  unsigned long fraud_period = 60;
  
  // Fraud request
  client.process_request(
    NSLookupRequest().
      tid(fetch_string("REVERSEDORDER/TID")).
      referer_kw(fetch_string("REVERSEDORDER/KWD")).
      debug_time(time - 30));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("REVERSEDORDER/CC"),
      client.debug_info.ccid).check(),
    "Fraud beginner. CC check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ClientFraudChecker(
        client,
        NSLookupRequest().
          referer_kw(fetch_string("REVERSEDORDER/KWD")).
          debug_time(time - 30)),
      AutoTest::GlobalSettings::instance().
        wait_timeout(),
      10).check());

  client.process_request(
    NSLookupRequest().
      tid(fetch_string("REVERSEDORDER/TID")).
      referer_kw(fetch_string("REVERSEDORDER/KWD")).
      debug_time(
        time + inactivity_period  +
          fraud_period - 30 - 1));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Before fraud expired. CC check");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "true",
      client.debug_info.fraud_request).check(),
    "Before fraud expired. Fraud check")

  client.process_request(
    NSLookupRequest().
      tid(fetch_string("REVERSEDORDER/TID")).
      referer_kw(fetch_string("REVERSEDORDER/KWD")).
      debug_time(
        time + inactivity_period +
          fraud_period - 30 +1));


  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("REVERSEDORDER/CC"),
      client.debug_info.ccid).check(),
    "After fraud expired. CC check");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "false",
      client.debug_info.fraud_request).check(),
    "After fraud expired. Fraud check")
}

void FraudProtectionFeatureTest::delayed_clicks_()
{
  // Yesterday
  AutoTest::Time time;
  time -= 24*60*60;

  StatsHourlyFraudCheck rsh_checker(
    this, pq_conn_, time, "DELAYEDCLICKS/CC",
    FraudStats(
      FE_FRAUD,
      CLICK_FRAUD_COUNT,
      CLICK_FRAUD_COUNT,
      CLICK_FRAUD_COUNT, 0,
      fetch_float("DELAYEDCLICKS/CCCPC"),
      FraudStats::RTE_CPC));

  CmpStatsFraudCheck cmp_checker(
    this, pq_conn_, time, "DELAYEDCLICKS/CHANNEL",
    FraudStats(
      FE_FRAUD,
      CLICK_FRAUD_COUNT,
      CLICK_FRAUD_COUNT,
      CLICK_FRAUD_COUNT, 0,
      fetch_float("DELAYEDCLICKS/CHANNELCPM")));
  
  ADD_WAIT_CHECKER(
    "RequestStatsHourly check",
    rsh_checker);

  ADD_WAIT_CHECKER(
    "CMPStats check",
    cmp_checker);

  const FraudProtectionFeatureTest::TestRequest DELAYED_CLICKS[] =
  {
    {
      "DELAYEDCLICKS/KWD",
      "DELAYEDCLICKS/TID",
      "unit-test",
      0,
      "DELAYEDCLICKS/CHANNEL",
      "DELAYEDCLICKS/CC",
      CLICK_FRAUD_COUNT,
      TRF_CLICK | TRF_FRAUD | TRF_WAIT_FRAUD | TRF_ACTIONS_NOW
    },
    {
      "DELAYEDCLICKS/KWD",
      "DELAYEDCLICKS/TID",
      "unit-test",
      1 + fetch_int("UserInactivityPeriod"),
      "DELAYEDCLICKS/CHANNEL",
      0, 1, TRF_FRAUD
    }
  };
    
  AdClient client(AdClient::create_user(this));
  
  process_request(
    client, DELAYED_CLICKS, time);
}
