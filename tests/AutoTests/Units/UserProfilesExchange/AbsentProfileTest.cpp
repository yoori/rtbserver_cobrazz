
#include "AbsentProfileTest.hpp"
 
REFLECT_UNIT(AbsentProfileTest) (
  "UserProfilesExchange",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::AdditionalProfileChecker AdditionalProfileChecker;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  // 15 minutes need UserInfoExchanger to mark profile as absent
  static const unsigned long MAX_WAIT_TIME = 900;
  
  class WaitAdditionalProfileEmpty: public AutoTest::Checker
  {
  public:
    WaitAdditionalProfileEmpty(
        BaseUnit* test,
        AutoTest::AdClient& client,
        const std::string& expected_channels)
      : test_(test),
        client_(client),
        expected_channels_(expected_channels)
    { }

    virtual ~WaitAdditionalProfileEmpty() noexcept
    { }

    bool check(bool throw_error = true)
      /*throw(eh::Exception)*/
    {
      std::string uid = AutoTest::prepare_uid(
        client_.get_uid(),
        AutoTest::UUE_ADMIN_PARAMVALUE);

      bool result =
        AdditionalProfileChecker(
          test_,
          uid,
          false,
          AutoTest::UserInfoManagerController,
          AdditionalProfileChecker::Expected(),
          AutoTest::AEC_NOT_EXISTS).check(throw_error);

      client_.process_request(
        NSLookupRequest().
        debug_time(AutoTest::Time()));

      ChannelsCheck(
        test_,
        expected_channels_.c_str(),
        client_.debug_info.history_channels).check(true);
      
      return result;
    }

  private:
    BaseUnit* test_;
    AutoTest::AdClient& client_;
    std::string expected_channels_;
  };
}

bool 
AbsentProfileTest::run_test()
{

  AdClient client(AdClient::create_undef_user(this));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(
        CTE_ALL, STE_USER_INFO_MANAGER_CONTROLLER)),
    "UserInfoManagerController required in config");

  std::string today_history = "Channel/S1,Channel/S2,Channel/HT1,Channel/HT2";
  std::string tommorow_history = "Channel/H1,Channel/H2";

  NSLookupRequest request;

  add_descr_phrase("Send request with \"unknown\" uid");
  
  request.referer_kw = fetch_string("Keyword1");
  AutoTest::Time today;
  request.debug_time = today;
  client.process_request(request);

  std::string colo_id =  client.debug_info.colo_id;
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("AbsentColo"),
      colo_id,
      AutoTest::CT_NOT_EQUAL).check(),
    "Absent colo should not appear in debug_info.colo_id");

  add_descr_phrase("Send request with \"unknown\" uid and lc=<unknown colo>");

  client.set_cookie_value("lc", fetch_string("AbsentColo").c_str(), false);

  request.referer_kw =
    fetch_string("Keyword1") + "," + fetch_string("Keyword2");
  request.debug_time = today + 1;
  client.process_request(request);

  AutoTest::AdminsArray<AutoTest::UserInfoAdminLog> admin;
  
  admin.initialize(
    this, CTE_ALL,
    STE_USER_INFO_MANAGER_CONTROLLER,
    AutoTest::prepare_uid(
      client.get_uid(),
      AutoTest::UUE_ADMIN_PARAMVALUE).c_str(),
    AutoTest::UserInfoManagerController);
  
  admin.log(AutoTest::Logger::thlog());

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      today_history.c_str(),
      client.debug_info.history_channels).check(),
    "History check tomorrow");

  std::string lc_cookie;
  client.get_cookies().find_value("lc", lc_cookie);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo_id,
      lc_cookie).check(),
    "lc cookie must be changed");

  {
    add_descr_phrase("Wait NOT empty additional profile");
    
    std::string uid = AutoTest::prepare_uid(
      client.get_uid(),
      AutoTest::UUE_ADMIN_PARAMVALUE);

    std::string time2_st = (today+1).get_gm_time().format("%Y-%m-%d %H:%M:%S");
    
    std::string page_session_matches =
      "\\[ channel_id = " + fetch_string("Channel/S1") + ", timestamps = " +
      time2_st + " \\] "
      "\\[ channel_id = " + fetch_string("Channel/S2") + ", timestamps = " +
      time2_st + " \\]";
    
    std::string page_ht_candidates =
      "\\[ channel_id = "  + fetch_string("Channel/HT1") + ", req_visits = 1, "
      "visits = 1, weight = 0 \\] "
      "\\[ channel_id = "  + fetch_string("Channel/HT2") + ", req_visits = 0, "
      "visits = 1, weight = 1 \\]";
    
    std::string page_history_visits =
      "\\[ channel_id = "  + fetch_string("Channel/HT1") + ", visits = 1 \\] "
      "\\[ channel_id = "  + fetch_string("Channel/HT2") + ", visits = 1 \\] "
      "\\[ channel_id = "  + fetch_string("Channel/H1") + ", visits = 1 \\] "
      "\\[ channel_id = "  + fetch_string("Channel/H2") + ", visits = 1 \\]";

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AdditionalProfileChecker(
          this,
          uid,
          false,
          AutoTest::UserInfoManagerController,
          AdditionalProfileChecker::Expected().
            page_ht_candidates(page_ht_candidates).
            page_history_visits(page_history_visits).
            page_session_matches(page_session_matches))).check(),
      "Check url_session_matches");
  }

  add_descr_phrase("Wait empty additional profile");

  try
  {
    FAIL_CONTEXT(
      AutoTest::wait_checker(
        WaitAdditionalProfileEmpty(
          this,
          client,
          today_history),
        MAX_WAIT_TIME).check(),
      "Wait empty additional profile");
  }
  catch (const eh::Exception&)
  {
    admin.log(AutoTest::Logger::thlog());
    throw;
  }

  client.process_request(
    NSLookupRequest().
      debug_time(AutoTest::Time() + 24*60*60));
    
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      tommorow_history.c_str(),
      client.debug_info.history_channels).check(),
    "History check tomorrow");

  return true;
}

