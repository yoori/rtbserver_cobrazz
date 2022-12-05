#include <Generics/Rand.hpp>
#include "ChannelTriggerStatsTest.hpp"

REFLECT_UNIT(ChannelTriggerStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
}

template<size_t Count>
void ChannelTriggerStatsTest::add_stats_(
  Stats& stats,
  Diffs& diffs,
  const ChannelTriggerStatsRow (&expected)[Count])
{
  for (size_t i = 0; i < Count; ++i)
  {
    Stat::Key key;
    if (expected[i].colo_id)
    {
      key.colo_id(expected[i].colo_id);
    }
    if (expected[i].trigger_type)
    {
      key.trigger_type(expected[i].trigger_type);
    }
    Stat stat(
      key.
        channel_trigger_id(
          fetch_int(expected[i].channel_trigger_id)).
        sdate(now_));
     
    stat.description("#" + strof(i+1));
    stat.select(pq_conn_);

    stats.push_back(stat);

    diffs.push_back(Diff().hits(expected[i].hits));
  }
}

template<size_t Count>
void ChannelTriggerStatsTest::process_requests_(
  const ChannelTriggerRequest (&requests)[Count])
{
  for (size_t i = 0; i < Count; ++i)
  {
    NSLookupRequest request;
    request.debug_time = now_;
    if (requests[i].colo)
    {
      request.colo = requests[i].colo;
    }
    if (requests[i].tid)
    {
      request.tid = requests[i].tid;
    }
    if (requests[i].referer_kw)
    {
      request.referer_kw = map_objects(requests[i].referer_kw);
    }
    if (requests[i].referer)
    {
      request.referer = map_objects(requests[i].referer);
    }
    if (requests[i].search)
    {
      request.search = map_objects(requests[i].search, " ");
    }
    if (requests[i].ft)
    {
      request.ft = map_objects(requests[i].ft, "\n");
    }

    AdClient client(AdClient::create_user(this));
    client.process_request(request);
    if (requests[i].ccid)
    {
      FAIL_CONTEXT(
        AutoTest::entry_checker(
          strof(requests[i].ccid),
          AutoTest::SelectedCreativesCCID(client)).check(),
        "check expected creative")
    }
  }
}

void
ChannelTriggerStatsTest::set_up()
{ }

bool
ChannelTriggerStatsTest::run()
{
  
  AUTOTEST_CASE(
    no_tid_case_(),
    "Requests without tid param");

  AUTOTEST_CASE(
    with_ad_case_(),
    "Requests with tid param");

  AUTOTEST_CASE(
    url_kwd_(),
    "Separate Page, Search and URL keyword lists");
  
  AUTOTEST_CASE(
    adsc_6348_(),
    "ADSC-6348");
  
  AUTOTEST_CASE(
    adsc_7962_(),
    "ADSC-7962");

  return true;
}

void
ChannelTriggerStatsTest::tear_down()
{ }

void ChannelTriggerStatsTest::no_tid_case_()
{
  Stats stats;
  Diffs diffs;
  
  ChannelTriggerRequest requests[] =
  {
    // colo tid ccid referer_kw referer search ft 
    { 0, 0, 0, "NoTid/PageTrigger", 0, 0, 0 },
    { 0, 0, 0, 0, 0, "NoTid/SearchTrigger", 0 },
    { 0, 0, 0, 0, "NoTid/UrlTrigger", 0, 0 }
  };

  ChannelTriggerStatsRow expected[] =
  {
    // colo_id channel_trigger_id trigger_type hits
    { default_colo_, "NoTid/PageChannelTriggerId", "P", 1 },
    { default_colo_, "NoTid/SearchChannelTriggerId", "S", 1 },
    { default_colo_, "NoTid/UrlChannelTriggerId", "U", 1 },
  };

  add_stats_(stats, diffs, expected);
  process_requests_(requests);

  ADD_WAIT_CHECKER(
    "ChannelTriggerStats check",
    AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats));

}

void ChannelTriggerStatsTest::with_ad_case_()
{
  Stats stats;
  Diffs diffs;

  unsigned long colo_id = fetch_int("WithAd/Colo");
  unsigned long tag_id = fetch_int("WithAd/Tid");
  unsigned long cc_id = fetch_int("WithAd/CC");

  ChannelTriggerRequest requests[] =
  {
    // colo tid ccid referer_kw referer search ft 
    { colo_id, tag_id, cc_id, "WithAd/PageTrigger", 0, 0, 0 },
    { colo_id, tag_id, cc_id, 0, 0, "WithAd/SearchTrigger", 0 },
    { colo_id, tag_id, cc_id, 0, "WithAd/UrlTrigger", 0, 0 }
  };

  ChannelTriggerStatsRow expected[] =
  {
    // colo_id channel_trigger_id trigger_type hits
    { colo_id, "WithAd/PageChannelTriggerId", "P", 1 },
    { colo_id, "WithAd/SearchChannelTriggerId", "S", 1 },
    { colo_id, "WithAd/UrlChannelTriggerId", "U", 1 },
  };

  add_stats_(stats, diffs, expected);
  process_requests_(requests);

  ADD_WAIT_CHECKER(
    "ChannelTriggerStats check",
    AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats));
}

void ChannelTriggerStatsTest::url_kwd_()
{
  Stats stats;
  Diffs diffs;

  // Known bug ADSC-8816: UrlKwdChannelTriggerId not logged
  ChannelTriggerStatsRow expected[] =
  {
    // colo_id channel_trigger_id trigger_type hits
    { default_colo_, "URLKWD/PageChannelTriggerId", "P", 1 },
    { default_colo_, "URLKWD/SearchChannelTriggerId", "S", 1 },
    { default_colo_, "URLKWD/UrlKwdChannelTriggerId", "R", 1 },
    { 0, "URLKWD/PageChannelTriggerId", 0, 1 },
    { 0, "URLKWD/SearchChannelTriggerId", 0, 1 },
    { 0, "URLKWD/UrlKwdChannelTriggerId", 0, 1 }
  };

  add_stats_(stats, diffs, expected);

  AdClient client(AdClient::create_user(this));
  client.process_request(
    NSLookupRequest().
      debug_time(now_).
      referer_kw(
        map_objects(
          "URLKWD/PageTrigger,"
          "URLKWD/SearchTrigger,"
          "URLKWD/UrlKwdTrigger")).
      search(
        map_objects(
          "URLKWD/PageTrigger "
          "URLKWD/SearchTrigger "
          "URLKWD/UrlKwdTrigger",
          " ")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "URLKWD/Channel/P,URLKWD/Channel/S,URLKWD/Channel/R",
      client.debug_info.trigger_channels).check(),
    "Expected trigger_channels check");
  
  ADD_WAIT_CHECKER(
    "ChannelTriggerStats check",
    AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats));  
}

void ChannelTriggerStatsTest::adsc_6348_()
{
  Stats stats;
  Diffs diffs;
  
  ChannelTriggerRequest requests[] =
  {
    // colo tid ccid referer_kw referer search ft      
    { 0, 0, 0, "ADSC-6348/PageTrigger", 0, 0, 0 }
  };

  ChannelTriggerStatsRow expected[] =
  {
    // colo_id channel_trigger_id trigger_type hits
    { default_colo_, "ADSC-6348/PageChannelTriggerId/1", "P", 1 },
    { default_colo_, "ADSC-6348/PageChannelTriggerId/2", "P", 1 },
  };

  add_stats_(stats, diffs, expected);
  process_requests_(requests);

  ADD_WAIT_CHECKER(
    "ChannelTriggerStats check",
    AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats));
}

void ChannelTriggerStatsTest::adsc_7962_()
{
  Stats stats;
  Diffs diffs;
  
  ChannelTriggerRequest requests[] =
  {
    // colo tid ccid referer_kw referer search ft 
    {
      0, 0, 0, 0, 0, "ADSC-7962/KEYWORD",
      "ADSC-7962/KEYWORD\nADSC-7962/TEXT1\nADSC-7962/TEXT2" },
    {
      0, 0, 0, "ADSC-7962/PAGE_KEYWORD,ADSC-7962/SEARCH_KEYWORD",
      0, "ADSC-7962/PAGE_KEYWORD ADSC-7962/SEARCH_KEYWORD", 0
    },
  };

  ChannelTriggerStatsRow expected[] =
  {
    // colo_id channel_trigger_id trigger_type hits
    { default_colo_, "ADSC-7962/KEYWORD/CHANNEL_TRIGGER_ID/P", "P", 1 },
    { default_colo_, "ADSC-7962/TEXT1/CHANNEL_TRIGGER_ID/P", "P", 1 },
    { default_colo_, "ADSC-7962/TEXT2/CHANNEL_TRIGGER_ID/P", "P", 1 },
    { default_colo_, "ADSC-7962/KEYWORD/CHANNEL_TRIGGER_ID/S", "S", 1 },
    { default_colo_, "ADSC-7962/PAGE_KEYWORD/CHANNEL_TRIGGER_ID/P", "P", 1 },
    { default_colo_, "ADSC-7962/PAGE_KEYWORD/CHANNEL_TRIGGER_ID/P", "S", 0 },
    { default_colo_, "ADSC-7962/SEARCH_KEYWORD/CHANNEL_TRIGGER_ID/S", "P", 0 },
    { default_colo_, "ADSC-7962/SEARCH_KEYWORD/CHANNEL_TRIGGER_ID/S", "S", 1 }
  };

  add_stats_(stats, diffs, expected);
  process_requests_(requests);

  ADD_WAIT_CHECKER(
    "ChannelTriggerStats check",
    AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats));
}
