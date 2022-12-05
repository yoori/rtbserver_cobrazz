#include "ChannelTriggerPerformanceTest.hpp"

REFLECT_UNIT(ChannelTriggerPerformanceTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::UserTriggerMatchProfileEmptyChecker
    UserTriggerMatchProfileChecker;

  const int MINUTE = 60;
  const int HOUR = 60 * MINUTE;
  const int DAY = 24 * HOUR;

  const char PROFILE_NOT_FOUND[] = "Profile.*not found.";
}

template<size_t Count>
void ChannelTriggerPerformanceTest::add_stats(
  const std::string& prefix,
  const StatsRow (&stats)[Count],
  Stats& container,
  Diffs& diffs)
{
  for (size_t i = 0; i < Count; ++i)
  {
    Stat::Key key;
    key.sdate(stats[i].sdate).
        colo_id(stats[i].colo_id).
        channel_id(
          fetch_int(prefix + "/" + stats[i].channel_id + "/" + "CHANNEL_ID"));
    if (stats[i].trigger_type)
    { key.trigger_type(stats[i].trigger_type); }
    if (stats[i].channel_trigger_id)
    { 
      key.channel_trigger_id(
        fetch_int(prefix + "/" + stats[i].channel_id +
          "/TRIGGERS/" + stats[i].channel_trigger_id));
    }
    Stat stat(key);
    stat.description(prefix);
    stat.select(conn_);

    container.push_back(stat);

    diffs.push_back(Diff().imps(stats[i].imps).clicks(stats[i].clicks));
  }
}

void ChannelTriggerPerformanceTest::reason_of_impression_()
{
  const std::string description(
    "Test 2.1. Reason of impression (channel types logging)");
  add_descr_phrase(description);

  const StatsRow CASE_STATS[] = {
//    sdate   colo_id       channel_id        trigger_type  channel_trigger_id  imps  clicks
    { today_, default_colo_, "Channel1",           0,                0,            1,    1 },
    { today_, default_colo_, "Channel2",           0,                0,            1,    1 },
    { today_, default_colo_, "Channel3",           0,                0,            1,    1 },
    { today_, default_colo_, "Expression",         0,                0,            0,    0 },
    { today_, default_colo_, "ChannelWithoutCCG",  0,                0,            0,    0 },
    { today_, default_colo_, "EChannel",           0,                0,            0,    0 },
    { today_, default_colo_, "KeywordChannel2",    0,                0,            0,    0 },
    { today_, default_colo_, "KeywordChannel1",    0,                0,            0,    0 },
    { today_, default_colo_, "KeywordChannel3",    0,                0,            0,    0 },
    { today_, default_colo_, "ChannelCMP",         0,                0,            1,    1 },
    { today_, default_colo_, "Channel1",           "P",              0,            1,    1 },
    { today_, default_colo_, "Channel2",           "P",              0,            1,    1 },
    { today_, default_colo_, "Channel3",           "P",              0,            1,    1 },
    { today_, default_colo_, "ChannelCMP",         "P",              0,            1,    1 },
  };

  add_stats("Test#2.1", CASE_STATS, stats_, diffs_);

  AdClient client(AdClient::create_user(this));

  client.process_request(NSLookupRequest().
    search(
      fetch_string("Test#2.1/KeywordChannel1/KEYWORDS/KKeyword1") + " " +
      fetch_string("Test#2.1/KeywordChannel2/KEYWORDS/KKeyword2") + " " +
      fetch_string("Test#2.1/KeywordChannel3/KEYWORDS/KKeyword3")).
    debug_time(today_));

  {
    std::string expected[] = {
      fetch_string("Test#2.1/KeywordChannel1/SEARCH_KEY"),
      fetch_string("Test#2.1/KeywordChannel2/SEARCH_KEY"),
      fetch_string("Test#2.1/KeywordChannel3/SEARCH_KEY")
    };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "checking expected channels in adserver response");
  }

  client.process_request(
    NSLookupRequest().tid(default_tag_).debug_time(today_));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Test#2.1/TextCampaign/CCID"),
      client.debug_info.ccid).check(),
    "checking expected text creative");

  client.process_request(NSLookupRequest().
    referer_kw(
      fetch_string("Test#2.1/Channel1/KEYWORDS/Keyword1_9") + "," +
      fetch_string("Test#2.1/Channel2/KEYWORDS/Keyword2_1") + "," +
      fetch_string("Test#2.1/Channel3/KEYWORDS/Keyword3_1") + "," +
      fetch_string("Test#2.1/ChannelWithoutCCG/KEYWORDS/Keyword4_1") + "," +
      fetch_string("Test#2.1/ChannelCMP/KEYWORDS/KeywordCMP_10")).
    debug_time(today_));

  {
    std::string expected[] = {
      fetch_string("Test#2.1/Channel1/PAGE_KEY"),
      fetch_string("Test#2.1/Channel2/PAGE_KEY"),
      fetch_string("Test#2.1/Channel3/PAGE_KEY"),
      fetch_string("Test#2.1/ChannelWithoutCCG/PAGE_KEY"),
      fetch_string("Test#2.1/ChannelCMP/PAGE_KEY")
    };

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "checking expected channels in adserver response");
  }

  client.process_request(
    NSLookupRequest().tid(default_tag_).debug_time(today_));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Test#2.1/DisplayCampaign/CCID"),
      client.debug_info.ccid).check(),
    "checking expected display creative");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.selected_creatives.first().click_url.empty()),
    "server must return not empty click_url");

  client.process_request(
    client.debug_info.selected_creatives.first().click_url);  

}

void ChannelTriggerPerformanceTest::behavioral_params_restrictions_()
{
  const std::string description("Test 2.2. Behavioral params restrictions");
  add_descr_phrase(description);

  AutoTest::Time tomorrow = today_ + DAY + 1;
  std::string cc1 = fetch_string("Test#2.2/Campaign1/CCID");
  std::string cc2 = fetch_string("Test#2.2/Campaign2/CCID");
  std::string cc3 = fetch_string("Test#2.2/Campaign3/CCID");
  unsigned long tag = fetch_int("TAG/300x250");

  const StatsRow CASE_STATS[] = {
//    sdate     colo_id       channel_id  trigger_type  channel_trigger_id  imps        clicks
    { today_,   default_colo_, "Channel1",   0,                0,            1,          0 },
    { today_,   default_colo_, "Channel2",   0,                0,            2,          1 },
    { today_,   default_colo_, "Channel3",   0,                0,            1,          0 },
    { tomorrow, default_colo_, "Channel3",   0,                0,            1,          0 },
    { today_,   default_colo_, "Channel1",   "P",              0,            0.33333332, 0 },
    { today_,   default_colo_, "Channel1",   "U",              0,            0.66666667, 0 },
    { today_,   default_colo_, "Channel2",   "P",              0,            0.5,        0.5 },
    { today_,   default_colo_, "Channel2",   "S",              0,            1.5,        0.5 },
    { today_,   default_colo_, "Channel3",   "P",              0,            0.33333332, 0 },
    { today_,   default_colo_, "Channel3",   "S",              0,            0.33333332, 0 },
    { today_,   default_colo_, "Channel3",   "U",              0,            0.33333332, 0 },
    { tomorrow, default_colo_, "Channel3",   "S",              0,            0.5,        0 },
    { tomorrow, default_colo_, "Channel3",   "U",              0,            0.5,        0 },
  };

  add_stats("Test#2.2", CASE_STATS, stats_, diffs_);

  AdClient client(AdClient::create_user(this));

  client.process_request(NSLookupRequest().
    referer_kw(fetch_string("Test#2.2/Channel1/KEYWORDS/Keyword1_1")).
    referer(fetch_string("Test#2.2/Channel1/URLS/URL1_1")).
    debug_time(today_));

  {
    std::string expected[] = {
      fetch_string("Test#2.2/Channel1/PAGE_KEY"),
      fetch_string("Test#2.2/Channel1/URL_KEY")
    };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "checking expected channels in adserver response");
  }

  client.process_request(NSLookupRequest().
    referer(fetch_string("Test#2.2/Channel1/URLS/URL1_1")).
    debug_time(today_));

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      fetch_string("Test#2.2/Channel1/URL_KEY"),
      client.debug_info.trigger_channels).check(),
    "checking expected channels in adserver response");

  client.process_request(
    NSLookupRequest().tid(default_tag_).debug_time(today_));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc1,
      client.debug_info.ccid).check(),
    "checking expected display creative");

  for (size_t i = 0; i < 2; ++i)
  {
    client.process_request(NSLookupRequest().
      referer_kw(fetch_string("Test#2.2/Channel2/KEYWORDS/Keyword2_1")).
      search(fetch_string("Test#2.2/Channel2/KEYWORDS/Keyword2_2")).
      debug_time(today_));

    std::string expected[] = {
      fetch_string("Test#2.2/Channel2/PAGE_KEY"),
      fetch_string("Test#2.2/Channel2/SEARCH_KEY")
    };


    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "checking expected channels in adserver response");
  }

  client.process_request(
    NSLookupRequest().tid(default_tag_).debug_time(today_));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc2,
      client.debug_info.ccid).check(),
    "checking expected display creative");

  std::string click_url =
    client.debug_info.selected_creatives.first().click_url;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !click_url.empty()),
    "server must return click_url");

  for (size_t i = 0; i < 2; ++i)
  {
    client.process_request(NSLookupRequest().
      referer_kw(fetch_string("Test#2.2/Channel3/KEYWORDS/Keyword3_1")).
      search(fetch_string("Test#2.2/Channel3/KEYWORDS/Keyword3_2")).
      debug_time(today_));

    std::string expected[] = {
      fetch_string("Test#2.2/Channel3/PAGE_KEY"),
      fetch_string("Test#2.2/Channel3/SEARCH_KEY")
    };


    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "checking expected channels in adserver response");
  }

  for (size_t i = 0; i < 2; ++i)
  {
    client.process_request(NSLookupRequest().
      referer(fetch_string("Test#2.2/Channel3/URLS/URL3_5")).
      debug_time(today_));

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("Test#2.2/Channel3/URL_KEY"),
        client.debug_info.trigger_channels).check(),
      "checking expected channels in adserver response");
  }

  client.process_request(NSLookupRequest().tid(tag).debug_time(today_));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc3,
      client.debug_info.ccid).check(),
    "checking expected display creative");

  client.process_request(
    NSLookupRequest().tid(default_tag_).debug_time(today_ + 5 * MINUTE + 1));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc2,
      client.debug_info.ccid).check(),
    "checking expected display creative");

  client.process_request(click_url);

  client.process_request(
    NSLookupRequest().tid(tag).debug_time(tomorrow));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc3,
      client.debug_info.ccid).check(),
    "checking expected display creative");
}

void ChannelTriggerPerformanceTest::trigger_status_restrictions_()
{
  const std::string description("Test 2.3. Trigger status restrictions");
  add_descr_phrase(description);

  const StatsRow CASE_STATS[] = {
//    sdate     colo_id       channel_id  trigger_type  channel_trigger_id  imps        clicks
    { today_,   default_colo_, "Channel",   "P",         0,                  0.5,          0 },
    { today_,   default_colo_, "Channel",   "U",         0,                  0.5,          0 },
    { today_,   default_colo_, "Channel",   "P",         "NegativeKeyword",  0,            0 },
    { today_,   default_colo_, "Channel",   "P",         "Keyword4",         0,            0 },
    { today_,   default_colo_, "Channel",   "P",         "Keyword5",         0,            0 },
    { today_,   default_colo_, "Channel",   "U",         "NegativeURL",      0,            0 },
    { today_,   default_colo_, "Channel",   "U",         "URL4",             0,            0 },
    { today_,   default_colo_, "Channel",   "U",         "URL5",             0,            0 },
  };

  add_stats("Test#2.3", CASE_STATS, stats_, diffs_);

  AdClient client(AdClient::create_user(this));

  client.process_request(NSLookupRequest().
    referer_kw(
      fetch_string("Test#2.3/Channel/KEYWORDS/Keyword1") + "," +
      fetch_string("Test#2.3/Channel/KEYWORDS/Keyword2")).
    referer(fetch_string("Test#2.3/Channel/URLS/URL1")).
    debug_time(today_));

  std::string expected[] = {
    fetch_string("Test#2.3/Channel/PAGE_KEY"),
    fetch_string("Test#2.3/Channel/URL_KEY")
  };


  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "checking expected channels in adserver response");

  client.process_request(
    NSLookupRequest().tid(default_tag_).debug_time(today_));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Test#2.3/Campaign/CCID"),
      client.debug_info.ccid).check(),
    "checking expected display creative");
}

void ChannelTriggerPerformanceTest::channels_linked_with_text_campaign_()
{
  const std::string description(
    "Test 2.4. Channels linked with text campaigns");
  add_descr_phrase(description);

  const unsigned long colo = fetch_int("NonDefaultColo");

  const StatsRow CASE_STATS[] = {
//    sdate     colo_id         channel_id  trigger_type  channel_trigger_id  imps  clicks
    { today_,   default_colo_,  "Channel1",   0,            0,                  2,    2 },
    { today_,   colo,           "Channel1",   0,            0,                  0,    0 },
    { today_,   colo,           "Channel2",   0,            0,                  0,    0 },
    { today_,   default_colo_,  "Channel1",   "P",          0,                  2,    2 }
  };

  add_stats("Test#2.4", CASE_STATS, stats_, diffs_);

  AdClient client(AdClient::create_user(this));

  client.process_request(NSLookupRequest().
    referer_kw(
      fetch_string("Test#2.4/Channel1/KEYWORDS/Keyword1_1") + "," +
      fetch_string("Test#2.4/Channel1/KEYWORDS/CommonKeyword")).
    colo(colo).
    debug_time(today_));

  std::string channels[] = {
    fetch_string("Test#2.4/Channel1/PAGE_KEY"),
    fetch_string("Test#2.4/Channel2/PAGE_KEY")
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      channels,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "checking expected channels in adserver response");

  client.process_request(NSLookupRequest().tid(default_tag_).
    colo(colo).format("unit-test-imp").debug_time(today_));

  std::string creatives[] = {
    fetch_string("Test#2.4/CTTCampaign1/CCID"),
    fetch_string("Test#2.4/CTTCampaign2/CCID")
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      creatives,
      AutoTest::SelectedCreativesCCID(client)).check(),
    "checking expected text creatives");

  std::string click_url1 =
    client.debug_info.selected_creatives.first().click_url;
  std::string click_url2 =
    client.debug_info.selected_creatives.last().click_url;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.track_pixel_url.empty()),
    "server must return non empty track_pixel_url");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !click_url1.empty() && !click_url2.empty()),
    "server must return non empty click_url for both creatives");

  client.process_request(client.debug_info.track_pixel_url);
  client.process_request(click_url1);
  client.process_request(click_url2);
}

void ChannelTriggerPerformanceTest::last_visits_for_history_channels_()
{
  const std::string description("Test 2.5. Last visits for history channels");
  add_descr_phrase(description);

  const AutoTest::Time tomorrow = today_ + DAY;

  const StatsRow CASE_STATS[] = {
//    sdate     colo_id         channel_id  trigger_type  channel_trigger_id  imps  clicks
    { tomorrow, default_colo_,  "Channel",  "U",            0,                0.5,    0 },
    { tomorrow, default_colo_,  "Channel",  "P",            0,                0.5,    0 },
  };

  add_stats("Test#2.5", CASE_STATS, stats_, diffs_);

  AdClient client(AdClient::create_user(this));

  client.process_request(NSLookupRequest().
    referer_kw(fetch_string("Test#2.5/Channel/KEYWORDS/Keyword1")).
    referer(fetch_string("Test#2.5/Channel/URLS/URL1")).
    debug_time(today_.get_gm_time().format("%d-%m-%Y:") + "01-02-00"));

  std::string channels[] = {
    fetch_string("Test#2.5/Channel/PAGE_KEY"),
    fetch_string("Test#2.5/Channel/URL_KEY")
  };

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      channels,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "checking expected channels in adserver response");

  client.process_request(NSLookupRequest().tid(default_tag_).
    debug_time(tomorrow.get_gm_time().format("%d-%m-%Y:") + "01-01-00"));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("Test#2.5/Campaign/CCID"),
      client.debug_info.ccid).check(),
    "checking expected display creative");
}

void ChannelTriggerPerformanceTest::trigger_performance_for_temp_users_()
{
  const std::string description(
    "Test 2.6. Trigger performance for temporary users");
  add_descr_phrase(description);

  const StatsRow CASE_STATS[] = {
//    sdate     colo_id         channel_id  trigger_type  channel_trigger_id  imps  clicks
    { today_, default_colo_,  "Channel1",   0,              0,                1,    1 },
    { today_, default_colo_,  "Channel1",   "P",            0,                1,    1 },
    { today_, default_colo_,  "Channel2",   "U",            0,                1,    0 },
  };

  add_stats("Test#2.6", CASE_STATS, stats_, diffs_);

  add_descr_phrase("Test 2.6.1. Regular merging with temporary user");
  {
    TemporaryAdClient temp_client(TemporaryAdClient::create_user(this));

    temp_client.process_request(NSLookupRequest().
      referer_kw(fetch_string("Test#2.6/Channel1/KEYWORDS/Keyword1_9")).
      debug_time(today_));

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("Test#2.6/Channel1/PAGE_KEY"),
        temp_client.debug_info.trigger_channels).check(),
      "checking expected channels in adserver response");

    AdClient client(AdClient::create_user(this));

    client.merge(temp_client,
      NSLookupRequest().tid(default_tag_).debug_time(today_));

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("Test#2.6/Campaign/CCID"),
        client.debug_info.ccid).check(),
      "checking expected display creative");

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.first().click_url.empty()),
      "server must return not empty click_url");

    client.process_request(
      client.debug_info.selected_creatives.first().click_url);
  }

  add_descr_phrase("Test 2.6.2. Temporary profile expired on merging");
  {
    TemporaryAdClient temp_client(TemporaryAdClient::create_user(this));

    temp_client.process_request(NSLookupRequest());

    AdClient client(AdClient::create_user(this));

    client.process_request(NSLookupRequest().
      referer(fetch_string("Test#2.6/Channel2/URLS/URL2_1")).
      debug_time(today_));

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("Test#2.6/Channel2/URL_KEY"),
        client.debug_info.trigger_channels).check(),
      "checking expected channels in adserver response");

    // assuming that UserTriggerMatch profile for temp uid has expired
    client.merge(temp_client,
      NSLookupRequest().tid(default_tag_).debug_time(today_));

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("Test#2.6/Campaign/CCID"),
        client.debug_info.ccid).check(),
      "checking expected display creative");
  }
}

void ChannelTriggerPerformanceTest::asynchronous_logging_()
{
  const std::string description("Test 2.7. Asynchronous logging");
  add_descr_phrase(description);

  const StatsRow IMTERMEDIATE_STATS[] = {
//    sdate     colo_id         channel_id  trigger_type  channel_trigger_id  imps        clicks
    { today_, default_colo_,  "Channel1",   0,              0,                1,            0 },
    { today_, default_colo_,  "Channel1",   "U",            0,                1,            0 },
  };

  Stats intermediate_stats;
  Diffs intermediate_diffs;

  add_stats("Test#2.7", IMTERMEDIATE_STATS,
    intermediate_stats, intermediate_diffs);

  add_descr_phrase("Test 2.7.1. Asynchronous logging for persistent users");
  {
    AdClient client(AdClient::create_user(this));

    client.process_request(NSLookupRequest().
      referer(fetch_string("Test#2.7/Channel1/URLS/URL1_1")).
      debug_time(today_));

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("Test#2.7/Channel1/URL_KEY"),
        client.debug_info.trigger_channels).check(),
      "checking expected channels in adserver response");

    client.process_request(NSLookupRequest().tid(default_tag_).
      debug_time(today_ + 1));
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("Test#2.7/Campaign/CCID"),
        client.debug_info.ccid).check(),
      "checking expected display creative");

    // Intermediate diffs check
    FAIL_CONTEXT(AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          conn_, intermediate_diffs, intermediate_stats)).check(),
      "ChannelTriggerStats intermediate check");

    const StatsRow CASE_STATS[] = {
//    sdate     colo_id         channel_id  trigger_type  channel_trigger_id  imps        clicks
      { today_, default_colo_,  "Channel1",   0,              0,                0,            0 },
      { today_, default_colo_,  "Channel1",   "P",            0,                0.33333332,   0 },
      { today_, default_colo_,  "Channel1",   "U",            0,                -0.33333332,  0 },
      { today_, default_colo_,  "Channel2",   0,              0,                1,            0 },
      { today_, default_colo_,  "Channel2",   "P",            0,                0.33333332,   0 },
      { today_, default_colo_,  "Channel2",   "U",            0,                0.66666668,   0 }
    };

    add_stats("Test#2.7", CASE_STATS, stats_, diffs_);

    client.process_request(NSLookupRequest().
      referer_kw(fetch_string("Test#2.7/Channel1/KEYWORDS/Keyword1_1")).
      debug_time(today_ - 1));

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("Test#2.7/Channel1/PAGE_KEY"),
        client.debug_info.trigger_channels).check(),
      "checking expected channels in adserver response");
  }

  add_descr_phrase("Test 2.7.2. Asynchronous logging "
    "on merging with temporary users");
  {
    TemporaryAdClient temp_client(TemporaryAdClient::create_user(this));

    temp_client.process_request(NSLookupRequest().
      referer(fetch_string("Test#2.7/Channel2/URLS/URL2_1")).
      debug_time(today_));

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("Test#2.7/Channel2/URL_KEY"),
        temp_client.debug_info.trigger_channels).check(),
      "checking expected channels in adserver response");

    AdClient client(AdClient::create_user(this));

    client.merge(temp_client, NSLookupRequest().tid(default_tag_).
      debug_time(today_ + 1));

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("Test#2.7/Campaign/CCID"),
        client.debug_info.ccid).check(),
      "checking expected display creative");

    temp_client.process_request(NSLookupRequest().
      referer_kw(fetch_string("Test#2.7/Channel2/KEYWORDS/Keyword2_1")).
      debug_time(today_ - 1));

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("Test#2.7/Channel2/PAGE_KEY"),
        temp_client.debug_info.trigger_channels).check(),
      "checking expected channels in adserver response");
  }
}

bool 
ChannelTriggerPerformanceTest::run_test()
{
  add_descr_phrase("Run");

  reason_of_impression_();
  behavioral_params_restrictions_();
  trigger_status_restrictions_();
  channels_linked_with_text_campaign_();
  last_visits_for_history_channels_();
  trigger_performance_for_temp_users_();
  asynchronous_logging_();

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_, diffs_, stats_)).check(),
    "ChannelTriggerStats check");

  return true;
}
