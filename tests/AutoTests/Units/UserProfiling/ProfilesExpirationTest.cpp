
#include "ProfilesExpirationTest.hpp"
 
REFLECT_UNIT(ProfilesExpirationTest) (
  "UserProfiling",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::NSLookupRequest NSLRequest;
  typedef AutoTest::DeleteOldUserProfiles DelAdmin;
  typedef AutoTest::BaseProfileChecker BaseProfileChecker;
  typedef AutoTest::FreqCapProfileChecker FreqCapProfileChecker;
  typedef AutoTest::AdditionalProfileChecker AdditionalProfileChecker;
  typedef AutoTest::HistoryProfileChecker HistoryProfileChecker;

  static const Generics::Time TIME_SHIFT = Generics::Time::ONE_MINUTE; // 1 min
};

void ProfilesExpirationTest::check_profiles_exist(
  const std::string& uid,
  bool exists,
  bool temp_user)
{
  std::string dsc1 =
    std::string("UIM must ") + (exists ? "" : "not ") + "contain ";
  std::string dsc2 = std::string("profile for user '") + uid + "'";

  AutoTest::AdminExistCheck check_empty = 
    exists? AutoTest::AEC_NOT_EXISTS: AutoTest::AEC_EXISTS;

  FAIL_CONTEXT(
    AutoTest::BaseProfileEmptyChecker(
      this, uid, temp_user,
      AutoTest::UserInfoManagerController,
      "Profile is empty.",
      check_empty).check(),
    dsc1 + "base " + dsc2);

  FAIL_CONTEXT(
    AutoTest::HistoryProfileEmptyChecker(
      this, uid, temp_user,
      AutoTest::UserInfoManagerController,
      "History profile is empty.",
      check_empty).check(),
    dsc1 + "history " + dsc2);


  if (remote_case_ && !temp_user)
  {
    FAIL_CONTEXT(
      AutoTest::AdditionalProfileEmptyChecker(
        this, uid, temp_user,
        AutoTest::UserInfoManagerController,
        "Profile is empty.",
        check_empty).check(),
      dsc1 + "additional " + dsc2);
  }
}

void
ProfilesExpirationTest::expired_visits_removal_()
{
  std::string two_days_earlier =
    (AutoTest::Time() - 2*24*60*60).
      get_gm_time().
      format(AutoTest::DEBUG_TIME_FORMAT);
  
  AdClient user(AdClient::create_user(this));

  add_descr_phrase("Match HT + H channels");

  std::string ht_ch1 = fetch_string("HT/Channel1");
  std::string ht_ch2 = fetch_string("HT/Channel2");
  std::string h_ch1 = fetch_string("H/Channel1");
  std::string h_ch2 = fetch_string("H/Channel2");
  std::string h_ch3 = fetch_string("H/Channel3");

  user.process_request(
    NSLRequest().
      referer_kw(fetch_string("HT/Keyword") + "," +
        fetch_string("H/Keyword")).
      debug_time(two_days_earlier));

  {
    std::string triggered_channels[] = {
      ht_ch1 + "P",
      ht_ch2 + "P",
      h_ch1 + "P",
      h_ch2 + "P",
      h_ch3 + "P"};

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        triggered_channels,
        user.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "match 'H' and 'HT' channels");
  }

  add_descr_phrase("Match S channels");

  std::string one_day_earlier =
    (AutoTest::Time() - 24*60*60).
      get_gm_time().
      format(AutoTest::DEBUG_TIME_FORMAT);

  std::string s_ch1 = fetch_string("S/Channel1");

  user.process_request(
    NSLRequest().
      referer_kw(fetch_string("S/Keyword")).
      debug_time(one_day_earlier));

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      s_ch1 + "P",
      user.debug_info.trigger_channels).check(),
    "match 'S' channel");

  std::string uid = user.debug_info.uid.value();

  add_descr_phrase("Deleting old profiles");

  DelAdmin::execute(this);
  
  add_descr_phrase("Profiles check");

  FAIL_CONTEXT(
    BaseProfileChecker(
      this, uid, false,
      AutoTest::UserInfoManagerController,
      BaseProfileChecker::Expected().
        page_ht_candidates(
          "\\[ channel_id = " +
          ht_ch1 +
          ", req_visits = 0, visits = 1, weight = 1 \\]").
        page_history_matches(
          "\\[ channel_id = " +
          h_ch2 +
          ", weight = 1 \\]").
        page_history_visits("").
        page_session_matches("")).check(),
    "Check base user profile for expected channels");

  FAIL_CONTEXT(
    HistoryProfileChecker(
      this, uid, false,
      AutoTest::UserInfoManagerController,
      HistoryProfileChecker::Expected().
        page_channels(
          "\\[ channel_id = " +
          ht_ch1 + ", days_visits_pairs = 2:1 \\] \\[ channel_id = " +
          h_ch1 + ", days_visits_pairs = 2:1 \\] \\[ channel_id = " +
          h_ch2 + ", days_visits_pairs = 2:1 \\]").
        search_channels("").
        url_channels("")).check(),
    "Check history user profile for expected channels");

};

void
ProfilesExpirationTest::user_profiles_removal_(
  AdClient& user1,
  unsigned long time1,
  AdClient& user2,
  unsigned long time2,
  bool temp_user)
{
  std::string cc_id = fetch_string("ADSC-5643/CC_ID");
  std::string fcap_id = fetch_string("ADSC-5643/FCAP");
  std::string channel_id = fetch_string("ADSC-5643/CHANNEL");
  std::string absent_colo = fetch_string("ABSENT_COLO");

  NSLRequest request;
  request.tid = fetch_string("ADSC-5643/TAG");
  request.referer_kw = fetch_string("ADSC-5643/KEYWORD");

  AutoTest::Time debug_time1 = AutoTest::Time() - time1 - 1;

  Generics::Timer timer;
  timer.start();

  request.debug_time =
    (debug_time1 - 24*60*60).get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT);
  user1.process_request(request);
  
  request.debug_time =
    debug_time1.get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT);
  user1.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      channel_id,
      user1.debug_info.history_channels).check(),
    "must get expected channel in history_channels debug_info header");
  
  if (!temp_user)
  {
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc_id,
        user1.debug_info.ccid).check(),
      "must got expected ccid");
  }
  
  if (remote_case_ && !temp_user)
  {
    user1.set_cookie_value("lc", absent_colo.c_str());
    user1.repeat_request();
  }

  AutoTest::Time debug_time2 =
    AutoTest::Time() - time2 + TIME_SHIFT;

  request.debug_time = (debug_time2 - 24*60*60).get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT);
  user2.process_request(request, "default colo");
  request.debug_time = debug_time2.get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT);
  user2.process_request(request, "default colo");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      channel_id,
      user2.debug_info.history_channels).check(),
    "must get expected channel in history_channels debug_info header");

  if (!temp_user)
  {
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc_id,
        user2.debug_info.ccid).check(),
      "must got expected ccid");
  }
  if (remote_case_ && !temp_user)
  {
      user2.set_cookie_value("lc", absent_colo.c_str());
      user2.repeat_request("with changed lc cookie");
  }

  std::string uid1 = user1.debug_info.uid.value();
  std::string uid2 = user2.debug_info.uid.value();

  add_descr_phrase("Check profiles exists");

  FAIL_CONTEXT(
    check_profiles_exist(uid2, true, temp_user),
    "Check profile#1");
  
  if (!temp_user)
  {
    FAIL_CONTEXT(
      FreqCapProfileChecker(
        this, uid2, false,
        AutoTest::UserInfoManagerController,
        FreqCapProfileChecker::Expected().
          fc_id(fcap_id)).check(),
      "UIM must contain freq cap profile for user 2");
  }

  add_descr_phrase("Deleting old profiles");

  DelAdmin::execute(this);

  timer.stop();

  add_descr_phrase("Check profiles have been deleted");

  if (timer.elapsed_time() >= TIME_SHIFT)
  {
    AutoTest::Logger::thlog().log("Test may fail: "
      "time between profiles creation "
      "and its deletion greater than expected.", Logging::Logger::WARNING);
  }

  FAIL_CONTEXT(
    check_profiles_exist(uid1, false, temp_user),
    "Check profile#2");

  FAIL_CONTEXT(
    check_profiles_exist(uid2, true, temp_user),
    "Check profile#3");
  
  if (!temp_user)
  {

    FAIL_CONTEXT(
      FreqCapProfileChecker(
        this, uid1, false,
        AutoTest::UserInfoManagerController,
        FreqCapProfileChecker::Expected(),
        AutoTest::AEC_NOT_EXISTS).check(),
      "UIM must contain freq cap profile for user 1");
    
    
    FAIL_CONTEXT(
      FreqCapProfileChecker(
        this, uid2, false, 
        AutoTest::UserInfoManagerController,
        FreqCapProfileChecker::Expected().
          fc_id(fcap_id)).check(),
      "UIM must contain freq cap profile for user 2");
  }
}

bool
ProfilesExpirationTest::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(
        CTE_ALL,
        STE_USER_INFO_MANAGER_CONTROLLER)),
    "UserInfoManagerController need for this test");

  remote_case_ =
    get_config().check_service(CTE_CENTRAL, STE_USER_INFO_EXCHANGER);

  AdClient
    user1(AdClient::create_user(this)),
    user2(AdClient::create_user(this));

  TemporaryAdClient
    tuser1(TemporaryAdClient::create_user(this)),
    tuser2(TemporaryAdClient::create_user(this));

  AUTOTEST_CASE(
    expired_visits_removal_(),
    "Expired visits removal");

  AUTOTEST_CASE(
    user_profiles_removal_(
      user1, 225*24*60*60,   // 225 days
      user2, 180*24*60*60),
    "Persistent users profile removal"); // 179 days

  AUTOTEST_CASE(
    user_profiles_removal_(
      tuser1, 37*60 + 30,  // 37 mins 30 secs
      tuser2, 30*60,       // 30 mins
      true),
    "Temporary users profile removal");
  
  return true;
}

