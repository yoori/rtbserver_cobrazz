#include "TagsTest.hpp"

REFLECT_UNIT(TagsTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

bool 
TagsTest::run_test()
{
  set_up();

  NOSTOP_FAIL_CONTEXT(empty_country_case());
  NOSTOP_FAIL_CONTEXT(country_ru_case());
  NOSTOP_FAIL_CONTEXT(country_us_case());
  NOSTOP_FAIL_CONTEXT(unknown_country_case());
  NOSTOP_FAIL_CONTEXT(cpm_filter_case());

  return true; 
}


void TagsTest::set_up()
{
  request.referer_kw = fetch_string("TagsTest/01");

  tag_id1 = fetch_string("Tag Id/1");
  tag_id2 = fetch_string("Tag Id/2");
  tag_id3 = fetch_string("Tag Id/3");
  
  siterate_id1    = fetch_string("SiteRate Id/1");
  siterate_id2    = fetch_string("SiteRate Id/2");
  siterate_id_def = fetch_string("SiteRate Id/1/Default");
  siterate_id5    = fetch_string("SiteRate Id/5");

  cc_id1 = fetch_string("CCID/1");
  cc_id2 = fetch_string("CCID/2");
  cc_id3 = fetch_string("CCID/3");
  cc_id4 = fetch_string("CCID/4");
  cc_id5 = fetch_string("CCID/5");
  cc_id6 = fetch_string("CCID/6");
 
}

// May be this case is obsolete
void TagsTest::empty_country_case()
{
  add_descr_phrase("Empty country case.");
  AdClient client(AdClient::create_user(this));
  
  request.loc_name.clear();
  client.process_request(request.tid(tag_id1));

  // In this case country detected by IP.
  // Commonly, server can not detect country by IP -> country is null.
  // There is no CCGs for null-country,
  // server shouldn't return creative.
  // tag_id & site_rate_id is empty.

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id1,
      client.debug_info.tag_id).check(),
    "must got tag_id = Tag Id/1 in response");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must got ccid = 0 in response");
 
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.site_rate_id).check(),
    "must got site_rate_id = 0 in response");
}

void TagsTest::country_ru_case()
{
  add_descr_phrase("'RU' country case.");
  AdClient client(AdClient::create_user(this));

  client.process_request(request.tid(tag_id1).loc_name("ru"));
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id1,
      client.debug_info.tag_id).check(), 
    "must got tag_id = Tag Id/1 in response");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_id3,
      client.debug_info.ccid).check(),
    "must got CCID/3 of RU CCG in response");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      siterate_id1,
      client.debug_info.site_rate_id).check(), 
    "must got site_rate_id = SiteRate Id/1 in response");
}


void TagsTest::country_us_case()
{
  add_descr_phrase("'US' country case.");
  AdClient client(AdClient::create_user(this));

  client.process_request(request.tid(tag_id3).loc_name("us"));
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id3,
      client.debug_info.tag_id).check(),
    "must got tag_id = Tag Id/3 in response");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_id2,
      client.debug_info.ccid).check(),
    "must got CCID/2 of US CCG in response");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      siterate_id5,
      client.debug_info.site_rate_id).check(),
    "must got site_rate_id = SiteRate Id/5 in response");
}

void TagsTest::unknown_country_case()
{
  add_descr_phrase("Unknown country case.");
  AdClient client(AdClient::create_user(this));

  client.process_request(request.tid(tag_id1).loc_name("kr"));
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id1,
      client.debug_info.tag_id).check(), 
    "must got tag_id = Tag Id/1 in response");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_id5,
      client.debug_info.ccid).check(),
    "must got CCID/5 of KR CCG in response");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      siterate_id_def,
      client.debug_info.site_rate_id).check(),
    "must got site_rate_id = SiteRate Id/1/Default in response");
}

void TagsTest::cpm_filter_case()
{
  add_descr_phrase("CPM filter case.");
  AdClient client(AdClient::create_user(this));

  client.process_request(request.tid(tag_id2).loc_name("ru"));
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id2,
      client.debug_info.tag_id).check(),
    "must got tag_id = 0 in response");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "must got ccid = 0 in response");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.site_rate_id).check(),
    "must site_rate_id = 0 in response");
}
