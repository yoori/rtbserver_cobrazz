
#include "NoAdvNoTrackTest.hpp"

REFLECT_UNIT(NoAdvNoTrackTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::BaseProfileChecker BaseProfileChecker;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::SpecialEffectsChecker SpecialEffectsChecker;
  typedef AutoTest::TagPassbackChecker TagPassbackChecker;
  const char* TIME_FORMAT  = "%Y-%m-%d %H:%M:%S";
}

NoAdvNoTrackTest::NoAdvNoTrackTest(
  UnitStat& stat_var, 
  const char* task_name, 
  XsdParams params_var)
  : BaseUnit(stat_var, task_name, params_var)
{ }

NoAdvNoTrackTest::~NoAdvNoTrackTest() noexcept
{ }

bool 
NoAdvNoTrackTest::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_USER_INFO_MANAGER_CONTROLLER)),
    "UserInfoManagerController need for this test");
  
  // Initialization
  no_track_words = fetch_string ("no_track_words");
  no_track_url_words = fetch_string ("no_track_url_words");
  no_track_urls = fetch_string ("no_track_urls");
  no_adv_words = fetch_string ("no_adv_words");
  no_adv_url_words = fetch_string ("no_adv_url_words");
  no_adv_urls = fetch_string ("no_adv_urls");

  // Tests
  AdClient client(AdClient::create_user(this));
  
  AUTOTEST_CASE(
    no_adv_case_(client),
    "No Advertisement");

  AUTOTEST_CASE(
    no_track_case_(client),
    "No Track");

  AUTOTEST_CASE(
    no_adv_and_no_track_case_(client),
    "No Advertisement and No Track");
  
  AUTOTEST_CASE(
    session_info_case_(),
    "Banned channels and session info");
  
  AUTOTEST_CASE(
    passback_case_(),
    "Banned channels and passback");
  
  return true;
}

void
NoAdvNoTrackTest::no_adv_case_(
  AdClient& client)
{
  {
    add_descr_phrase("No adv by urls");

    SpecialEffectsChecker checker(
      client,
      NSLookupRequest().
       tid(fetch_string("Tag")).
        referer(no_adv_urls + "/advertising.html"),
      "0", SpecialEffectsChecker::SE_NO_ADV);

    FAIL_CONTEXT(
      checker.check(),
      "Check 'NO ADV' effect");
    
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel1_2",
        checker.client().debug_info.trigger_channels).check(),
      "Check url channels");
  }

  {
    add_descr_phrase("No adv by url keywords");

    SpecialEffectsChecker checker(
      client,
      NSLookupRequest().
        tid(fetch_string("Tag")).
        search(no_adv_url_words),
      "0", SpecialEffectsChecker::SE_NO_ADV);
    
    FAIL_CONTEXT(
      checker.check(),
      "Check 'NO ADV' effect");

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel1_4",
        checker.client().debug_info.trigger_channels).check(),
      "Check page channels");

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel1",
        checker.client().debug_info.history_channels).check(),
      "Check history channels");
  }

      
  {
    add_descr_phrase("No adv by keywords");

    SpecialEffectsChecker checker(
      client,
      NSLookupRequest().
        tid(fetch_string("Tag")).
        ft(no_adv_words),
      "0", SpecialEffectsChecker::SE_NO_ADV);
    
    FAIL_CONTEXT(
      checker.check(),
      "Check 'NO ADV' effect");

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel2_1,Channel3_1,Channel4_1",
        checker.client().debug_info.trigger_channels).check(),
      "Check page channels");

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Channel1,Channel2",
        checker.client().debug_info.history_channels).check(),
      "Check history channels");
  }

}
 
void
NoAdvNoTrackTest::no_track_case_(
  AdClient& client)
{

  {
    add_descr_phrase("No track by url keywords");

    {
      SpecialEffectsChecker checker(
        client,
        NSLookupRequest().
          tid(fetch_string("Tag")).
          referer(fetch_string("not_store_kwd")  + ".com/" + no_track_url_words ),
        "0", SpecialEffectsChecker::SE_NO_TRACK);

      FAIL_CONTEXT(
        checker.check(),
        "Check 'NO TRACK' effect");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel5_3",
          checker.client().debug_info.trigger_channels).check(),
        "Check trigger channels");

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !checker.client().debug_info.history_channels.size()), 
        "Check empty history on NO TRACK");
    }

    {
      SpecialEffectsChecker checker(
        client,
        NSLookupRequest().tid(fetch_string("Tag")),
        fetch_string("CC"));

      FAIL_CONTEXT(
        checker.check(),
        "Check creative");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel1,Channel2",
          checker.client().debug_info.history_channels).check(),
        "Check history channels");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "ChannelNotProfile",
          checker.client().debug_info.history_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        "Check not profile channel");
    }
  }
  
  
  {
    add_descr_phrase("No track by urls");

    {
      SpecialEffectsChecker checker(
        client,
        NSLookupRequest().
          tid(fetch_string("Tag")).
          referer(no_track_urls + "/abs").
          referer_kw(fetch_string("not_store_kwd")),
        "0", SpecialEffectsChecker::SE_NO_TRACK);

      FAIL_CONTEXT(
        checker.check(),
        "Check 'NO TRACK' effect");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel5_1",
          checker.client().debug_info.trigger_channels).check(),
        "Check trigger channels");

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !checker.client().debug_info.history_channels.size()), 
        "Check empty history on NO TRACK");
    }

    {
      SpecialEffectsChecker checker(
        client,
        NSLookupRequest().tid(fetch_string("Tag")),
        fetch_string("CC"));

      FAIL_CONTEXT(
        checker.check(),
        "Check creative");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel1,Channel2",
          checker.client().debug_info.history_channels).check(),
        "Check history channels");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "ChannelNotProfile",
          checker.client().debug_info.history_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        "Check not profile channel");
    }
  }

  {
    add_descr_phrase("No track by keywords");

    {
      SpecialEffectsChecker checker(
        client,
        NSLookupRequest().
          tid(fetch_string("Tag")).
          search(no_track_words + "+" + fetch_string("not_store_kwd")).
          ft(no_track_words),
        "0", SpecialEffectsChecker::SE_NO_TRACK);

      FAIL_CONTEXT(
        checker.check(),
        "Check 'NO TRACK' effect");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel5_2",
          checker.client().debug_info.trigger_channels).check(),
        "Check trigger channels");

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !checker.client().debug_info.history_channels.size()), 
        "Check empty history on NO TRACK");

    }

    {
      SpecialEffectsChecker checker(
        client,
        NSLookupRequest().tid(fetch_string("Tag")),
        fetch_string("CC"));

      FAIL_CONTEXT(
        checker.check(),
        "Check creative");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel1,Channel2",
          checker.client().debug_info.history_channels).check(),
        "Check history channels");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "ChannelNotProfile",
          checker.client().debug_info.history_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        "Check not profile channel");
    }
  }
}


void
NoAdvNoTrackTest::no_adv_and_no_track_case_(
  AdClient& client)
{

  {
    SpecialEffectsChecker checker(
      client,
      NSLookupRequest().
        tid(fetch_string("Tag")).
        referer(no_track_urls + "/abs").
        referer_kw(no_adv_words + "," + fetch_string("not_store_kwd")),
      "0", SpecialEffectsChecker::SE_NO_TRACK | SpecialEffectsChecker::SE_NO_ADV);

    FAIL_CONTEXT(
      checker.check(),
      "Check 'NO TRACK & NO ADV' effects");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !checker.client().debug_info.history_channels.size()), 
      "Check empty history on NO TRACK");
  }

  {
    SpecialEffectsChecker checker(
      client,
      NSLookupRequest().tid(fetch_string("Tag")),
      fetch_string("CC"));

      FAIL_CONTEXT(
        checker.check(),
        "Check creative");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "Channel1,Channel2",
          checker.client().debug_info.history_channels).check(),
        "Check history channels");

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "ChannelNotProfile",
          checker.client().debug_info.history_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        "Check not profile channel");
  }
}

void
NoAdvNoTrackTest::session_info_case_()
{
  AutoTest::Time now;
  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;

  FAIL_CONTEXT(
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        tid(fetch_string("Tag")).
        referer(no_track_urls + "/abs").
        referer_kw(no_adv_words + ",").
        debug_time(now),
      "0",
      SpecialEffectsChecker::SE_NO_TRACK |
      SpecialEffectsChecker::SE_NO_ADV).check(),
    "Check 'NO TRACK & NO ADV' effects (Now)");

  FAIL_CONTEXT(
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        tid(fetch_string("Tag")).
        referer(no_track_urls + "/parliament.html").
        referer_kw(no_adv_words + ",").
        debug_time(now+10),
      "0",
      SpecialEffectsChecker::SE_NO_TRACK |
      SpecialEffectsChecker::SE_NO_ADV).check(),
    "Check 'NO TRACK & NO ADV' effects (10 sec later)");
  
  
  std::string session_start_expected =
    now.get_gm_time().format(TIME_FORMAT);
  std::string last_request_expected =
    (now + 10).get_gm_time().format(TIME_FORMAT);

  std::string uid =
    AutoTest::prepare_uid(
      client.get_uid(),
      AutoTest::UUE_ADMIN_PARAMVALUE);
   
  FAIL_CONTEXT(
    BaseProfileChecker(
      this, uid, false,
      AutoTest::UserInfoManagerController,
      BaseProfileChecker::Expected().
        last_request_time(last_request_expected).
        session_start_time(session_start_expected)).check(),
    "Check admin times");

  client.process_request(
    NSLookupRequest().
      tid(fetch_string("Tag")).
      referer(no_track_urls + "/parliament.html").
      referer_kw(no_adv_words + ",").
      debug_time(now+10));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      last_request_expected,
      client.debug_info.last_request_time).check(),
    "Check last_request_time");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      session_start_expected,
      client.debug_info.session_start).check(),
    "Check session_start");
}

void
NoAdvNoTrackTest::passback_case_()
{
  AdClient client(AdClient::create_user(this));

  NSLookupRequest requests[] =
  {
    NSLookupRequest().referer_kw(no_adv_words),
    NSLookupRequest().referer(no_track_urls + "/abs"),
    NSLookupRequest().referer_kw(no_adv_words).referer(no_track_urls + "/abs"),
  };

  for (size_t i = 0; i < countof(requests); ++i)
  {
    client.process_request(
      requests[i].tid(fetch_string("PassbackTag")));

    FAIL_CONTEXT(
      TagPassbackChecker(
        client,
        fetch_string("PassbackURL")).check(),
      "Passback check#" + strof(i+1));
  }
}
