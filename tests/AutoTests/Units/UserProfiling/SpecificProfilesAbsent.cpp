
#include "SpecificProfilesAbsent.hpp"

REFLECT_UNIT(SpecificProfilesAbsent) (
  "UserProfiling",
  AUTO_TEST_SLOW);

namespace 
{

  enum CorbaRefOptEnum
  {
    CROE_Controller,
    CROE_Manager,
    CROE_Ref
  };

  enum UserIdOptEnum
  {
    UIOE_User,
    UIOE_Temp,
    UIOE_None
  };

  const String::SubString PROFILE_EMPTY_STRING(
    "[Pp]rofile is empty.|[Pp]rofile not found.");
  class ProfileAdminCmd : public AutoTest::ShellCmd
  {

  protected:

    std::string admin_;
    std::string command_;
    std::string service_;
    /**
     * @brief Makes admin call string.
     * @param admin - admin to use
     * @param command - command to ask
     * @param service - service corba object name
     * @param UUID. 
     * @param address manager object address.
     * @param controller - corba reference option switch
     * @param user - uid option switch
     */
    void make_cmd (const char* admin,
      const char* command,
      const char* service,
      const char* uuid,
      const char* address,
      CorbaRefOptEnum controller = CROE_Controller,
      UserIdOptEnum user = UIOE_User);

  public:

    /**
     * @brief Creates AdminCmd object and initialize it.
     * @param admin - admin to use
     * @param command - command to ask
     * @param service - service corba object name
     * @param UUID. 
     * @param address manager object address.
     * @param controller - corba reference option switch
     * @param user - uid option switch
     */
    ProfileAdminCmd(const char* admin,
      const char* command,
      const char* service,
      const char* uuid,
      const char* address,
      CorbaRefOptEnum controller = CROE_Controller,
      UserIdOptEnum user = UIOE_User);

    class Examiner;
    /**
     * @brief Do profiles check
     * @return true if empty
     */
    bool check();
    bool check(ProfileAdminCmd::Examiner& examiner);

    /*
     * @brief Internal checking of command output
     */
    class Examiner:
      public AutoTest::ShellCmd::Examiner
    {
    protected:

      ProfileAdminCmd* admin_;
      String::RegEx    re_;
    public:
      Examiner (ProfileAdminCmd& admin)
        : admin_(&admin), re_(PROFILE_EMPTY_STRING)
      {};

      Examiner ()
        : admin_(0), re_(PROFILE_EMPTY_STRING)
      {};

      bool examine (std::istream& in);
    };
  };

  namespace ORM = ::AutoTest::ORM;

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ActionRequest ActionRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef ORM::HourlyStats HourlyStats;
}

bool
SpecificProfilesAbsent::run_test()
{
  NOSTOP_FAIL_CONTEXT(case_for_non_opted_in_users());
  return true;
}

void SpecificProfilesAbsent::case_for_non_opted_in_users()
{
  add_descr_phrase("Scenario#1. Empty profile for non opted in users");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(
        CTE_ALL,
        STE_USER_INFO_MANAGER)),
    "this test requires UserInfoManager in autotest config");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(
        CTE_ALL,
        STE_REQUEST_INFO_MANAGER)),
    "this test requires RequestInfoManager in autotest config");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(
        CTE_ALL,
        STE_EXPRESSION_MATCHER)),
    "this test requires ExpressionMatcher in autotest config");

  GlobalConfig::Service service
    = get_config().get_service(CTE_ALL, STE_FRONTEND);
  
  std::string uid = 
    service.remote ? "AAAAAAAAAAAAAAAAAAAAAA.." 
    : AutoTest::PROBE_UID;

  
  add_descr_phrase("Scenario#1. Prepare");

  Generics::Time target_request_time = Generics::Time::get_time_of_day();
  target_request_time = target_request_time.get_gm_time().get_date() +
    Generics::Time::ONE_HOUR * target_request_time.get_gm_time().tm_hour;
  Generics::Time target_sdate = target_request_time;

  add_descr_phrase("Scenario#1. Prepare stats");

  AutoTest::DBC::Conn pq_conn(open_pq());
  
  ORM::StatsArray<HourlyStats, 2> stats_hs;
  stats_hs[0].key(
    HourlyStats::Key().
      cc_id(fetch_int("CCId1")).
      num_shown(1).
      stimestamp(target_sdate));
  stats_hs[1].key(
    HourlyStats::Key().
      cc_id(fetch_int("CCId2")).
      num_shown(1).
      stimestamp(target_sdate));
  stats_hs.select(pq_conn);

  unsigned int default_colo = fetch_int("DefaultColo");
  unsigned int chan1 = fetch_int("Channel1");
  unsigned int chan2 = fetch_int("Channel2");

  ORM::StatsArray<ORM::ChannelImpInventory, 2> stats_cii;
  stats_cii[0].key(
    ORM::ChannelImpInventory::Key().
      channel_id(chan1).
      ccg_type("D").
      colo_id(default_colo));
  stats_cii[1].key(
    ORM::ChannelImpInventory::Key().
      channel_id(chan2).
      ccg_type("D").
      colo_id(default_colo));
  stats_cii.select(pq_conn);

  add_descr_phrase("Scenario#1. do requests");

  AdClient client(AdClient::create_nonoptin_user(this));
  client.change_base_url(service.address.c_str());

  AutoTest::NSLookupRequest request;
  request.format("unit-test");
  request.tid(fetch_string("TagId"));
  request.debug_time(target_request_time);

  request.referer(fetch_string("Keyword1"));
  client.process_request(request, "Ad request1");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("Channel1") + "U", 
      client.debug_info.trigger_channels).check(),
    "must match first channel");

  request.referer(fetch_string("Keyword2"));
  client.process_request(request, "Ad request2");

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("Channel2") + "U", 
      client.debug_info.trigger_channels).check(),
    "must match second channel");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CCId2"), 
      client.debug_info.ccid).check(),
    "must match second channel");

  client.process_request(ActionRequest()
    .cid(fetch_string("CCGId2"))
    .actionid(fetch_string("Action")),
    "action request");

  add_descr_phrase("Scenario#1. Check stats");

  const HourlyStats::Diffs diffs_hs[] =
  {
    HourlyStats::Diffs().
      imps(0).
      requests(0),
    HourlyStats::Diffs().
      imps(1).
      requests(1)
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn, diffs_hs, stats_hs)).check(),
    "must got expected diffs for HourlyStats");

  const ORM::ChannelImpInventory::Diffs diffs_cii[] =
  {
    ORM::ChannelImpInventory::Diffs().
      imps_other(0).
      impops_no_imp(1),
    ORM::ChannelImpInventory::Diffs().
      imps_other(1).
      impops_no_imp(0),    
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn, diffs_cii, stats_cii)).check(),
    "must got expected diffs for ChannelImpInventory");

  add_descr_phrase("Scenario#1. Check admins");

  ProfileAdminCmd::Examiner examiner;

  const GlobalConfig::ServiceList& uim_services = get_config().get_services(
    CTE_ALL, STE_USER_INFO_MANAGER);
  for (unsigned int i = 0; i < uim_services.size(); ++i)
  {
    std::string uim_manager = uim_services[i].address;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "UserInfoAdmin", "print-base",
          "UserInfoManager", uid.c_str(),
          uim_manager.c_str(), CROE_Manager).check(examiner)),
      "profile UserInfoAdmin print-base not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "UserInfoAdmin", "print-add",
          "UserInfoManager", uid.c_str(),
          uim_manager.c_str(), CROE_Manager).check(examiner)),
      "profile UserInfoAdmin print-add not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "UserInfoAdmin", "print-history",
          "UserInfoManager", uid.c_str(),
          uim_manager.c_str(), CROE_Manager).check(examiner)),
      "profile UserInfoAdmin print-history not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "UserInfoAdmin", "print-wd",
          "UserInfoManager", uid.c_str(),
          uim_manager.c_str(), CROE_Manager).check(examiner)),
      "profile UserInfoAdmin print-wd not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "UserInfoAdmin", "print-freq-cap",
          "UserInfoManager", uid.c_str(),
          uim_manager.c_str(),CROE_Manager).check(examiner)),
      "profile UserInfoAdmin print-freq-cap not empty ");
  }

  const GlobalConfig::ServiceList& rim_services = get_config().get_services(
    CTE_ALL, STE_REQUEST_INFO_MANAGER);
  for (unsigned int i = 0; i < rim_services.size(); ++i)
  {
    std::string rim_manager = rim_services[i].address;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "RequestInfoAdmin", "print-reach",
          "RequestInfoManager", uid.c_str(),
          rim_manager.c_str(), CROE_Ref).check(examiner)),
      "profile RequestInfoAdmin print-reach not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "RequestInfoAdmin", "print-action",
          "RequestInfoManager", uid.c_str(),
          rim_manager.c_str(),CROE_Ref).check(examiner)),
      "profile RequestInfoAdmin print-action not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "RequestInfoAdmin", "print-fraud",
          "RequestInfoManager", uid.c_str(),
          rim_manager.c_str(), CROE_Ref).check(examiner)),
      "profile RequestInfoAdmin print-fraud not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "RequestInfoAdmin", "print-site-reach",
          "RequestInfoManager", uid.c_str(),
          rim_manager.c_str(), CROE_Ref).check(examiner)),
      "profile RequestInfoAdmin print-site-reach not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "RequestInfoAdmin", "print-tag-request-groups",
          "RequestInfoManager", uid.c_str(),
          rim_manager.c_str(), CROE_Ref).check(examiner)),
      "profile RequestInfoAdmin print-tag-request-groups not empty ");
  }

  const GlobalConfig::ServiceList& em_services = get_config().get_services(
    CTE_ALL, STE_EXPRESSION_MATCHER);
  for (unsigned int i = 0; i < em_services.size(); ++i)
  {
    std::string em_manager = em_services[i].address;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "ExpressionMatcherAdmin", "print",
          "ExpressionMatcher", uid.c_str(), em_manager.c_str(),
          CROE_Ref, UIOE_None).check(examiner)),
      "ExpressionMatcherAdmin print not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "ExpressionMatcherAdmin", "print-est",
          "ExpressionMatcher", uid.c_str(), em_manager.c_str(),
          CROE_Ref, UIOE_None).check(examiner)),
      "ExpressionMatcherAdmin print-est not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "ExpressionMatcherAdmin", "print-user-trigger-match",
          "ExpressionMatcher", uid.c_str(),
          em_manager.c_str(), CROE_Ref, UIOE_None).check(examiner)),
      "ExpressionMatcherAdmin print-user-trigger-match not empty ");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ProfileAdminCmd(
          "ExpressionMatcherAdmin", "print-request-trigger-match",
          "ExpressionMatcher", uid.c_str(),
          em_manager.c_str(), CROE_Ref, UIOE_None).check(examiner)),
      "ExpressionMatcherAdmin print-request-trigger-match not empty ");
  }
}

//////////////////// implementation ///////////////
namespace 
{
  void
  ProfileAdminCmd::make_cmd (const char* admin,
     const char* command,
     const char* service,
     const char* uuid,
     const char* address,
     CorbaRefOptEnum controller,
     UserIdOptEnum user)
  {
    clear();
    add_cmd_i(admin);
    add_cmd_i(command);
    switch (user)
    {
    case UIOE_User : 
      {
        add_cmd_i(std::string("--uid=") + uuid);
        break;
      }
    case UIOE_Temp : 
      {
        add_cmd_i(std::string("--tuid=") + uuid);
        break;
      }
    case UIOE_None : 
      {
        add_cmd_i(uuid);
        break;
      }
    }
    switch (controller)
    {
    case CROE_Controller : 
      {
        add_cmd_i(std::string("-r orbaloc::") 
                  +  address + "/" + service);
        break;
      }
    case CROE_Manager :
      {
        add_cmd_i(std::string("-r corbaloc::") 
                  +  address + "/" + service);
        break;
      }
    case CROE_Ref :
      {
        add_cmd_i(std::string("-r corbaloc::") 
                  +  address + "/" + service);
        break;
      }
    };
  }
  ProfileAdminCmd::ProfileAdminCmd(const char* admin,
    const char* command,
    const char* service,
    const char* uuid,
    const char* address,
    CorbaRefOptEnum controller,
    UserIdOptEnum user)
  {
    admin_ = admin;
    command_ = command;
    service_ = service;
    make_cmd(admin, command, service, uuid, address, controller, user);
  }

  bool ProfileAdminCmd::check(ProfileAdminCmd::Examiner& examiner)
  {
    bool ret = ShellCmd::fetch(examiner);
    ShellCmd::log(AutoTest::Logger::thlog(), Logging::Logger::INFO);
    return ret;
  }

  bool ProfileAdminCmd::check()
  {
    Examiner examiner(*this);
    return check(examiner);
  }


  bool
  ProfileAdminCmd::Examiner::examine(std::istream& in)
  {
    bool ret = true;
    std::string line;
    while (!in.eof())
    {
      std::getline(in, line);
      if (!line.empty())
      {
        if (re_.match(line))
        {
          ret = true;
          break;
        }
        ret = false;
      }
    }
    return ret;
  };
}
