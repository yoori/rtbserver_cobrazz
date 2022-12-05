#include "PageLoadsDailyStats.hpp"

REFLECT_UNIT(PageLoadsDailyStats) (
  "Statistics",
  AUTO_TEST_SLOW);


namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
}

bool
PageLoadsDailyStats::run()
{
  debug_time  = Generics::Time::get_time_of_day();

  NOSTOP_FAIL_CONTEXT(case15_user_statuses());
  NOSTOP_FAIL_CONTEXT(case01_multiple_tags_in_one_domain());
  NOSTOP_FAIL_CONTEXT(case02_multiple_tags_iframe_eq_page());
  NOSTOP_FAIL_CONTEXT(case03_multiple_tags_iframe_noteq_page());
  NOSTOP_FAIL_CONTEXT(case04_multiple_tags_iframes());
  NOSTOP_FAIL_CONTEXT(case05_merging_without_page_id());
  NOSTOP_FAIL_CONTEXT(case06_merging_on_the_same_tag());
  NOSTOP_FAIL_CONTEXT(case07_different_page_id_equal_referrer_time());
  NOSTOP_FAIL_CONTEXT(case08_equal_referrers_time_exceeds_2seconds());
  NOSTOP_FAIL_CONTEXT(case09_no_referrer_and_page_id());
  NOSTOP_FAIL_CONTEXT(case10_unconfirmed_impressions());
  NOSTOP_FAIL_CONTEXT(case11_different_sites());
  NOSTOP_FAIL_CONTEXT(case12_different_countries());
  NOSTOP_FAIL_CONTEXT(case13_inventory_mode_tags());
  AdClient client(AdClient::create_user(this));
  NOSTOP_FAIL_CONTEXT(
    case14_reverse_logs_delivery_order_part_1(client));

  add_descr_phrase("PageLoadsDaily check#1.");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        stats)).check(),
    "PageLoadsDailyStats: check stats");

  stats.clear();
  diffs.clear();

  NOSTOP_FAIL_CONTEXT(
    case14_reverse_logs_delivery_order_part_2(client));

  add_descr_phrase("PageLoadsDaily check#2.");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_,
        diffs,
        stats)).check(),
    "PageLoadsDailyStats: check stats");


  return true;
}

std::string
PageLoadsDailyStats::fetch_tag_group(
  const char* tags)
{
  return
    "|" + map_objects(tags, "|") + "|";
}

template <size_t Count>
void
PageLoadsDailyStats::initialize_stats(
  const std::string& description,
  const TagGroupExpected (&expected)[Count])
{
  for (size_t i = 0; i < Count; ++i)
  {
    ORM::PageLoadsDaily::Key key;
    key.
      site_id(fetch_int(expected[i].site)).
      country_code("GN").
      colo_id(1).
      country_sdate(debug_time);

    if (expected[i].tag_group)
    {
      key.
        tag_group(
          fetch_tag_group(expected[i].tag_group));
    }
    
    ORM::PageLoadsDaily stat(key);

    stat.description(description + " #" + strof(i+1));
    stat.select(pq_conn_);
    stats.push_back(stat);

    diffs.push_back(
      ORM::PageLoadsDaily::Diffs().
        page_loads(expected[i].page_loads).
        utilized_page_loads(
          expected[i].utilized_page_loads));
  }
}

void
PageLoadsDailyStats::case01_multiple_tags_in_one_domain()
{
  std::string description("Multiple tags in one domain.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string pl = "9152101";
  std::string tag1 = fetch_string("TAG01_1");
  std::string tag2 = fetch_string("TAG01_2");
  std::string keyword1 = fetch_string("KEYWORD01_1");
  std::string keyword2 = fetch_string("KEYWORD01_2");

  ORM::PageLoadsDaily stat(
    ORM::PageLoadsDaily::Key().
      site_id(fetch_int("SITE01")).
      country_code("GN").
      colo_id(1).
      tag_group(fetch_tag_group("TAG01_1|TAG01_2")).
      country_sdate(debug_time));
  stat.select(pq_conn_);
  stat.description(description);
  stats.push_back(stat);

  diffs.push_back(
    ORM::PageLoadsDaily::Diffs().
      page_loads(1).
      utilized_page_loads(0));
  
  NSLookupRequest request;
  request.debug_time = debug_time;
  request.referer = "www.pageloadsdailystats1.com";
  request.pl = pl;

  request.referer_kw = keyword1;
  request.tid = tag1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case01 1");

  request.referer_kw = keyword2;
  request.tid = tag2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case01 2");
}

void
PageLoadsDailyStats::case02_multiple_tags_iframe_eq_page()
{
  std::string description("Multiple tags iframe = page.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string pl = "9152102";
  std::string tag1 = fetch_string("TAG02_1");
  std::string tag2 = fetch_string("TAG02_2");
  std::string tag3 = fetch_string("TAG02_3");
  std::string keyword1 = fetch_string("KEYWORD02_1");
  std::string keyword2 = fetch_string("KEYWORD02_2");
  std::string keyword3 = fetch_string("KEYWORD02_3");

  ORM::PageLoadsDaily stat(
    ORM::PageLoadsDaily::Key().
      site_id(fetch_int("SITE02")).
      country_code("GN").
      colo_id(1).
      tag_group(
        fetch_tag_group(
          "TAG02_1|TAG02_2|TAG02_3")).
      country_sdate(debug_time));
  stat.select(pq_conn_);
  stat.description(description);
  stats.push_back(stat);

  diffs.push_back(
    ORM::PageLoadsDaily::Diffs().
      page_loads(1).
      utilized_page_loads(1));

  NSLookupRequest request;
  request.debug_time = debug_time;
  request.pl = pl;
  request.referer = "www.pageloadsdailystats2.com";

  request.tid = tag1;
  request.referer_kw = keyword1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC02_1"),
      client.debug_info.ccid).check(),
    "must select expected creative case02 1");

  request.tid = tag2;
  request.referer_kw = keyword2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case02 2");

  request.tid = tag3;
  request.referer_kw = keyword3;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case02 3");
}

void
PageLoadsDailyStats::case03_multiple_tags_iframe_noteq_page()
{
  std::string description("Multiple tags iframe noteq page.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string tag1 = fetch_string("TAG03_1");
  std::string tag2 = fetch_string("TAG03_2");
  std::string tag3 = fetch_string("TAG03_3");
  std::string keyword1 = fetch_string("KEYWORD03_1");
  std::string keyword2 = fetch_string("KEYWORD03_2");
  std::string keyword3 = fetch_string("KEYWORD03_3");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE03", "TAG03_1", 1, 1 },
    { "SITE03", "TAG03_2", 1, 0 },
    { "SITE03", "TAG03_3", 1, 0 }
  };

  initialize_stats(description, EXPECTED);
 
  NSLookupRequest request;
  request.debug_time = debug_time;

  request.referer = "www.pageloadsdailystats3_1.com";
  request.tid = tag1;
  request.referer_kw = keyword1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC03_1"),
      client.debug_info.ccid).check(),
    "must select expected creative case03 1");

  request.referer = "www.pageloadsdailystats3_2.com";
  request.tid = tag2;
  request.referer_kw = keyword2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case03 2");

  request.referer = "www.pageloadsdailystats3_3.com";
  request.tid = tag3;
  request.referer_kw = keyword3;
  request.pl = "1384247382";
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case03 3");
}

void
PageLoadsDailyStats::case04_multiple_tags_iframes()
{
  std::string description("Multiple tags iframes.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string pl3 = "91521043";
  std::string tag1 = fetch_string("TAG04_1");
  std::string tag2 = fetch_string("TAG04_2");
  std::string tag3 = fetch_string("TAG04_3");
  std::string keyword1 = fetch_string("KEYWORD04_1");
  std::string keyword2 = fetch_string("KEYWORD04_2");
  std::string keyword3 = fetch_string("KEYWORD04_3");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE04", "TAG04_1", 1, 1 },
    { "SITE04", "TAG04_2", 1, 0 },
    { "SITE04", "TAG04_3", 1, 0 }
  };

  initialize_stats(description, EXPECTED);
 
  NSLookupRequest request;
  request.debug_time = debug_time;
  
  request.referer = "www.pageloadsdailystats4_1.com";
  request.tid = tag1;
  request.referer_kw = keyword1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC04_1"),
      client.debug_info.ccid).check(),
    "must select expected creative case04 1");

  request.referer = "www.pageloadsdailystats4_2.com";
  request.tid = tag2;
  request.referer_kw = keyword2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case04 2");

  request.pl = pl3;
  request.referer = "www.pageloadsdailystats4_3.com";
  request.tid = tag3;
  request.referer_kw = keyword3;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case04 3");
}

void
PageLoadsDailyStats::case05_merging_without_page_id()
{
  std::string description("Merging without page id.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string tag1 = fetch_string("TAG05_1");
  std::string tag2 = fetch_string("TAG05_2");
  std::string tag3 = fetch_string("TAG05_3");
  std::string tag4 = fetch_string("TAG05_4");
  std::string tag5 = fetch_string("TAG05_5");
  std::string tag6 = fetch_string("TAG05_6");
  std::string keyword1 = fetch_string("KEYWORD05_1");
  std::string keyword2 = fetch_string("KEYWORD05_2");
  std::string keyword3 = fetch_string("KEYWORD05_3");
  std::string keyword4 = fetch_string("KEYWORD05_4");
  std::string keyword5 = fetch_string("KEYWORD05_5");
  std::string keyword6 = fetch_string("KEYWORD05_6");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE05", "TAG05_1|TAG05_2", 1, 1 },
    { "SITE05", "TAG05_3", 1, 1 },
    { "SITE05", "TAG05_4", 1, 1 },
    { "SITE05", "TAG05_5", 1, 1 },
    { "SITE05", "TAG05_6", 1, 0 }
  };

  initialize_stats(description, EXPECTED);

  NSLookupRequest request;
  request.debug_time = debug_time;

  request.referer = "www.pageloadsdailystats5.com";
  request.tid = tag1;
  request.referer_kw = keyword1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC05_1"),
      client.debug_info.ccid).check(),
    "must select expected creative case05 1");

  request.referer = "www.pageloadsdailystats5.com";
  request.tid = tag2;
  request.referer_kw = keyword2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case05 2");

  request.referer = "www.pageloadsdailystats5.com/path1";
  request.tid = tag3;
  request.referer_kw = keyword3;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC05_3"),
      client.debug_info.ccid).check(),
    "must select expected creative case05 3");

  request.referer = "www.pageloadsdailystats5.com/path2";
  request.tid = tag4;
  request.referer_kw = keyword4;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC05_4"),
      client.debug_info.ccid).check(),
    "must select expected creative case05 4");

  request.referer = "www.pageloadsdailystats5.com/path3?query1";
  request.tid = tag5;
  request.referer_kw = keyword5;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC05_5"),
      client.debug_info.ccid).check(),
    "must select expected creative case05 5");

  request.referer = "www.pageloadsdailystats5.com/path3?query2";
  request.tid = tag6;
  request.referer_kw = keyword6;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case05 6");
}

void
PageLoadsDailyStats::case06_merging_on_the_same_tag()
{
  std::string description("Merging on the same tag.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string pl = "9152106";

  std::string tag1 = fetch_string("TAG06_1");
  std::string tag2 = fetch_string("TAG06_2");
  std::string keyword1 = fetch_string("KEYWORD06_1");
  std::string keyword2 = fetch_string("KEYWORD06_2");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE06", "TAG06_1|TAG06_1|TAG06_2|TAG06_2", 1, 1 }
  };

  initialize_stats(description, EXPECTED);

  NSLookupRequest request1, request2;

  request1.debug_time = debug_time;
  request1.referer = "www.pageloadsdailystats6.com";
  request1.tid = tag1;
  
  request2.debug_time = debug_time;
  request2.referer = "www.pageloadsdailystats6.com";
  request2.pl = pl;
  request2.tid = tag2;

  client.process_request(request1);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case06 1");

  client.process_request(request2);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case06 2");

  request1.referer_kw = keyword1;
  request2.referer_kw = keyword2;

  client.process_request(request1);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC06_1"),
      client.debug_info.ccid).check(),
    "must select expected creative case06 3");

  client.process_request(request2);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC06_2"),
      client.debug_info.ccid).check(),
    "must select expected creative case06 4");
}

void
PageLoadsDailyStats::case07_different_page_id_equal_referrer_time()
{
  std::string description("Different page id equal referrer time.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string pl1 = "91521071";
  std::string pl2 = "91521072";
  std::string tag1 = fetch_string("TAG07_1");
  std::string tag2 = fetch_string("TAG07_2");
  std::string keyword1 = fetch_string("KEYWORD07_1");
  std::string keyword2 = fetch_string("KEYWORD07_2");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE07", "TAG07_1", 1, 1 },
    { "SITE07", "TAG07_2", 1, 0 }
  };

  initialize_stats(description, EXPECTED);

  NSLookupRequest request;
  request.debug_time = debug_time;
  request.referer = "www.pageloadsdailystats7.com";
  request.pl = pl1;
  request.tid = tag1;
  request.referer_kw = keyword1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC07_1"),
      client.debug_info.ccid).check(),
    "must select expected creative case07 1");

  request.pl = pl2;
  request.tid = tag2;
  request.referer_kw = keyword2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case07 2");
}

void
PageLoadsDailyStats::case08_equal_referrers_time_exceeds_2seconds()
{
  std::string description("Equal referrers time exceeds 2 seconds.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string tag1 = fetch_string("TAG08_1");
  std::string tag2 = fetch_string("TAG08_2");
  std::string keyword1 = fetch_string("KEYWORD08_1");
  std::string keyword2 = fetch_string("KEYWORD08_2");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE08", "TAG08_1", 1, 1 },
    { "SITE08", "TAG08_2", 1, 0 }
  };

  initialize_stats(description, EXPECTED);

  Generics::ExtendedTime ext_now = debug_time.get_gm_time();
  ext_now.tm_hour = 13;
  Generics::Time now(ext_now);
  Generics::Time next = now + Generics::Time::ONE_SECOND * 2;
  NSLookupRequest request;

  request.referer = "www.pageloadsdailystats8.com";
  request.debug_time = now;
  request.tid = tag1;
  request.referer_kw = keyword1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC08_1"),
      client.debug_info.ccid).check(),
    "must select expected creative case08 1");

  request.debug_time = next;
  request.tid = tag2;
  request.referer_kw = keyword2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case08 2");
}

void
PageLoadsDailyStats::case09_no_referrer_and_page_id()
{
  std::string description("No referrer and page id.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string tag1 = fetch_string("TAG09_1");
  std::string tag2 = fetch_string("TAG09_2");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE09", "TAG09_1", 1, 0 },
    { "SITE09", "TAG09_2", 1, 0 }
  };

  initialize_stats(description, EXPECTED);

  NSLookupRequest request;
  request.debug_time = debug_time;

  request.tid = tag1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case09 1");

  request.tid = tag2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case09 2");
}

void
PageLoadsDailyStats::case10_unconfirmed_impressions()
{
  std::string description("Unconfirmed impressions.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string tag = fetch_string("TAG10_1");
  std::string keyword = fetch_string("KEYWORD10_1");

  ORM::PageLoadsDaily stat(
    ORM::PageLoadsDaily::Key().
      site_id(fetch_int("SITE10")).
      country_code("GN").
      colo_id(1).
      tag_group(fetch_tag_group("TAG10_1")).
      country_sdate(debug_time));
  stat.select(pq_conn_);
  stat.description(description);
  stats.push_back(stat);

  diffs.push_back(
    ORM::PageLoadsDaily::Diffs().
      page_loads(1).
      utilized_page_loads(1));

  NSLookupRequest request;
  request.debug_time = debug_time;
  request.referer_kw = keyword;
  request.tid = tag;
  request.format = "unit-test-imp";
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC10_1"),
      client.debug_info.ccid).check(),
    "must select expected creative case10 1");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.track_pixel_url.empty()),
    "must have track pixel url");
}

void
PageLoadsDailyStats::case11_different_sites()
{
  std::string description("Different sites.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string pl = "9152111";

  std::string tag1 = fetch_string("TAG11a_1");
  std::string tag2 = fetch_string("TAG11a_2");
  std::string tag3 = fetch_string("TAG11b_1");
  std::string tag4 = fetch_string("TAG11b_2");
  std::string keyword1 = fetch_string("KEYWORD11a_1");
  std::string keyword2 = fetch_string("KEYWORD11a_2");
  std::string keyword3 = fetch_string("KEYWORD11b_1");
  std::string keyword4 = fetch_string("KEYWORD11b_2");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE11a", "TAG11a_1|TAG11a_2", 1, 0 },
    { "SITE11b", "TAG11b_1|TAG11b_2", 1, 1 }
  };

  initialize_stats(description, EXPECTED);

  NSLookupRequest request;
  request.debug_time = debug_time;
  request.referer = "www.pageloadsdailystats11.com";
  request.pl = pl;
    
  request.referer_kw = keyword1;
  request.tid = tag1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case11a 1");

  request.referer_kw = keyword2;
  request.tid = tag2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case11a 2");

  request.referer_kw = keyword3;
  request.tid = tag3;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case11b 1");

  request.referer_kw = keyword4;
  request.tid = tag4;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC11b_2"),
      client.debug_info.ccid).check(),
    "must select expected creative case11b 2");
}

void
PageLoadsDailyStats::case12_different_countries()
{
  std::string description("Different countries.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));

  std::string tag1 = fetch_string("TAG12_1");
  std::string tag2 = fetch_string("TAG12_2");
  std::string keyword1 = fetch_string("KEYWORD12_1");
  std::string keyword2 = fetch_string("KEYWORD12_2");

    {
    ORM::PageLoadsDaily stat(
      ORM::PageLoadsDaily::Key().
        site_id(fetch_int("SITE12")).
        country_code("GB").
        colo_id(1).
        tag_group(fetch_tag_group("TAG12_1")).
        country_sdate(debug_time));
    stat.select(pq_conn_);
    stat.description(description + " #1");
    stats.push_back(stat);

    diffs.push_back(
      ORM::PageLoadsDaily::Diffs().
        page_loads(1).
        utilized_page_loads(0));
  }

  {
    ORM::PageLoadsDaily stat(
      ORM::PageLoadsDaily::Key().
        site_id(fetch_int("SITE12")).
        country_code("LU").
        colo_id(1).
        tag_group(fetch_tag_group("TAG12_2")).
        country_sdate(debug_time));
    stat.select(pq_conn_);
    stat.description(description + " #2");
    stats.push_back(stat);

    diffs.push_back(
      ORM::PageLoadsDaily::Diffs().
        page_loads(1).
        utilized_page_loads(1));
  }


  NSLookupRequest request;
  request.debug_time = debug_time;

  request.pl = "1384249123";
  request.referer_kw = keyword1;
  request.tid = tag1;
  request.loc_name = "gb";
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must select expected creative case12 1");

  request.referer_kw = keyword2;
  request.tid = tag2;
  request.loc_name = "lu";
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CC12_2"),
      client.debug_info.ccid).check(),
    "must select expected creative case12 2");
}

void
PageLoadsDailyStats::case13_inventory_mode_tags()
{
  std::string description("Inventory mode tags.");
  add_descr_phrase(description);

  AdClient client(AdClient::create_user(this));
  
  std::string pl = "9152113";
  std::string tag1 = fetch_string("TAG13_1");
  std::string tag2 = fetch_string("TAG13_2");
  std::string tag3 = fetch_string("TAG13_INV");

  ORM::PageLoadsDaily stat(
    ORM::PageLoadsDaily::Key().
      site_id(fetch_int("SITE13")).
      country_code("GN").
      colo_id(1).
      tag_group(
        fetch_tag_group(
          "TAG13_1|TAG13_2")).
      country_sdate(debug_time));
  stat.select(pq_conn_);
  stat.description(description);
  stats.push_back(stat);

  diffs.push_back(
    ORM::PageLoadsDaily::Diffs().
      page_loads(1).
      utilized_page_loads(0));


  NSLookupRequest request;
  request.debug_time = debug_time;
  request.referer = "www.pageloadsdailystats13.com";
  request.pl = pl;

  request.tid = tag1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check empty cc#1");

  request.tid = tag2;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check empty cc#2");

  request.tid = tag3;
  request.tag_inv = 1;
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check empty cc#3");
}

void
PageLoadsDailyStats::case14_reverse_logs_delivery_order_part_1(
  AdClient& client)
{
  std::string description("Reverse logs delivery order (part#1).");
  add_descr_phrase(description);

  std::string pl = "9152114";
  std::string tag1 = fetch_string("TAG14a_1");
  std::string tag2 = fetch_string("TAG14b_1");
  std::string keyword1 = fetch_string("KEYWORD14a_1");
  std::string keyword2 = fetch_string("KEYWORD14b_1");

  Generics::ExtendedTime ext_now = debug_time.get_gm_time();
  ext_now.tm_hour = 13;
  AutoTest::Time now(ext_now);

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE14a", "TAG14a_1", 1, 1 },
    { "SITE14b", "TAG14b_1", 1, 0 }
  };

    
  initialize_stats(description, EXPECTED);

  {
    NSLookupRequest request;
    request.referer_kw = keyword1;
    request.referer = "www.pageloadsdailystats14.com";
    request.pl = pl;
    request.tid = tag1;
    request.debug_time = now;
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("CC14a_1"),
        client.debug_info.ccid).check(),
      "must select expected creative case14a 1");
  }
  {
    NSLookupRequest request;
    request.referer_kw = keyword2;
    request.referer = "www.pageloadsdailystats14.com";
    request.tid = tag2;
    request.debug_time = now;
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "must select expected creative case14b 1");
  }
}

void
PageLoadsDailyStats::case14_reverse_logs_delivery_order_part_2(
  AdClient& client)
{
  std::string description("Reverse logs delivery order (part#2).");
  add_descr_phrase(description);


  std::string pl = "9152114";
  std::string tag3 = fetch_string("TAG14a_2");
  std::string tag4 = fetch_string("TAG14b_2");
  std::string keyword3 = fetch_string("KEYWORD14a_2");
  std::string keyword4 = fetch_string("KEYWORD14b_2");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE14a", "TAG14a_1", -1, -1 },
    { "SITE14a", "TAG14a_2", 0, 0 },
    { "SITE14a", "TAG14a_1|TAG14a_2", 1, 1 },
    { "SITE14b", "TAG14b_1", -1, 0 },
    { "SITE14b", "TAG14b_2", 0, 0 },
    { "SITE14b", "TAG14b_1|TAG14b_2", 1, 0 }
  };

  initialize_stats(description, EXPECTED);

  Generics::ExtendedTime ext_now = debug_time.get_gm_time();
  ext_now.tm_hour = 13;
  AutoTest::Time prev(ext_now);
  prev -= Generics::Time::ONE_SECOND;

  {
    NSLookupRequest request;
    request.referer_kw = keyword3;
    request.referer = "www.pageloadsdailystats14.com";
    request.pl = pl;
    request.tid = tag3;
    request.debug_time = prev;
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "must select expected creative case14a 2");
  }
  {
    NSLookupRequest request;
    request.referer_kw = keyword4;
    request.referer = "www.pageloadsdailystats14.com";
    request.tid = tag4;
    request.debug_time = prev;
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "must select expected creative case14b 2");
  }
  
}

void
PageLoadsDailyStats::case15_user_statuses()
{
  std::string description("User statuses.");
  add_descr_phrase(description);

  std::string tag1 = fetch_string("TAG15_1");
  std::string tag2 = fetch_string("TAG15_2");
  std::string tag3 = fetch_string("TAG15_3");

  const TagGroupExpected EXPECTED[] =
  {
    { "SITE15", 0, 0, 0 }
  };

  initialize_stats(description, EXPECTED);

  NSLookupRequest request;
  request.debug_time = debug_time;

  {
    std::string pl = "915211501";
    AdClient client(AdClient::create_nonoptin_user(this));
    request.referer = "www.pageloadsdailystats15_1.com";
    request.pl = pl;
    
    request.tid = tag1;
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("CC15_1"),
        client.debug_info.ccid).check(),
      "must select expected creative case15 1");

  }
  {
    std::string pl = "915211502";
    AdClient client(AdClient::create_optout_user(this));
    request.referer = "www.pageloadsdailystats15_2.com";
    request.pl = pl;
    request.tid = tag2;
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "must select expected creative case15 3");

    request.tid = tag3;
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "must select expected creative case15 4");
  }
  {
    std::string pl = "915211503";
    AdClient client(AdClient::create_user(this));
    client.set_probe_uid();
    request.referer = "www.pageloadsdailystats15_3.com";
    request.pl = pl;
    request.tid = tag2;
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        "0",
        client.debug_info.ccid).check(),
      "must select expected creative case15 3");

  }
}
