#include "SiteChannelStatsTest.hpp"

REFLECT_UNIT(SiteChannelStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  int iteration_count = 10;
}


bool
SiteChannelStatsTest::run_test()
{
  const int tid1 = fetch_int("TID1");

  const std::string keyword1 = fetch_string("KW1");
  const std::string url1 = fetch_string("URL1");

  const std::string bp1 = fetch_string("BP1");
  const std::string bp2 = fetch_string("BP2");
  const std::string bp31 = fetch_string("BP31");
  const std::string bp32 = fetch_string("BP32");

  const std::string ch1 = fetch_string("CH1");
  const std::string ch2 = fetch_string("CH2");
  const std::string ch3 = fetch_string("CH3");

  const std::string cc1 = fetch_string("CC1");
  const std::string cc2 = fetch_string("CC2");

  AutoTest::DBC::Conn conn(open_pq());

  AutoTest::ORM::StatsArray<AutoTest::ORM::SiteChannelStats, 3> stats;
  stats[0].key().
    tag_id(tid1).
    channel_id(valueof<unsigned int>(ch1));
  stats[1].key().
    tag_id(tid1).
    channel_id(valueof<unsigned int>(ch2));
  stats[2].key().
    tag_id(tid1).
    channel_id(valueof<unsigned int>(ch3));
  stats.select(conn);

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  
  AutoTest::NSLookupRequest request1;
  request1.tid(tid1).referer_kw(keyword1);
  std::string expect1[] =
    {
      bp1, bp31
    };

  AutoTest::NSLookupRequest request2;
  request2.tid(tid1).referer(url1);
  std::string expect2[] =
    {
      bp2, bp32
    };

  AutoTest::NSLookupRequest request3;
  request3.tid(tid1).referer_kw(keyword1).referer(url1);
  std::string expect3[] =
    {
      bp1, bp2, bp31, bp32
    };

  unsigned int clicks1 = 0;
  unsigned int clicks2 = 0;
  unsigned int campaign2 = 0;
  for (int i = 0; i < iteration_count; ++i)
  {
    client.process_request(request1);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expect1,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "BehavioralParams ids");
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        1,
        client.debug_info.selected_creatives.size()).check(),
      "must got 1 creative");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc1,
        client.debug_info.selected_creatives.begin()->ccid).check(),
      "must got expected ccid");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.begin()->click_url.empty()),
      "click_url is empty");
    if (i % 2 == 1)
    {
      ++clicks1;
      client.process_request
        (client.debug_info.selected_creatives.begin()->click_url);
    }

    client.process_request(request2);
    
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expect2,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "BehavioralParams ids");
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        1,
        client.debug_info.selected_creatives.size()).check(),
      "must got 1 creative");
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc2,
        client.debug_info.selected_creatives.begin()->ccid).check(),
      "must got expected ccid");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.begin()->click_url.empty()),
       "click_url is empty");
    if (i % 4 == 3)
    {
      ++clicks2;
      client.process_request
        (client.debug_info.selected_creatives.begin()->click_url);
    }

    client.process_request(request3);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expect3,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "BehavioralParams ids");
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        1,
        client.debug_info.selected_creatives.size()).check(),
      "must got 1 creative");
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.begin()->click_url.empty()),
       "click_url is empty");
    if (client.debug_info.selected_creatives.begin()->ccid == cc2)
    {
      ++campaign2;
    }
  }

  unsigned int imps1 = 20 - campaign2;
  unsigned int imps2 = 10 + campaign2;
  AutoTest::ORM::SiteChannelStats::Diffs diff[] =
  {
    AutoTest::ORM::SiteChannelStats::Diffs().
      imps(imps1).
      adv_revenue((imps1 * 200) / 1000.0 + clicks1 * 20).
      pub_revenue((imps1 * 50) / 1000.0),
    AutoTest::ORM::SiteChannelStats::Diffs().
      imps(imps2).
      adv_revenue((imps2 * 100) / 1000.0 + clicks2 * 10).
      pub_revenue((imps2 * 50) / 1000.0),
    AutoTest::ORM::SiteChannelStats::Diffs(0)
  };
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn, diff, stats)).check(),
    "SiteChannelStats check");

  return true;
}
