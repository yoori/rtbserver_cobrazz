#include <math.h>
#include <algorithm>
#include <functional>
#include "CCGKeywordStatsTest.hpp"

REFLECT_UNIT(CCGKeywordStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::DebugInfo::SelectedCreativesList SelectedCreativesList;

  const unsigned short REPEAT_COUNT = 10;
  struct ExpectedStat
  {
    unsigned long ccg_keyword_id;
    AutoTest::Time sdate;
    AutoTest::Time adv_sdate;
    unsigned long flags;
  };
}

void 
CCGKeywordStatsTest::set_up()
{ }

bool 
CCGKeywordStatsTest::run()
{
  NOSTOP_FAIL_CONTEXT(system_currency_case_());
  NOSTOP_FAIL_CONTEXT(advertiser_currency_case_());
  NOSTOP_FAIL_CONTEXT(mixed_currency_case_());
  NOSTOP_FAIL_CONTEXT(advertiser_timezone_case_());

  // Check stats
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_, diffs_, hourly_)).check(),
    "Check CCGKeywordStatsHourly");

  {
    std::list<ORM::CCGKeywordStatsDaily::Diffs> diffs(diffs_.size());
    std::copy(diffs_.begin(), diffs_.end(), diffs.begin());
    /*
    std::transform(
      diffs_.begin(), diffs_.end(), diffs.begin(),
      ConvertDiff<ORM::CCGKeywordStatsDaily::Diffs>());
    */
    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_, diffs, daily_)).check(),
      "Check CCGKeywordStatsDaily");
  }

  {
    std::list<ORM::CCGKeywordStatsToW::Diffs> diffs(diffs_.size());
    std::copy(diffs_.begin(), diffs_.end(), diffs.begin());
    /*
    std::transform(
      diffs_.begin(), diffs_.end(), diffs.begin(),
      ConvertDiff<ORM::CCGKeywordStatsToW::Diffs>());
    */
    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::stats_diff_checker(
          pq_conn_, diffs, week_)).check(),
      "Check CCGKeywordStatsToW");
  }

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_, total_diffs_, total_)).check(),
    "Check CCGKeywordStatsTotal");

  return true;
}

void 
CCGKeywordStatsTest::tear_down()
{ }

void 
CCGKeywordStatsTest::add_stats_(
  const std::string& description,
  const ORM::CCGKeywordStatsHourly::Key& key,
  const ORM::CCGKeywordStatsHourly::Diffs& diff,
  unsigned long flags)
{
  diffs_.push_back(diff);

  current_total_ += diff;
  {
    ORM::CCGKeywordStatsHourly stat(key);
    stat.description(description);
    stat.select(pq_conn_);
    hourly_.push_back(stat);
  }

  {
    ORM::CCGKeywordStatsDaily stat;
    stat.key().
      ccg_keyword_id(key.ccg_keyword_id()).
      adv_sdate(
        key.adv_sdate_used()? key.adv_sdate(): key.sdate());
    stat.description(description);
    stat.select(pq_conn_);
    daily_.push_back(stat);
  }

  if ( flags & SO_DUMP_TOTAL )
  {
    ORM::CCGKeywordStatsTotal stat;
    stat.key().ccg_keyword_id(key.ccg_keyword_id());
    stat.description(description);
    stat.select(pq_conn_);
    total_.push_back(stat);

    total_diffs_.push_back(current_total_);
    current_total_ = ORM::CCGKeywordStatsTotal::Diffs(0);
  }

  {
    ORM::CCGKeywordStatsToW stat;
    stat.key().
      day_of_week(key.sdate().get_gm_time().tm_wday + 1).
      hour(key.sdate().get_gm_time().tm_hour).
      ccg_keyword_id(key.ccg_keyword_id());
    stat.description(description);
    stat.select(pq_conn_);
    week_.push_back(stat);
  }
}

void 
CCGKeywordStatsTest::system_currency_case_()
{
  const char ENTITY_SUFFIX[] = "SystemCurrency";

  std::string description("System currency.");
  add_descr_phrase(description);

  Generics::Time now = Generics::Time::get_time_of_day();
  double revenue = fetch_float(std::string("ClickAmount1/") + ENTITY_SUFFIX);

  //expected data
  std::list<std::string> ccid_exp;
  ccid_exp.push_back(fetch_string(std::string("CC1/") + ENTITY_SUFFIX));

  // database

  add_stats_(
    description,
    ORM::CCGKeywordStatsHourly::Key().
      ccg_keyword_id(
        fetch_int(std::string("CCGKeywordId1/") + ENTITY_SUFFIX)).
      sdate(now),
    ORM::CCGKeywordStatsHourly::Diffs().
      imps(REPEAT_COUNT).
      clicks(REPEAT_COUNT / 2).
      adv_amount((REPEAT_COUNT / 2) * revenue));

  AdClient client = AdClient::create_user(this);
  AutoTest::NSLookupRequest request; 
  request.referer_kw(fetch_string(std::string("Keyword1/") + ENTITY_SUFFIX));
  request.tid(fetch_int(std::string("Tid/") + ENTITY_SUFFIX));
  request.debug_time(now);

  client.process_request(request);

  for (unsigned long i = 0; i < REPEAT_COUNT; i++)
  {
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        ccid_exp,
        SelectedCreativesCCID(client)).check(),
      description + " Check CC");

    // Send click request after every second impression
    if (i % 2 == 1)
    {
      client.process_request(
        client.debug_info.selected_creatives.begin()->click_url);
    }
  }
}

void 
CCGKeywordStatsTest::advertiser_currency_case_()
{
  const char ENTITY_SUFFIX[] = "AdvCurrency";

  std::string description("Advertiser currency(non system).");
  add_descr_phrase(description);

  Generics::Time now = Generics::Time::get_time_of_day();
  double revenue = fetch_float(std::string("ClickAmount1/") + ENTITY_SUFFIX);
  

  //expected data
  std::list<std::string> ccid_exp;
  ccid_exp.push_back(fetch_string(std::string("CC1/") + ENTITY_SUFFIX));
  

  // database
  add_stats_(
    description,
    ORM::CCGKeywordStatsHourly::Key().
      ccg_keyword_id(
        fetch_int(std::string("CCGKeywordId1/") + ENTITY_SUFFIX)).
      sdate(now),
    ORM::CCGKeywordStatsHourly::Diffs().
      imps(REPEAT_COUNT).
      clicks(REPEAT_COUNT / 2).
      adv_amount((REPEAT_COUNT / 2) * revenue));

  AdClient client = AdClient::create_user(this);
  AutoTest::NSLookupRequest request; 
  request.referer_kw(fetch_string(std::string("Keyword1/") + ENTITY_SUFFIX));
  request.tid(fetch_string(std::string("Tid/") + ENTITY_SUFFIX));
  request.debug_time(now);

  client.process_request(request);

  for (unsigned long i = 0; i < REPEAT_COUNT; i++)
  {
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        ccid_exp,
        SelectedCreativesCCID(client)).check(),
      description + " Check CC");

    // Send click request after every second impression
    if (i % 2 == 1)
    {
      client.process_request(
        client.debug_info.selected_creatives.begin()->click_url);
    }
  }
}

void
CCGKeywordStatsTest::mixed_currency_case_()
{
  const char ENTITY_SUFFIX[] = "MixedCurrency";

  std::string description("Mixed currency.");
  add_descr_phrase(description);

  Generics::Time now = Generics::Time::get_time_of_day();

  // expected data, campaign in decrease actual_cpc order
  std::list<std::string> ccid_exp;
  ccid_exp.push_back(fetch_string(std::string("CC1/") + ENTITY_SUFFIX));
  ccid_exp.push_back(fetch_string(std::string("CC2/") + ENTITY_SUFFIX));

  double revenue1 = fetch_float(std::string("ClickAmount1/") + ENTITY_SUFFIX);
  double revenue2 = fetch_float(std::string("ClickAmount2/") + ENTITY_SUFFIX);

  // database
  add_stats_(
    description,
    ORM::CCGKeywordStatsHourly::Key().
      ccg_keyword_id(
        fetch_int(std::string("CCGKeywordId1/") + ENTITY_SUFFIX)).
      sdate(now),
    ORM::CCGKeywordStatsHourly::Diffs().
      imps(REPEAT_COUNT).
      clicks(REPEAT_COUNT / 2).
      adv_amount((REPEAT_COUNT / 2) * revenue1 ));

  add_stats_(
    description,
    ORM::CCGKeywordStatsHourly::Key().
      ccg_keyword_id(
        fetch_int(std::string("CCGKeywordId2/") + ENTITY_SUFFIX)).
      sdate(now),
    ORM::CCGKeywordStatsHourly::Diffs().
      imps(REPEAT_COUNT).
      clicks(REPEAT_COUNT / 4).
      adv_amount((REPEAT_COUNT / 4) * revenue2 ));

  const std::string keyword1 = std::string("Keyword1/") + ENTITY_SUFFIX;
  const std::string keyword2 = std::string("Keyword2/") + ENTITY_SUFFIX;

  AdClient client = AdClient::create_user(this);
  AutoTest::NSLookupRequest request; 
  request.referer_kw(
    fetch_string(std::string("Keyword1/") + ENTITY_SUFFIX) + "," +
    fetch_string(std::string("Keyword2/") + ENTITY_SUFFIX));
  request.tid(fetch_int(std::string("Tid/") + ENTITY_SUFFIX));
  request.debug_time(now);
  client.process_request(request);

  for (unsigned long i = 0; i < REPEAT_COUNT; i++)
  {
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        ccid_exp,
        SelectedCreativesCCID(client)).check(),
      description + " Check CC");

    AutoTest::DebugInfo::DebugInfo debug_info = client.debug_info;
    SelectedCreativesList::const_iterator creative = debug_info.selected_creatives.begin();
    if (i % 2 == 1)
    {
      // campaign#1 - 1 click for 2 impressions
      client.process_request(creative->click_url);
    }
    ++creative;
    if (i % 4 == 3)
    {
      // campaign#2 - 1 click for 4 impressions
      client.process_request(creative->click_url);
    }
  }

}

void
CCGKeywordStatsTest::advertiser_timezone_case_()
{
  const char ENTITY_SUFFIX[] = "AdvTimezone";

  std::string description("Advertiser timezone.");
  add_descr_phrase(description);

  const Generics::Time ADV1_TZ_OFFSET =
    AutoTest::ORM::get_tz_ofset(this, "Europe/Copenhagen");
  const Generics::Time ADV2_TZ_OFFSET =
    AutoTest::ORM::get_tz_ofset(this, "America/Sao_Paulo");
  
  AutoTest::Time now;
  AutoTest::Time now_date(now.get_gm_time().get_date());

  // request times in GMT:
  AutoTest::Time adv1_time1 = now_date - ADV1_TZ_OFFSET - 60;
  AutoTest::Time adv1_time2 = now_date - ADV1_TZ_OFFSET + 60;
  AutoTest::Time adv2_time1 = now_date - ADV2_TZ_OFFSET - 60;
  AutoTest::Time adv2_time2 = now_date - ADV2_TZ_OFFSET + 60;

  // check dates in advertiser timezones (adv_sdate):
  unsigned long adv1_ccg_keyword_id = fetch_int(
    std::string("CCGKeywordId1/") + ENTITY_SUFFIX); 
  unsigned long adv2_ccg_keyword_id = fetch_int(
    std::string("CCGKeywordId2/") + ENTITY_SUFFIX);

  const ExpectedStat STATS[] =
  {
    {
      adv1_ccg_keyword_id,
      adv1_time1,
      now_date - 60,
      0
    },
    {
      adv1_ccg_keyword_id,
      adv1_time2,
      now_date + 60,
      SO_DUMP_TOTAL
    },
    {
      adv2_ccg_keyword_id,
      adv2_time1,
      now_date - 60,
      0
    },
    {
      adv2_ccg_keyword_id,
      adv2_time2,
      now_date + 60,
      SO_DUMP_TOTAL
    }
  };

  for (size_t i = 0; i < countof(STATS); ++i)
  {
    add_stats_(
      description,
      ORM::CCGKeywordStatsHourly::Key().
        ccg_keyword_id(STATS[i].ccg_keyword_id).
      sdate(STATS[i].sdate).
      adv_sdate(STATS[i].adv_sdate),
    ORM::CCGKeywordStatsHourly::Diffs().
      imps(1).
      clicks(0).
      adv_amount(0),
      STATS[i].flags);
  }

  {
    AdClient client = AdClient::create_user(this);
    AutoTest::NSLookupRequest request; 
    request.referer_kw(fetch_string(std::string("Keyword1/") + ENTITY_SUFFIX));
    client.process_request(request); // REVIEW !!! actual ?

    request.debug_time(adv1_time1);
    request.tid(fetch_string(std::string("Tid/") + ENTITY_SUFFIX));
    client.process_request(request);

    request.debug_time(adv1_time2);
    client.process_request(request);

    std::list<std::string> ccid_exp;
    ccid_exp.push_back(fetch_string(std::string("CC1/") + ENTITY_SUFFIX));

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        ccid_exp,
        SelectedCreativesCCID(client)).check(),
      description + " Check CC#1");
  }
  {
    AdClient client = AdClient::create_user(this);
    AutoTest::NSLookupRequest request; 
    request.referer_kw(fetch_string(std::string("Keyword2/") + ENTITY_SUFFIX));
    client.process_request(request);

    request.debug_time(adv2_time1);
    request.tid(fetch_string(std::string("Tid/") + ENTITY_SUFFIX));
    client.process_request(request);

    request.debug_time(adv2_time2);
    client.process_request(request);

    std::list<std::string> ccid_exp;
    ccid_exp.push_back(fetch_string(std::string("CC2/") + ENTITY_SUFFIX));

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        ccid_exp,
        SelectedCreativesCCID(client)).check(),
      description + " Check CC#2");
  }

} 
