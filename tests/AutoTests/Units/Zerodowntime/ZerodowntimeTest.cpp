
#include "ZerodowntimeTest.hpp"

REFLECT_UNIT(ZerodowntimeTest) (
  "Zerodowntime",
  AUTO_TEST_SLOW);

namespace
{
  const char OIX_MANAGER[] = "/opt/foros/manager/bin/oixmgr";
  const std::string EMPTY_PROFILE(".*Profile is empty.*");

  const char* PRESTARTS[] =
  {
    "*:tr1-PreStart",
    "*:tr2-PreStart"
  };

  const char* GROUPS[] =
  {
    "tr1",
    "tr2",
    "fe1",
    "fe2",
    "be",
    "lp",
    0
  };

  const char* COMMANDS[] =
  {
    "start",
    "stop",
    "restart"
  };

  typedef AutoTest::SelectedCreativesChecker SelectedCreativesChecker;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ActionRequest ActionRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  template <class DBEntity>
  class DeleteGuard
  {
  public:
    DeleteGuard(DBEntity* entity) :
      entity_(entity)
    { }

    ~DeleteGuard() noexcept
    {
      entity_->delet();
    }
  private:
    DBEntity* entity_;
  };
}


void ZerodowntimeTest::exec_cluster_command_(
  ClusterGroup group,
  ClusterCommand command)
{
  if (get_global_params().ClusterConfigPath())
  {
    {
      AutoTest::ShellCmd cmd;
      cmd.add_cmd_i(OIX_MANAGER);
      cmd.add_cmd_i("-C");
      cmd.add_cmd_i(get_global_params().ClusterConfigPath()->path());
      if (GROUPS[group])
      {
        cmd.add_cmd_i(GROUPS[group]);
      }
      cmd.add_cmd_i(COMMANDS[command]);

      cmd.log(
        AutoTest::Logger::thlog(),
        Logging::Logger::DEBUG,
        true);
    }

    // Run prestart
    if (command == CC_START && (group == CG_TR_1 || group == CG_TR_2))
    {
      AutoTest::ShellCmd cmd;
      cmd.add_cmd_i(OIX_MANAGER);
      cmd.add_cmd_i("-C");
      cmd.add_cmd_i(get_global_params().ClusterConfigPath()->path());
      cmd.add_cmd_i(PRESTARTS[group]);
      cmd.add_cmd_i(COMMANDS[command]);
      cmd.log(
        AutoTest::Logger::thlog(),
        Logging::Logger::DEBUG,
        true);
    }
  }
}

void
ZerodowntimeTest::set_up()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_global_params().ClusterConfigPath()),
    "Test should have cluster config path");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      campaign_managers_.size(),
      2).check(),
    "The cluster should have 2 CampaignManagers");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      channel_controllers_.size(),
      2).check(),
    "The cluster should have 2 ChannelServerControllers");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      channel_search_servers_.size(),
      2).check(),
    "The cluster should have 2 ChannelSearchServers");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      user_info_managers_.size(),
      2).check(),
    "The cluster should have 2 UserInfoManagers");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      expression_matchers_.size(),
      1).check(),
    "The cluster should have 1 ExpressionMatcher");


  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::CampaignChecker(
        this,
        fetch_int("CCG"),
        AutoTest::CampaignChecker::Expected().
          sites(fetch_string("SITE/1")))).check(),
    "Initial. Check CCG sites");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::TriggerChecker(
        this,
        fetch_string("CHANNEL/CTX"),
        AutoTest::ChannelManagerController,
        AutoTest::TriggerChecker::Expected().
          page_word(
            fetch_string("KWD/CTX/NORM") ))).check(),
    "Initial. Check channel");
}

void
ZerodowntimeTest::init_stats_()
{
  for (int i = 1; i <= 8; i++)
  {
    {
      // Custom report
      ORM::HourlyStats stat;
      stat.key().
        cc_id(fetch_int("CPA/CC/" + strof(i))).
        stimestamp(time_);
      stat.select(pq_conn_);
      request_stats_.push_back(stat);
    }

    {
      // Channel Triggers report
      ORM::ChannelTriggerStats stat;
      stat.key().
        channel_trigger_id(fetch_int("TRIGGER/"  + strof(i))).
        sdate(time_);
      stat.select(pq_conn_);
      trigger_stats_.push_back(stat);
    }

    {
      // Site channel report
      ORM::SiteChannelStats stat;
      stat.key().
        channel_id(fetch_int("CHANNEL/"  + strof(i))).
        sdate(time_);
      stat.select(pq_conn_);
      site_channel_stats_.push_back(stat);
    }

    // ChannelInventoryStats
    {
      ORM::ChannelInventoryStats stat;
      stat.key().
        channel_id(fetch_int("CHANNEL/"  + strof(i))).
        colo_id(1).
        sdate(time_);
      stat.select(pq_conn_);
      channel_inventory_stats_.push_back(stat);
    }

    {
      ORM::ChannelInventoryStats stat;
      stat.key().
        channel_id(fetch_int("CHANNEL/"  + strof(i))).
        colo_id(fetch_int("COLO/CPA")).
        sdate(time_);
      stat.select(pq_conn_);
      zero_channel_inventory_stats_.push_back(stat);
    }

    {
      // ChannelInventoryByCPM
      ORM::ChannelInventoryByCPMStats stat;
      stat.key().
        channel_id(fetch_int("CHANNEL/"  + strof(i))).
        colo_id(fetch_int("COLO/CPA")).
        ecpm(10.0).
        sdate(time_);
      stat.select(pq_conn_);
      channel_by_cpm_.push_back(stat);
    }

    {
      // CCGUserStats
      ORM::CCGUserStats stat;
      stat.key().
        ccg_id(fetch_int("CPA/CCG/"  + strof(i))).
        adv_sdate(time_);
      stat.select(pq_conn_);
      ccg_reach_.push_back(stat);
    }

    {
      // Custom action
      ORM::ActionStats stat;
      stat.key().
        action_id(fetch_int("ACTION/"  + strof(i))).
        cc_id(fetch_int("CPA/CC/"  + strof(i))).
        imp_date(time_).
        click_date(time_);
      stat.select(pq_conn_);
      action_stats_.push_back(stat);
    }

    {
      //  Custom action requests
      ORM::ActionRequests stat;
      stat.key().
        action_id(fetch_int("ACTION/"  + strof(i))).
        action_date(time_);
      stat.select(pq_conn_);
      action_requests_.push_back(stat);
    }

  }

  {
    // ISP report
    isp_stats_.key().
      isp_sdate(time_).
      colo_id(fetch_int("COLO/CPA"));
    isp_stats_.select(pq_conn_);
  }

}

void
ZerodowntimeTest::tear_down()
{
 exec_cluster_command_(CG_ALL, CC_RESTART);
}


bool
ZerodowntimeTest::run()
{
  std::string uid;
  std::ostringstream user_profile;
  std::ostringstream inventory_profile;

  FAIL_CONTEXT(
    precondition_(
      uid,
      user_profile,
      inventory_profile),
    "Precondition");

  AUTOTEST_CASE(
    fe1_stop_(),
    "Stop fe1");

  AUTOTEST_CASE(
    lp_stop_(),
    "Stop lp");

  AUTOTEST_CASE(
    be_stop_(),
    "Stop be");

  AUTOTEST_CASE(
    be_start_(),
    "Start be");

  AUTOTEST_CASE(
    fe1_start_(),
    "Start fe1");

  AUTOTEST_CASE(
    fe2_stop_(),
    "Stop fe2");

  AUTOTEST_CASE(
    lp_start_(
      uid,
      user_profile.str(),
      inventory_profile.str()),
    "Start lp");

  AUTOTEST_CASE(
    fe2_start_(),
    "Start fe2");

  AUTOTEST_CASE(
    check_stats_(),
    "Check statistics");

  return true;
}

void
ZerodowntimeTest::check_click_and_actions_(
  unsigned long index,
  ClusterGroup group)
{
  // Click & action
  AdClient client(
    AdClient::create_user(
      this, group == CG_FE_2? AutoTest::UF_FRONTEND_MINOR: 0));

  AutoTest::ConsequenceActionList actions;
  actions.push_back(
    AutoTest::ConsequenceAction(
      AutoTest::CLICK, time_));
  actions.push_back(
    AutoTest::ConsequenceAction(
      AutoTest::ACTION, time_));

  std::list<std::string> expected_ccs;
  expected_ccs.push_back(
    fetch_string("CPA/CC/" + strof(index)));

  FAIL_CONTEXT(
    client.do_ad_requests(
      NSLookupRequest().
        debug_time(time_).
        colo(fetch_int("COLO/CPA")).
        referer(fetch_string("URL/" + strof(index))).
        referer_kw(fetch_string("KEYWORD/" + strof(index))).
        tid(fetch_int("TAG/CPA")),
      expected_ccs, actions),
    "Click & actions check");

  client.process_request(
    ActionRequest().
      debug_time(time_).
      actionid(fetch_int("ACTION/" + strof(index))));
}

void
ZerodowntimeTest::check_balancing_(
  ClusterGroup group,
  const char* cc_name)
{
  AdClient client(
    AdClient::create_user(
      this, group == CG_FE_2? AutoTest::UF_FRONTEND_MINOR: 0));

  FAIL_CONTEXT(
    SelectedCreativesChecker(
        this,
        client,
        NSLookupRequest().
          debug_time(time_).
          referer_kw(fetch_string("KWD/CTX")).
          tid(fetch_int("TAG/1")), cc_name).check(),
      "Check creative");
}

void
ZerodowntimeTest::check_user_profiling_(
  ClusterGroup group,
  const char* cc_name)
{
  AdClient client(
    AdClient::create_user(
      this, group == CG_FE_2? AutoTest::UF_FRONTEND_MINOR: 0));

    FAIL_CONTEXT(
      SelectedCreativesChecker(
        this,
        client,
        NSLookupRequest().
          debug_time(time_).
          referer_kw(fetch_string("KWD/NON_CTX")).
          tid(fetch_int("TAG/1")), 0).check(),
      "Check creative non context#1");

    FAIL_CONTEXT(
      SelectedCreativesChecker(
        this,
        client,
        NSLookupRequest().
          debug_time(time_).
          referer_kw(fetch_string("KWD/NON_CTX")).
          tid(fetch_int("TAG/1")),
        cc_name).check(),
      "Check creative non context#1");
}

void
ZerodowntimeTest::check_channels_(
  ClusterGroup group,
  const char* cc_name,
  bool no_db)
{
  AdClient client(
    AdClient::create_nonoptin_user(
      this, group == CG_FE_2? AutoTest::UF_FRONTEND_MINOR: 0));

  FAIL_CONTEXT(
    AutoTest::admin_checker(
      AutoTest::TriggerAdmin(
        channel_controllers_[group == CG_FE_2? 1: 0].address,
        fetch_string("CHANNEL/CTX"),
        AutoTest::ChannelManagerController),
      no_db?
      AutoTest::TriggerAdmin::Expected().
        page_word(
          fetch_string("KWD/CTX/NORM") ):
        AutoTest::TriggerAdmin::Expected().
          page_word(
            AutoTest::ComparableStringList(
             { fetch_string("KWD/CTX/NORM"),
               fetch_string("KWD/CTX_NEW/NORM") } ))).check(),
    "Check trigger loaded");

  client.process_request(
    NSLookupRequest().
      debug_time(time_).
      referer_kw(fetch_string("KWD/CTX_NEW/NORM")).
      tid(fetch_int("TAG/1")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "BP/CTX",
      client.debug_info.trigger_channels,
      no_db? AutoTest::SCE_NOT_ENTRY: AutoTest::SCE_ENTRY).check(),
    "Trigger channels");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      no_db? "0": fetch_string(cc_name).c_str(),
      client.debug_info.ccid).check(),
    "Check creative");
}

ZerodowntimeTest::CCGSite*
ZerodowntimeTest::campaign_update_(
  ClusterGroup group,
  const char* ccg_name,
  const char* cc_name,
  bool no_db)
{
  AdClient client(
    AdClient::create_nonoptin_user(
      this, group == CG_FE_2? AutoTest::UF_FRONTEND_MINOR: 0));

  CCGSite* ccgsite =
    create<AutoTest::ORM::PQ::CCGSite>();

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccgsite->insert(
        fetch_int(ccg_name),
        fetch_int("SITE/2"))),
    "insert CCGSite link");

  if (!no_db)
  {
    ADD_WAIT_CHECKER(
      "Check CCG sites",
      AutoTest::and_checker(
        AutoTest::admin_checker(
          AutoTest::CampaignAdmin(
            campaign_managers_[group == CG_FE_2? 1: 0].address,
            fetch_int(ccg_name)),
          AutoTest::CampaignAdmin::Expected().
            sites(
              AutoTest::ComparableStringList(
                { fetch_string("SITE/1"),
                  fetch_string("SITE/2")}) )),
        SelectedCreativesChecker(
          this,
          client,
          NSLookupRequest().
            debug_time(time_).
            referer_kw(fetch_string("KWD/CTX")).
            tid(fetch_int("TAG/2")),
          cc_name)));
  }

  return ccgsite;
}

void
ZerodowntimeTest::check_channel_search_(
  ClusterGroup group)
{
  FAIL_CONTEXT(
      AutoTest::admin_checker(
        AutoTest::ChannelSearchAdmin(
          channel_search_servers_[group == CG_FE_2? 1: 0].address,
          fetch_string("KWD/CTX"),
          AutoTest::ChannelSearch),
        AutoTest::ChannelSearchAdmin::Expected().
          channel_id(fetch_string("CHANNEL/CTX"))).check(),
      "ChannelSearchAdmin check");
}

void
ZerodowntimeTest::precondition_(
  std::string& uid,
  std::ostream& user_profile,
  std::ostream& inventory_profile)
{
  init_stats_();

  AdClient client(
    AdClient::create_user(this));

  for (int i = 1; i <= 2; ++i)
  {
    client.process_request(
      NSLookupRequest().
        referer(fetch_string("URL/PRE/" + strof(i))).
        tid(fetch_int("TAG/PRE")).
      debug_time(i == 1? time_ - 24*60*60: time_));

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        ("BP/PRE/" + strof(i)).c_str(),
        client.debug_info.trigger_channels).check(),
      "Trigger channels");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("CC/PRE/" + strof(i)),
        client.debug_info.ccid).check(),
      "Check creative");
  }

  uid = client.debug_info.uid.value();

  AutoTest::InventoryProfileAdmin inventory_adm(
    expression_matchers_[0].address,
    uid);

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::admin_checker(
        inventory_adm,
        AutoTest::InventoryProfileAdmin::Expected().
          imp_count(2) )).check(),
      "Check inventory profile");

  inventory_adm.dump_native_out(inventory_profile);

  FAIL_CONTEXT(
    AutoTest::BaseProfileEmptyChecker(
      this,
      uid,
      false,
      AutoTest::UserInfoManager,
      EMPTY_PROFILE,
      AutoTest::AEC_NOT_EXISTS),
    "Check base profile");

  // Store first non empty profile
  for (int i = 0; i < 2; ++i)
  {
    AutoTest::BaseProfileAdmin admin(
      user_info_managers_[i].address,
      uid, false,
      AutoTest::UserInfoManager);

    admin.log(AutoTest::Logger::thlog());

    if (
      !AutoTest::admin_checker(
        admin,
        EMPTY_PROFILE).check(false) )
    {
      admin.dump_native_out(user_profile);
      break;
    }
  }

  {
    AutoTest::AdminsArray<AutoTest::InventoryProfileAdmin> admins;

    admins.initialize(
      this,
      CTE_ALL,
      STE_EXPRESSION_MATCHER,
      uid);

    admins.log(AutoTest::Logger::thlog());
  }

}


void
ZerodowntimeTest::fe1_stop_()
{
  exec_cluster_command_(CG_FE_1, CC_STOP);
  exec_cluster_command_(CG_TR_1, CC_STOP);

  // Second frontend works
  check_balancing_(CG_FE_2, "CC");

  // Campaign update
  campaign_update_(CG_FE_2, "CCG", "CC", no_db_);


  // Check channels loaded
  {
    // Channel update
    AutoTest::ORM::ORMRestorer<AutoTest::ORM::TriggerChannel>* trigger =
      create<AutoTest::ORM::TriggerChannel>(fetch_int("CHANNELTRIGGER/CTX_NEW"));

    trigger->qa_status = "A";
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        trigger->update(false)),
      "activate trigger");

    if (!no_db_)
    {
      ADD_WAIT_CHECKER(
        "Check trigger loaded",
        AutoTest::admin_checker(
          AutoTest::TriggerAdmin(
            channel_controllers_[1].address,
            fetch_string("CHANNEL/CTX"),
            AutoTest::ChannelManagerController),
          AutoTest::TriggerAdmin::Expected().
            page_word(
              AutoTest::ComparableStringList(
              { fetch_string("KWD/CTX/NORM"),
                fetch_string("KWD/CTX_NEW/NORM") } ))));
    }
  }

  // User profiling work
  check_user_profiling_(CG_FE_2, "CC");

  // Channel Search work
  check_channel_search_(CG_FE_2);

  // Click & action
  check_click_and_actions_(1, CG_FE_2);

  check();
}

void
ZerodowntimeTest::lp_stop_()
{
  exec_cluster_command_(CG_LP, CC_STOP);

  // Click & action
  check_click_and_actions_(2, CG_FE_2);
}

void
ZerodowntimeTest::be_stop_()
{
  exec_cluster_command_(CG_BE, CC_STOP);

  // Second frontend works
  check_balancing_(CG_FE_2, "CC");

  // Check old channels loaded
  if (!no_db_)
  {
    ADD_WAIT_CHECKER(
      "Check trigger loaded",
      AutoTest::admin_checker(
        AutoTest::TriggerAdmin(
          channel_controllers_[1].address,
          fetch_string("CHANNEL/CTX"),
          AutoTest::ChannelManagerController),
        AutoTest::TriggerAdmin::Expected().
          page_word(
            AutoTest::ComparableStringList(
              { fetch_string("KWD/CTX/NORM"),
                fetch_string("KWD/CTX_NEW/NORM") } ))));
  }

  // User profiling work
  check_user_profiling_(CG_FE_2, "CC");

  // Click & action
  check_click_and_actions_(3, CG_FE_2);

  check();
}

void
ZerodowntimeTest::be_start_()
{
  {
    AdClient client(
      AdClient::create_nonoptin_user(
        this, AutoTest::UF_FRONTEND_MINOR));

    AutoTest::ORM::ORMRestorer<AutoTest::ORM::PQ::Campaign>* campaign =
      create<AutoTest::ORM::PQ::Campaign>(
        fetch_int("CAMPAIGN/D"));

    campaign->status = "A";

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        campaign->update(false)),
      "activate campaign");

    ADD_WAIT_CHECKER(
      "Check CCG loaded",
      AutoTest::and_checker(
        AutoTest::admin_checker(
          AutoTest::CampaignAdmin(
            campaign_managers_[1].address,
            fetch_int("CCG/D")),
          AutoTest::CampaignAdmin::Expected().
            sites( fetch_string("SITE/1") ) ),
        SelectedCreativesChecker(
          this,
          client,
          NSLookupRequest().
            debug_time(time_).
            referer_kw(fetch_string("KWD/CTX")).
            tid(fetch_int("TAG/1")),
          "CC/D")));
  }

  exec_cluster_command_(CG_BE, CC_START);

  check();

  // Check channels loaded
  check_channels_(CG_FE_2, "CC/D", no_db_);

  // User profiling work
  check_user_profiling_(CG_FE_2, "CC/D");

  // Click & action
  check_click_and_actions_(4, CG_FE_2);
}

void
ZerodowntimeTest::fe1_start_()
{
    exec_cluster_command_(CG_FE_1, CC_START);
    exec_cluster_command_(CG_TR_1, CC_START);

    // First frontend works
    check_balancing_(CG_FE_1, "CC/D");

    // User profiling work
    check_user_profiling_(CG_FE_1, "CC/D");

    // Channel Search work
    check_channel_search_(CG_FE_1);
    check_channel_search_(CG_FE_2);

    // Campaign update
    DeleteGuard<CCGSite> guard(
      campaign_update_(CG_FE_1, "CCG/D", "CC/D"));

    // Check channels loaded
    check_channels_(CG_FE_1, "CC/D");
    check_channels_(CG_FE_2, "CC/D", no_db_);

    // Click & action
    check_click_and_actions_(5, CG_FE_1);

    check();
}

void
ZerodowntimeTest::fe2_stop_()
{
  exec_cluster_command_(CG_FE_2, CC_STOP);
  exec_cluster_command_(CG_TR_2, CC_STOP);

  // First frontend works
  check_balancing_(CG_FE_1, "CC/D");

  // User profiling work
  check_user_profiling_(CG_FE_1, "CC/D");

  // Channel Search work
  check_channel_search_(CG_FE_1);

  // Click & action
  check_click_and_actions_(6, CG_FE_1);
}

void
ZerodowntimeTest::lp_start_(
  const std::string& uid,
  const std::string& user_profile,
  const std::string& inventory_profile)
{
  exec_cluster_command_(CG_LP, CC_START);

  // Click & action
  check_click_and_actions_(7, CG_FE_1);

  // Old user profiles consistency

  FAIL_CONTEXT(
    AutoTest::admin_checker(
      AutoTest::InventoryProfileAdmin(
        expression_matchers_[0].address,
        uid),
      inventory_profile).check(),
    "Check inventory profile");

  FAIL_CONTEXT(
    AutoTest::admin_checker(
      AutoTest::BaseProfileAdmin(
        user_info_managers_[0].address,
        uid,
        false,
        AutoTest::UserInfoManager),
      user_profile).check(),
    "Check inventory profile");
}

void
ZerodowntimeTest::fe2_start_()
{
  exec_cluster_command_(CG_FE_2, CC_START);
  exec_cluster_command_(CG_TR_2, CC_START);

  // First frontend works
  check_balancing_(CG_FE_1, "CC/D");
  // Second frontend works
  check_balancing_(CG_FE_2, "CC/D");

  // User profiling work
  check_user_profiling_(CG_FE_2, "CC/D");

  // Channel Search work
  check_channel_search_(CG_FE_1);
  check_channel_search_(CG_FE_2);

  // Campaign update
  campaign_update_(CG_FE_2, "CCG/D", "CC/D");

  // Check channels loaded
  check_channels_(CG_FE_1, "CC/D");
  check_channels_(CG_FE_2, "CC/D");

  // Click & action
  check_click_and_actions_(8, CG_FE_2);

  check();
}

void
ZerodowntimeTest::check_stats_()
{
  ADD_WAIT_CHECKER(
    "Custom report",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::HourlyStats::Diffs().
        imps(1).
        clicks(1).
        actions(1).
        adv_amount(1).
        // isp_amount = adv_amount * revenue_share * currency_rate
        isp_amount( 1.0 * 0.5 * 0.51) ,
      request_stats_));

  ADD_WAIT_CHECKER(
    "ISP stats",
    AutoTest::stats_diff_checker(
      pq_conn_,
      ORM::ColoUserStats::Diffs().
        unique_users(8),
      isp_stats_));

  ADD_WAIT_CHECKER(
    "ChannelTriggerStats",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ChannelTriggerStats::Diffs().hits(1),
      trigger_stats_));

  ADD_WAIT_CHECKER(
    "SiteChannelStats",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::SiteChannelStats::Diffs().imps(1),
      site_channel_stats_));

  ADD_WAIT_CHECKER(
    "ChannelInventoryStats",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ChannelInventoryStats::Diffs().
        active_users(1).
        sum_ecpm(0),
      channel_inventory_stats_));

  ADD_WAIT_CHECKER(
    "ChannelInventoryStats zero",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ChannelInventoryStats::Diffs().
        active_users(0).
        sum_ecpm(0),
      zero_channel_inventory_stats_));

  ADD_WAIT_CHECKER(
    "ChannelInventoryByCPM",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ChannelInventoryByCPMStats::Diffs().user_count(1),
      channel_by_cpm_));

  ADD_WAIT_CHECKER(
    "CCGUserStats",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::CCGUserStats::Diffs().unique_users(1),
      ccg_reach_));

  ADD_WAIT_CHECKER(
    "Custom action",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      action_stats_));

  ADD_WAIT_CHECKER(
    "Custom action",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ORM::ActionRequests::Diffs().count(1),
      action_requests_));

}

