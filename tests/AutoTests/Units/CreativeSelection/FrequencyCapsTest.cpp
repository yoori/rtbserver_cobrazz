
#include "FrequencyCapsTest.hpp"

REFLECT_UNIT(FrequencyCapsTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::FreqCapProfileAdmin FreqCapProfileAdmin;
  const unsigned int ITERATION_DELAY = 10;
  const unsigned int MINUTE = 60;
  const char ASPECT[] = "FrequencyCapsTest";
  const unsigned long INFO = Logging::Logger::INFO;
}

namespace Data
{
  const char FC_WINDOW_TIME[] = "WindowTime";
  const char FC_WINDOW_LIMIT[] = "WindowLimit";
  const char FC_LIFE_LIMIT[] = "LifeLimit";
  const char FC_PERIOD[] = "Period";

  const char FC_COMB_WINDOW_TIME[] = "CombWindowTime";
  const char FC_COMB_PERIOD[] = "CombPeriod";

  const char TRACK_PIXEL_CREATIVE_FORMAT[] = "unit-test-imp";
  const unsigned long FC_CONFIRM_TIMEOUT = 60; // 60 seconds
}

bool
FrequencyCapsTest::run_test()
{
  min_request_period = 0;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_USER_INFO_MANAGER)),
    "UserInfoManager need for this test");

  add_descr_phrase("Starting");

  time_ = AutoTest::Time::get_time_of_day();

  // campaign frequency caps cases
  NOSTOP_FAIL_CONTEXT(process_window_limit_display_ad_case_(
    "Campaign window limit frequency caps and display ad",
    "Cmp-Window",
    false));

  NOSTOP_FAIL_CONTEXT(process_window_limit_display_ad_case_(
    "Campaign window limit frequency caps and display ad with Tracking",
    "Cmp-Window-Track",
    true));

  NOSTOP_FAIL_CONTEXT(process_window_limit_text_ads_case_(
    "Campaign window limit frequency caps and text channel ads",
    "Cmp-Window-TextCh",
    false));

  NOSTOP_FAIL_CONTEXT(process_life_count_display_ad_case_(
    "Campaign life limit frequency caps and display ad",
    "Cmp-LifeCount",
    false));

  NOSTOP_FAIL_CONTEXT(process_life_count_display_ad_case_(
    "Campaign life limit frequency caps and display ad with Tracking",
    "Cmp-LifeCount-Track",
    true));

  NOSTOP_FAIL_CONTEXT(process_campaign_combined_limits_case_());

  // ccg frequency caps cases
  NOSTOP_FAIL_CONTEXT(process_window_limit_display_ad_case_(
    "CCG window limit frequency caps and display ad",
    "Ccg-Window",
    false));

  NOSTOP_FAIL_CONTEXT(process_window_limit_text_ads_case_(
    "CCG window limit frequency caps and text keyword ads",
    "Ccg-Window-Text",
    false));

  NOSTOP_FAIL_CONTEXT(process_window_limit_text_ads_case_(
    "CCG window limit frequency caps and text channel ads",
    "Ccg-Window-TextCh",
    false));

  NOSTOP_FAIL_CONTEXT(process_life_count_display_ad_case_(
    "CCG life limit frequency caps and display ads",
    "Ccg-LifeCount",
    false));

  NOSTOP_FAIL_CONTEXT(process_life_count_text_ads_case_(
    "CCG life limit frequency caps and text keyword ads",
    "Ccg-LifeCount-Text",
    false));

  NOSTOP_FAIL_CONTEXT(process_life_count_text_ads_case_(
    "CCG life limit frequency caps and text channel ads",
    "Ccg-LifeCount-TextCh",
    false));

  NOSTOP_FAIL_CONTEXT(process_period_display_ad_case_(
    "CCG period frequency caps and display ad",
    "Ccg-Period"));

  NOSTOP_FAIL_CONTEXT(process_period_text_ads_case_(
    "CCG period frequency caps and text channel ads",
    "Ccg-Period-TextCh",
    false));

  // creative frequency caps cases
  NOSTOP_FAIL_CONTEXT(process_window_limit_display_ad_case_(
    "Creative window limit frequency caps and display ad",
    "Cc-Window",
    false));

  NOSTOP_FAIL_CONTEXT(process_life_count_display_ad_case_(
    "Creative life limit frequency caps and display ad",
    "Cc-LifeCount",
    false));

  NOSTOP_FAIL_CONTEXT(process_period_display_ad_case_(
    "Creative period frequency caps and display ad",
    "Cc-Period"));

  NOSTOP_FAIL_CONTEXT(process_period_text_ads_case_(
    "Creative period frequency caps and text keyword ads",
    "Cc-Period-Text",
    false));

  NOSTOP_FAIL_CONTEXT(
    process_creative_window_limit_with_competitive_creative_case_());

  // channel frequency caps cases
  NOSTOP_FAIL_CONTEXT(process_window_limit_text_ads_case_(
    "Channel life limit frequency caps and text keyword ads all linked to one channel",
    "Ch-Window-Text",
    false));

  // run stadard life count scenario but with freq cap multiplied to 3 (
  //   three channel fc impressions at each showing).
  NOSTOP_FAIL_CONTEXT(process_life_count_text_ads_case_(
    "Channel window limit frequency caps and text keyword ads",
    "Ch-LifeCount-Text-OneChannel",
    true // no ads if fc is full
    ));

  // site frequency caps cases
  // specific: all bundles created with enabled tracking
  // but realy, for site freq caps always used as confirmed
  // use track = false for scenario running
  NOSTOP_FAIL_CONTEXT(process_window_limit_display_ad_case_(
    "Site window limit frequency caps and display ad with Tracking",
    "Site-Window-Track",
    false
    ));

  NOSTOP_FAIL_CONTEXT(process_window_limit_text_ads_case_(
    "Site window limit frequency caps and text ads with Tracking",
    "Site-Window-Text-Track",
    true, // no ads if fullfil
    false));

  NOSTOP_FAIL_CONTEXT(process_life_count_display_ad_case_(
    "Site life limit frequency caps and display ad with Tracking",
    "Site-LifeCount-Track",
    false
    ));

  NOSTOP_FAIL_CONTEXT(process_life_count_text_ads_case_(
    "Site life limit frequency caps and text channel ads with Tracking",
    "Site-LifeCount-Text-Track",
    true, // no ads if fullfil
    false // no track scenario
    ));

  NOSTOP_FAIL_CONTEXT(process_period_display_ad_case_(
    "Site period frequency caps and display ad",
    "Site-Period"));

  NOSTOP_FAIL_CONTEXT(process_period_text_ads_case_(
    "Site period frequency caps and text keyword ads",
    "Site-Period-Text",
    true // no ads if fc full
    ));

  return true;
}

void
FrequencyCapsTest::process_campaign_combined_limits_case_()
{
  add_descr_phrase("Campaign combined frequency caps case");

  AdClient test_client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid = fetch_string("Tag/Cmp-Comb");
  request.referer_kw = fetch_string("KWD/Cmp-Comb");

  StrVector exp_cc;
  exp_cc.push_back(fetch_string("CC/Cmp-Comb"));

  FAIL_CONTEXT(process_combined_limits_case_(
    test_client,
    request,
    exp_cc));
}

void
FrequencyCapsTest::
process_creative_window_limit_with_competitive_creative_case_()
{
  // Create Campaign campaign#4. Create creative#4_1 & creative#4_2
  // linked with campaign#4. Link creative#4_1 with frequency caps:
  // window_length = 2 min & window_count=1. Creative#4_2 don't have
  // link with frequency caps. Create other entities (such as channel,
  // triggerlist, tag and so on) for making possibility advertising
  // request to creative#4_1 & creative#4_2.

  add_descr_phrase("Creative window count frequency caps with "
    "other competitive free creative case");

  AdClient test_client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid = fetch_string("Tag/Cc2-Window");
  request.referer_kw = fetch_string("KWD/Cc2-Window");

  FAIL_CONTEXT(process_window_limit_and_competitive_selection_case_(
    test_client, request));
}

void
FrequencyCapsTest::process_window_limit_simple_case_(
  AdClient& test_client,
  NSLookupRequest& request,
  const StrVector& nofc_ccids,
  const StrVector& fullfil_fc_ccids,
  bool expected_track_pixel,
  FreqCapConfirmStrategy confirm_imps)
{
  /* Expect:
   *   window_limit + 1 < window_time
   * Scenario:
   *   1. start_time + i: Send advertising request #i (0, window_limit),
   *    expect creative.
   *   2. start_time + window_time - 1 < step #1 time + window_limit:
   *      Send advertising request, expect no creative.
   *   3. start_time + window_time + 1 sec:
   *      Send advertising request, expect creative.
   */
  unsigned int window_limit = fetch_int(Data::FC_WINDOW_LIMIT);
  unsigned int window_time = fetch_int(Data::FC_WINDOW_TIME);

  // 1
  print_fcap_ui_(test_client);

  std::string track_pixel_url;
  std::list<std::string> track_requests;

  if(expected_track_pixel)
  {
    request.format(Data::TRACK_PIXEL_CREATIVE_FORMAT);
  }

  Generics::Time start_time = time_;

  for(unsigned int i = 0; i < window_limit; ++i)
  {
    request.debug_time(time_);

    test_client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        nofc_ccids,
        SelectedCreativesCCID(test_client)).check(),
      "unexpected creatives (not full freq caps)");

    if (expected_track_pixel)
    {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !test_client.debug_info.track_pixel_url.empty()),
      "must got track_pixel_url");

      if(confirm_imps == FCC_CONFIRM_PREVIOUS_AFTER_NEW)
      {
        std::string new_track_pixel_url = test_client.debug_info.track_pixel_url;

        if(!track_pixel_url.empty())
        {
          // track previous only after new done ad request
          test_client.process_request(track_pixel_url);
        }

        track_pixel_url = new_track_pixel_url;
      }
      else if(confirm_imps == FCC_CONFIRM_ALL_AFTER)
      {
        track_requests.push_back(test_client.debug_info.track_pixel_url);
      }
    }

    time_ += 1;
  }

  if(!track_pixel_url.empty())
  {
    // track previous only after new done ad request
    test_client.process_request(track_pixel_url);
  }

  for(std::list<std::string>::const_iterator tp_it =
        track_requests.begin();
      tp_it != track_requests.end(); ++tp_it)
  {
    test_client.process_request(*tp_it);
  }

  print_fcap_ui_(test_client);

  // 2
  time_ = start_time + window_time - 1;
  request.debug_time(time_);
  test_client.process_request(request);

  print_fcap_ui_(test_client);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      fullfil_fc_ccids,
      SelectedCreativesCCID(test_client)).check(),
    "unexpected creatives (full freq caps)");

  // 3
  time_ = start_time + window_time + 1;
  request.debug_time(time_);
  test_client.process_request(request);

  print_fcap_ui_(test_client);
  
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        nofc_ccids,
        SelectedCreativesCCID(test_client)).check(),
      "unexpected creatives (not full freq caps)");

  time_ += window_time + 2;
}

void
FrequencyCapsTest::process_window_limit_confirm_timeout_case_(
  AdClient& test_client,
  NSLookupRequest& request,
  const StrVector& nofc_ccids)
{
  /* Expect: window_time > confirm timeout
   *   window_limit + 1 < window_time
   * Scenario:
   *   1. start_time + i: Send advertising request #i (0, window_limit),
   *      expect creative. don't confirm it
   *   2. start_time + confirm_timeout + i: Send advertising request #i (0, window_limit),
   *      expect creative.
   *      If confirm_timeout < window_time window time mus block ad showing,
   *      but requests timeouted as unconfirmed - this allow to show ad.
   */
  unsigned int window_limit = fetch_int(Data::FC_WINDOW_LIMIT);

  // 1
  print_fcap_ui_(test_client);

  request.format(Data::TRACK_PIXEL_CREATIVE_FORMAT);

  for(unsigned int confirm_window_i = 0; confirm_window_i < 2; ++confirm_window_i)
  {
    for(unsigned int i = 0; i < window_limit; ++i)
    {
      request.debug_time(time_);

      test_client.process_request(request);

      print_fcap_ui_(test_client);

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          nofc_ccids,
          SelectedCreativesCCID(test_client)).check(),
        "server must return expected creatives");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !test_client.debug_info.track_pixel_url.empty()),
      "must got track_pixel_url");
    }

    time_ += Data::FC_CONFIRM_TIMEOUT + 1;
  }
}

void
FrequencyCapsTest::process_window_limit_display_ad_case_(
  const char* case_name,
  const char* entities_name,
  bool track_pixel)
{
  add_descr_phrase(case_name);

  AdClient ta_client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid = fetch_string(std::string("Tag/") + entities_name);
  request.referer_kw = fetch_string(std::string("KWD/") + entities_name);

  time_ += min_request_period + 1; // wait global freq caps period

  StrVector nofc_exp_ccids;
  nofc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name));

  FAIL_CONTEXT(process_window_limit_case_(
    case_name,
    ta_client,
    request,
    nofc_exp_ccids,
    StrVector(), // no creatives if fc is full
    track_pixel
    ));
}

void FrequencyCapsTest::process_window_limit_text_ads_case_(
  const char* case_name,
  const char* entities_name,
  bool noads_if_fc_full,
  bool track_pixel)
{
  add_descr_phrase(case_name);

  AdClient ta_client(AdClient::create_user(this));

  ta_client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string(std::string("KWD/") + entities_name)).
      debug_time(time_));

  StrVector nofc_exp_ccids;
  nofc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-1"));
  nofc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-2"));
  nofc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-3"));

  StrVector fullfc_exp_ccids;
  // for Site case no creatives expected
  if(!noads_if_fc_full)
  {
    fullfc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-2"));
    fullfc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-3"));
    fullfc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-4"));
  }

  NSLookupRequest request;
  request.tid = fetch_string(std::string("Tag/") + entities_name);
  request.referer_kw = fetch_string(std::string("KWD/") + entities_name);

  FAIL_CONTEXT(process_window_limit_case_(
    case_name,
    ta_client,
    request,
    nofc_exp_ccids,
    fullfc_exp_ccids,
    track_pixel));
}

void
FrequencyCapsTest::process_window_limit_case_(
  const char* case_name,
  AdClient& test_client,
  NSLookupRequest& request,
  const StrVector& nofc_ccids,
  const StrVector& fullfil_fc_ccids,
  bool track_pixel)
{
  /* For track_pixel = false simple scenario.
   * For track_pixel = true:
   *   [time = 0]: Run simple scenario with sending previous track pixel
   *     after new ad request done
   *   [time = window_limit]: Run simple scenario without track pixel sending.
   *   [time = 2*window_limit]: Run simple scenario without track pixel sending,
   *     for check that previous impressions expired by confirmation timeout
   */
  if(track_pixel)
  {
    add_descr_phrase(std::string(case_name) + " (confirm previous after new sending)");

    FAIL_CONTEXT(process_window_limit_simple_case_(
      test_client,
      request,
      nofc_ccids,
      fullfil_fc_ccids,
      true, // expect track pixel
      FCC_CONFIRM_PREVIOUS_AFTER_NEW));

    add_descr_phrase(std::string(case_name) + " (confirm all after)");

    FAIL_CONTEXT(process_window_limit_simple_case_(
      test_client,
      request,
      nofc_ccids,
      fullfil_fc_ccids,
      true, // expect track pixel
      FCC_CONFIRM_ALL_AFTER));

    add_descr_phrase(std::string(case_name) + " (confirm timeout check)");

    FAIL_CONTEXT(process_window_limit_confirm_timeout_case_(
      test_client,
      request,
      nofc_ccids));
  }
  else
  {
    add_descr_phrase(std::string(case_name) + " (no track)");

    FAIL_CONTEXT(process_window_limit_simple_case_(
      test_client,
      request,
      nofc_ccids,
      fullfil_fc_ccids,
      false));
  }
}

void
FrequencyCapsTest::process_window_limit_and_competitive_selection_case_(
  AdClient &test_client,
  NSLookupRequest& request)
{
  // Scenario:
  //
  // 1. start_time:
  //      repeat request for campaign #4 while creative#4_1 showing number < 2.
  // 2. start_time + i (i < window_time):
  //      repeat request for campaign#4: creative #2 must be returned.
  // 3. start_time + window_time: repeat request for campaign #4:
  //      success if creative #1 returned
  //      continue requesting if creative #2 returned
  //
  const unsigned long MAX_REQUESTS_AT_FIRST_STEP = 20;
  const unsigned long MAX_REQUESTS_AT_LAST_STEP = 20;
  const Generics::Time window_time(fetch_int(Data::FC_WINDOW_TIME));
  unsigned long cc1_allowed_showing_number =
    fetch_int(Data::FC_WINDOW_LIMIT);

  StrVector exp1_ccids;
  exp1_ccids.push_back(fetch_string("CC/Cc2-Window-1"));

  StrVector exp2_ccids;
  exp2_ccids.push_back(fetch_string("CC/Cc2-Window-2"));

  Generics::Time start_time = time_;

  // step 1
  unsigned long i = 0;
  request.debug_time(time_);

  do
  {
    test_client.process_request(request);

    if(
      AutoTest::sequence_checker(
        exp1_ccids,
        SelectedCreativesCCID(test_client)).check(false))
    {
      --cc1_allowed_showing_number;
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          exp2_ccids,
          SelectedCreativesCCID(test_client)).check(),
        "unexpected creatives (not full freq caps)");
    }

    ++i;
  }
  while (cc1_allowed_showing_number && i < MAX_REQUESTS_AT_FIRST_STEP);

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        i < MAX_REQUESTS_AT_FIRST_STEP),
      "must requests for first creative exceeds");

  // step 2
  start_time = time_;
  time_ += 1;

  do
  {
    request.debug_time(time_);

    test_client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp2_ccids,
        SelectedCreativesCCID(test_client)).check(),
      "unexpected creatives (full freq caps)");

    time_ += ITERATION_DELAY;
  }
  while (time_ - start_time < window_time);

  // step 3
  i = 0;

  do
  {
    test_client.process_request(request);

    if(
      AutoTest::sequence_checker(
        exp1_ccids,
        SelectedCreativesCCID(test_client)).check(false))
    {
      break;
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          exp2_ccids,
          SelectedCreativesCCID(test_client)).check(),
        "unexpected creatives (not full freq caps)");
    }

    ++i;
  }
  while (cc1_allowed_showing_number && i < MAX_REQUESTS_AT_LAST_STEP);

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      i < MAX_REQUESTS_AT_LAST_STEP),
    "must requests for first creative at last step exceeds");
}

void
FrequencyCapsTest::process_life_count_display_ad_case_(
  const char* case_name,
  const char* entities_name,
  bool track_pixel)
{
  add_descr_phrase(case_name);

  NSLookupRequest request;
  request.tid = fetch_string(std::string("Tag/") + entities_name);
  request.referer_kw = fetch_string(std::string("KWD/") + entities_name);

  StrVector nofc_exp_ccids;
  nofc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name));

  AdClient client(AdClient::create_user(this));

  FAIL_CONTEXT(process_life_count_case_(
    client,
    request,
    nofc_exp_ccids,
    StrVector(),
    track_pixel));
}

void
FrequencyCapsTest::process_life_count_text_ads_case_(
  const char* case_name,
  const char* entities_name,
  bool noads_if_fc_full,
  bool track_pixel)
{
  add_descr_phrase(case_name);

  NSLookupRequest request;
  request.tid = fetch_string(std::string("Tag/") + entities_name);
  request.referer_kw = fetch_string(std::string("KWD/") + entities_name);

  StrVector nofc_ccids;
  nofc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-1"));
  nofc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-2"));
  nofc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-3"));

  StrVector fullfil_fc_ccids;
  if(!noads_if_fc_full)
  {
    fullfil_fc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-2"));
    fullfil_fc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-3"));
    fullfil_fc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-4"));
  }

  if(track_pixel)
  {
    {
      AdClient client(AdClient::create_user(this));

      client.process_request(
        NSLookupRequest().
          referer_kw(fetch_string(std::string("KWD/") + entities_name)).
          debug_time(time_));

      FAIL_CONTEXT(process_life_count_case_(
        client,
        request,
        nofc_ccids,
        fullfil_fc_ccids,
        true // confirm previous after new sending
        ));
    }
    
    {
      add_descr_phrase(std::string(case_name) + " (don't confirm)");

      AdClient client(AdClient::create_user(this));

      FAIL_CONTEXT(process_life_count_case_(
        client,
        request,
        nofc_ccids,
        fullfil_fc_ccids,
        false // dont' confirm
        ));

      add_descr_phrase(std::string(case_name) + " (repeat after confirm timeout)");

      time_ += Data::FC_CONFIRM_TIMEOUT + 1;
  
      // repeat scenario without track confirmation after confirm timeout
      // must work because, previous requests timeouted (by fc, as unconfirmed)
      FAIL_CONTEXT(process_life_count_case_(
        client,
        request,
        nofc_ccids,
        fullfil_fc_ccids,
        false // don't confirm requests
        ));
    }
  }
  else
  {
    AdClient client(AdClient::create_user(this));

    client.process_request(
      NSLookupRequest().
        referer_kw(fetch_string(std::string("KWD/") + entities_name)).
        debug_time(time_));

    FAIL_CONTEXT(process_life_count_case_(
      client, request, nofc_ccids, fullfil_fc_ccids));
  }
}

void
FrequencyCapsTest::process_life_count_case_(
  AdClient& test_client,
  NSLookupRequest& request,
  const StrVector& nofc_ccids,
  const StrVector& fullfil_fc_ccids,
  bool track_pixel)
{
  // 1. Create user#1 (send request for probe uid from AdServer client#1).
  // 2. User#1 send advertising first request for campaign#5.
  // 3. Test that server return campaign#5 creative.
  // 4. User#1 send advertising second request for campaign#5.
  // 5. Test that server return campaign#5 creative.
  // 6. User#1 send advertising second request for campaign#5.
  // 7. Test creative not returned.

  const unsigned int life_count = fetch_int(Data::FC_LIFE_LIMIT);
  std::string track_pixel_url;

  if(track_pixel)
  {
    request.format(Data::TRACK_PIXEL_CREATIVE_FORMAT);
  }

  unsigned long i = 0;
  for (; i < life_count; ++i)
  {
    time_ += 1;
    request.debug_time(time_);
    test_client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        nofc_ccids,
        SelectedCreativesCCID(test_client)).check(),
      "unexpected creatives (not full freq caps)");

    if (track_pixel)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !test_client.debug_info.track_pixel_url.empty()),
        "must got track_pixel_url");

      if(!track_pixel_url.empty())
      {
        test_client.process_request(track_pixel_url);
      }

      track_pixel_url = test_client.debug_info.track_pixel_url;
    }
  }

  time_ += 1;
  request.debug_time(time_);
  test_client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      fullfil_fc_ccids,
      SelectedCreativesCCID(test_client)).check(),
    "unexpected creatives (full freq caps)");
}

void
FrequencyCapsTest::process_period_display_ad_case_(
  const char* case_name,
  const char* entities_name)
{
  add_descr_phrase(case_name);

  NSLookupRequest request;
  request.tid = fetch_string(std::string("Tag/") + entities_name);
  request.referer_kw = fetch_string(std::string("KWD/") + entities_name);

  StrVector nofc_exp_ccids;
  nofc_exp_ccids.push_back(fetch_string(std::string("CC/") + entities_name));

  AdClient client(AdClient::create_user(this));

  FAIL_CONTEXT(process_period_case_(
    client,
    request,
    nofc_exp_ccids,
    StrVector() // no creatives if fc is full
    ));
}

void
FrequencyCapsTest::process_period_text_ads_case_(
  const char* case_name,
  const char* entities_name,
  bool noads_if_fc_full)
{
  add_descr_phrase(case_name);

  NSLookupRequest request;
  request.tid = fetch_string(std::string("Tag/") + entities_name);
  request.referer_kw = fetch_string(std::string("KWD/") + entities_name);

  StrVector nofc_ccids;
  nofc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-1"));
  nofc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-2"));
  nofc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-3"));

  StrVector fullfil_fc_ccids;
  if(!noads_if_fc_full)
  {
    fullfil_fc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-2"));
    fullfil_fc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-3"));
    fullfil_fc_ccids.push_back(fetch_string(std::string("CC/") + entities_name + "-4"));
  }

  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string(std::string("KWD/") + entities_name)).
      debug_time(time_));

  FAIL_CONTEXT(process_period_case_(
    client, request, nofc_ccids, fullfil_fc_ccids));
}

void
FrequencyCapsTest::process_period_case_(
  AdClient &test_client,
  NSLookupRequest& request,
  const StrVector& nofc_ccids,
  const StrVector& fullfil_fc_ccids)
{
  // 1. Send first advertising request for campaign#6.
  // 2. Test that server return ccg#6 creative & store timespamp#1 for
  //    first creative showing.
  // 3. Send advertising requests for campaign#6, until creative for
  //    campaign#6 is returned.
  // 4. Compare timestamp#1 with timestamp#2 when was second
  //    campaign#6 creative showing. If difference between timestamps
  //    less than the delay - test fail, else - OK.
  Generics::Time period(fetch_int(Data::FC_PERIOD));

  request.debug_time(time_);

  test_client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      nofc_ccids,
      SelectedCreativesCCID(test_client)).check(),
    "unexpected creatives (not full freq caps)");

  Generics::Time start_time = time_;

  while(time_ - start_time < period)
  {
    request.debug_time(time_);

    test_client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        fullfil_fc_ccids,
        SelectedCreativesCCID(test_client)).check(),
      "unexpected creatives (full freq caps)");

    time_ += ITERATION_DELAY;
  }

  request.debug_time(time_);

  test_client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      nofc_ccids,
      SelectedCreativesCCID(test_client)).check(),
    "unexpected creatives (not full freq caps)");
}

void
FrequencyCapsTest::process_combined_limits_case_(
  AdClient &test_client,
  NSLookupRequest& request,
  const StrVector& cc_id)
{
  // expect that window_limit = 2, life_count = 4, period * 4 < window_time
  const unsigned int period = fetch_int(Data::FC_COMB_PERIOD);
  const unsigned int window_time = fetch_int(Data::FC_COMB_WINDOW_TIME);

  struct ExpectedRequestResult
  {
    unsigned long time;
    bool ad_expected;
  };

  const ExpectedRequestResult ETALON[] = {
    { 0, true },
    { period / 2, false },
    { period, true },
    { period + period / 2, false },
    { 2*period + 1, false }, // window freq caps block it
    { 2*period + 1 + period / 2, false },
    { 3*period + 1, false },
    { 4*period, false },
    { window_time + 1, true },
    { window_time + period + 1, true },
    { 2*window_time, false } // life count start to block all imps
  };

  Generics::Time start_time = time_;

  for(unsigned int i = 0; i < sizeof(ETALON) / sizeof(ETALON[0]); ++i)
  {
    time_ = start_time + ETALON[i].time;
    request.debug_time(time_);

    test_client.process_request(request);

    if(ETALON[i].ad_expected)
    {
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          cc_id,
          SelectedCreativesCCID(test_client)).check(),
        std::string("server must return expected creatives at step #") + strof(i));
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          StrVector(),
          SelectedCreativesCCID(test_client)).check(),
        std::string("server mustn't return creatives at step #") + strof(i));
    }
  }
}

void 
FrequencyCapsTest::print_fcap_ui_(AdClient &test_client)
{
  if (!test_client.debug_info.uid.value().empty())
  {
    FreqCapProfileAdmin admin(
      get_config().
      get_service(CTE_ALL, STE_USER_INFO_MANAGER).
      address.c_str(),
      test_client.debug_info.uid.value().c_str(),
      false,
      AutoTest::UserInfoManager);
    admin.log(AutoTest::Logger::thlog());
  }
  else
  {
    AutoTest::Logger::thlog().stream(Logging::Logger::TRACE, ASPECT) <<
      "Empty freqcaps, because new client haven't profile.";
  }      
}
