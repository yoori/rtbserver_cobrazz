
#include "StatProfilesExpirationTest.hpp"
 
REFLECT_UNIT(StatProfilesExpirationTest) (
  "Statistics",
  AUTO_TEST_SLOW,
  AUTO_TEST_SERIALIZE
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::NSLookupRequest NSLRequest;

  typedef AutoTest::ClearExpiredProfiles ClearExpiredProfiles;
  typedef AutoTest::DailyProcess DailyProcess;

  typedef AutoTest::RequestProfileEmptyChecker RequestProfileChecker;
  typedef AutoTest::PassbackProfileEmptyChecker PassbackProfileChecker;
  typedef AutoTest::InventoryProfileEmptyChecker InventoryProfileChecker;
  typedef AutoTest::UserTriggerMatchProfileEmptyChecker UserTriggerMatchProfileChecker;
  typedef AutoTest::RequestTriggerMatchProfileEmptyChecker RequestTriggerMatchProfileChecker;
  typedef AutoTest::TagRequestGroupProfileEmptyChecker TagRequestGroupProfileChecker;
  typedef AutoTest::ReachProfileEmptyChecker ReachProfileChecker;
  typedef AutoTest::ActionProfileEmptyChecker ActionProfileChecker;
  typedef AutoTest::FraudProfileEmptyChecker FraudProfileChecker;
  typedef AutoTest::SiteReachProfileEmptyChecker SiteReachProfileChecker;
  typedef AutoTest::UserKeywordProfileEmptyChecker UserKeywordProfileChecker;
  typedef AutoTest::RequestKeywordProfileEmptyChecker RequestKeywordProfileChecker;

  static const Generics::Time TIME_SHIFT = Generics::Time::ONE_MINUTE * 5; // 5 min
  static const char PAGE_LOAD_ID[] = "95917899";

  const int MINUTE = 60;
  const int HOUR = 60 * MINUTE;
  const int DAY = 24 * HOUR;

  const char PROFILE_NOT_FOUND[] = "Profile.*not found.";
};

void
StatProfilesExpirationTest::set_up()
{ }

bool
StatProfilesExpirationTest::run()
{
  add_descr_phrase("'Stats profiles removal' scenario started");

  std::string kw_disp = fetch_string("KEYWORD/DISPLAY");
  std::string cc_disp = fetch_string("CC/DISPLAY");
  std::string kw_text = fetch_string("KEYWORD/TEXT");
  std::string cc_text = fetch_string("CC/TEXT");
  std::string channel_id = fetch_string("CHANNEL");
  std::string k_channel = fetch_string("K-CHANNEL");

  NSLRequest request;
  request.tid = fetch_string("TAG");
  request.referer_kw = kw_disp;

  AutoTest::Time create_time(Generics::Time::get_time_of_day() - 226*DAY);

  AdClient
    base_user1(AdClient::create_user(this, create_time)),
    base_user2(AdClient::create_user(this, create_time)),
    request_user1(AdClient::create_user(this, create_time)),
    request_user2(AdClient::create_user(this, create_time)),
    passback_user1(AdClient::create_user(this, create_time)),
    passback_user2(AdClient::create_user(this, create_time)),
    tag_groups_user1(AdClient::create_user(this, create_time)),
    tag_groups_user2(AdClient::create_user(this, create_time)),
    em_user1(AdClient::create_user(this, create_time)),
    em_user2(AdClient::create_user(this, create_time)),
    ukeyword_user1(AdClient::create_user(this, create_time)),
    ukeyword_user2(AdClient::create_user(this, create_time)),
    rkeyword_user1(AdClient::create_user(this, create_time)),
    rkeyword_user2(AdClient::create_user(this, create_time)),
    rtmp_user1(AdClient::create_user(this, create_time)),
    rtmp_user2(AdClient::create_user(this, create_time));

  TemporaryAdClient
    temp_user1(TemporaryAdClient::create_user(this)),
    temp_user2(TemporaryAdClient::create_user(this));

  Generics::Timer timer;
  timer.start();

  // Reach, Action, Fraud, SiteReach profiles generation
  request.debug_time = //earlier_225
    (Generics::Time::get_time_of_day() - 225*DAY - 1).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  base_user1.process_request(request, "base profiles gen for user 1");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_disp,
      base_user1.debug_info.ccid).check(),
    "must got expected ccid");

  request.debug_time = //earlier_179
    (Generics::Time::get_time_of_day() - 180*DAY + TIME_SHIFT).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  base_user2.process_request(request, "base profiles gen for user 2");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_disp,
      base_user2.debug_info.ccid).check(),
                       "must got expected ccid");

  // RequestTriggerMatch profiles generation
  request.debug_time =
    (Generics::Time::get_time_of_day() - 2*DAY - 12*HOUR - 1).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  rtmp_user1.process_request(request, "RequestTriggerMatch profiles gen for user 1");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_disp,
      rtmp_user1.debug_info.ccid).check(),
    "must got expected ccid");

  request.debug_time =
    (Generics::Time::get_time_of_day() - 2*DAY + TIME_SHIFT).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  rtmp_user2.process_request(request, "RequestTriggerMatch profiles gen for user 2");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_disp,
      rtmp_user2.debug_info.ccid).check(),
                       "must got expected ccid");

  // Request info and UserKeyword profiles generation
  request.debug_time =
    (Generics::Time::get_time_of_day() - 9*DAY + 6*HOUR - 1).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  request_user1.process_request(request, "request profile gen for user 1");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc_disp,
        request_user1.debug_info.ccid).check(),
      "must got expected ccid");

  request.referer_kw = kw_text;
  ukeyword_user1.process_request(request, "user keyword profile gen for user 1");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      k_channel,
      ukeyword_user1.debug_info.trigger_channels).check(),
    "must get expected channel in trigger_channels debug_info header");

  request.referer_kw = kw_disp;
  request.debug_time =
    (Generics::Time::get_time_of_day() - 7*DAY + TIME_SHIFT).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  request_user2.process_request(request, "request profile gen for user 2");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_disp,
      request_user2.debug_info.ccid).check(),
    "must got expected ccid");

  request.referer_kw = kw_text;
  ukeyword_user2.process_request(request, "user keyword profile gen for user 2");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      k_channel,
      ukeyword_user2.debug_info.trigger_channels).check(),
    "must get expected channel in trigger_channels debug_info header");
  
  request.referer_kw = kw_disp;

  // Passback info profiles generation
  request.referer_kw.clear();
  request.debug_time =
    (Generics::Time::get_time_of_day() - 12*HOUR - 3*HOUR - 1).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  passback_user1.process_request(request, "passback profile gen for user 1");

  request.debug_time =
    (Generics::Time::get_time_of_day() - 12*HOUR + TIME_SHIFT).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  passback_user2.process_request(request, "passback profile gen for user 2");

  // Tag requests groups and RequestKeyword profiles geneartion
  request.referer = fetch_string("URL");
  request.pl = PAGE_LOAD_ID;
  request.debug_time =
    (Generics::Time::get_time_of_day() - 30*HOUR - 1).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  tag_groups_user1.process_request(request,
    "tag request groups profile gen for user 1");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_disp,
      tag_groups_user1.debug_info.ccid).check(),
    "must got expected ccid");

  request.referer.clear();
  request.pl.clear();
  request.referer_kw = kw_text;
  rkeyword_user1.process_request(request,
    "request keyword profile gen for user 1");
  rkeyword_user1.repeat_request();
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_text,
      rkeyword_user1.debug_info.ccid).check(),
    "must got expected ccid");
  
  request.referer_kw.clear();
  request.referer = fetch_string("URL");
  request.pl = PAGE_LOAD_ID;
  request.debug_time =
    (Generics::Time::get_time_of_day() - DAY + TIME_SHIFT).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  tag_groups_user2.process_request(request,
    "tag request groups profile gen for user 2");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_disp,
      tag_groups_user2.debug_info.ccid).check(),
    "must got expected ccid");

  request.referer.clear();
  request.pl.clear();
  request.referer_kw = kw_text;
  rkeyword_user2.process_request(request,
    "request keyword profile gen for user 2");
  rkeyword_user2.repeat_request();
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_text,
      rkeyword_user2.debug_info.ccid).check(),
    "must got expected ccid");

  // Inventory and UserTriggerMatch profiles generation
  request.tid.clear();
  request.referer.clear();
  request.referer_kw = kw_disp;
  request.debug_time =
    (Generics::Time::get_time_of_day() - 38*DAY + 12*HOUR - 1).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  em_user1.process_request(request,
    "inventory, estimation and user trigger match profiles gen for user 1");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      channel_id,
      em_user1.debug_info.trigger_channels).check(),
    "must get expected channel in trigger_channels debug_info header");

  request.debug_time =
    (Generics::Time::get_time_of_day() - 30*DAY + TIME_SHIFT).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  em_user2.process_request(request,
    "inventory, estimation and user trigger match profiles gen for user 2");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      channel_id,
      em_user2.debug_info.trigger_channels).check(),
    "must get expected channel in trigger_channels debug_info header");

  request.debug_time =
    (Generics::Time::get_time_of_day() - HOUR - 15*MINUTE - 1).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  temp_user1.process_request(request,
    "UserTriggerMatch profile gen for temp user 1");

  request.debug_time =
    (Generics::Time::get_time_of_day() - HOUR + TIME_SHIFT).
    get_gm_time().
    format(AutoTest::DEBUG_TIME_FORMAT);
  temp_user2.process_request(request,
    "UserTriggerMatch profile gen for temp user 2");

  std::string uid1 = base_user1.debug_info.uid.value().c_str();
  std::string uid2 = base_user2.debug_info.uid.value().c_str();

  // We can expect profiles only for user 2,
  // because profiles for user 1 can be deleted before check.
  add_wait_checker(
    "RIM must contain ccg reach profile for user 2",
    ReachProfileChecker(
      this,
      "\\" + uid2,
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "RIM must contain action profile for user 2",
    ActionProfileChecker(
      this,
      "\\" + uid2,
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "RIM must contain fraud profile for user 2",
    FraudProfileChecker(
      this,
      "\\" + uid2,
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "RIM must contain site reach profile for user 2",
    SiteReachProfileChecker(
      this,
      "\\" + uid2,
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "RIM must contain request profile for user 2",
    RequestProfileChecker(
      this,
      "\\" + request_user2.debug_info.creative_request_id.value(),
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "RIM must contain passback profile for user 2",
    PassbackProfileChecker(
      this,
      "\\" + passback_user2.debug_info.request_id.value(),
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "RIM must contain tag group profile for user 2",
    TagRequestGroupProfileChecker(
      this,
      "\\" + tag_groups_user2.debug_info.uid.value(),
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "EM must contain inventory profile for user 2",
    InventoryProfileChecker(
      this,
      "\\" + em_user2.debug_info.uid.value(),
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "EM must contain request trigger match profile for user 2",
    RequestTriggerMatchProfileChecker(
      this,
      "\\" + rtmp_user2.debug_info.creative_request_id.value(),
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "EM must contain user trigger match profile for user 2",
    UserTriggerMatchProfileChecker(
      this,
      "\\" + em_user2.debug_info.uid.value(),
        false,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  add_wait_checker(
    "EM must contain user trigger match profile for temp user 2",
    UserTriggerMatchProfileChecker(
      this,
      "\\" + temp_user2.debug_info.uid.value(),
      true,
      PROFILE_NOT_FOUND,
      AutoTest::AEC_NOT_EXISTS));

  check(false);

  add_descr_phrase("Deleting old profiles");
  ClearExpiredProfiles::execute(this);
  DailyProcess::execute(this);

  add_descr_phrase("Check profiles have been deleted");

  timer.stop();

  if (timer.elapsed_time() >= TIME_SHIFT)
  {
    AutoTest::Logger::thlog().log("Test may fail: "
      "time between profiles creation "
      "and its deletion greater than expected.", Logging::Logger::WARNING);
  }

  // Profiles checkers for user 2 are already in list,
  // need add profiles checkers for user 1
  add_wait_checker(
    "RIM must not contain ccg reach profile for user 1",
    ReachProfileChecker(
      this,
      "\\" + uid1,
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "RIM must not contain action profile for user 1",
    ActionProfileChecker(
      this,
      "\\" + uid1,
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "RIM must not contain fraud profile for user 1",
    FraudProfileChecker(
      this,
      "\\" + uid1,
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "RIM must not contain site reach profile for user 1",
    SiteReachProfileChecker(
      this,
      "\\" + uid1,
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "RIM must not contain request profile for user 1",
    RequestProfileChecker(
      this,
      "\\" + request_user1.debug_info.creative_request_id.value(),
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "RIM must not contain passback profile for user 1",
    PassbackProfileChecker(
      this,
      "\\" + passback_user1.debug_info.request_id.value(),
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "RIM must not contain tag group profile for user 1",
    TagRequestGroupProfileChecker(
      this,
      "\\" + tag_groups_user1.debug_info.uid.value(),
      AutoTest::RequestInfoManager,
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "EM must not contain inventory profile for user 1",
    InventoryProfileChecker(
      this,
      "\\" + em_user1.debug_info.uid.value(),
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "EM must not contain request trigger match profile for user 1",
    RequestTriggerMatchProfileChecker(
      this,
      "\\" + rtmp_user1.debug_info.creative_request_id.value(),
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "EM must notcontain user trigger match profile for user 1",
    UserTriggerMatchProfileChecker(
      this,
      "\\" + em_user1.debug_info.uid.value(),
      false,
      PROFILE_NOT_FOUND));

  add_wait_checker(
    "EM must not contain user trigger match profile for temp user 1",
    UserTriggerMatchProfileChecker(
      this,
      "\\" + temp_user1.debug_info.uid.value(),
      true,
      PROFILE_NOT_FOUND));

  check();

  return true;
}

void
StatProfilesExpirationTest::tear_down()
{}
