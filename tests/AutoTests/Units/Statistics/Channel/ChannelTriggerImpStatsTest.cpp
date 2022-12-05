#include "ChannelTriggerImpStatsTest.hpp"

REFLECT_UNIT(ChannelTriggerImpStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::ORM::stats_value_type stats_value_type;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  struct ExpectedStat
  {
    const char* channel;
    char trigger_type;
    int imps;
    int clicks;
  };
}

class ChannelTriggerImpStatsLapseChecker: public AutoTest::Checker
{
public:
  ChannelTriggerImpStatsLapseChecker(
    DBC::Conn& conn,
    ChannelTriggerStatsArray& initial,
    const TriggerCounterMap& imps_done_map,
    unsigned long imps_lapse,
    unsigned long avg_imps_lapse,
    const TriggerCounterMap& clicks_done_map,
    unsigned long clicks_lapse,
    unsigned long avg_clicks_lapse)
    : conn_(conn),
      initial_(initial),
      result_(initial),
      imps_done_map_(imps_done_map),
      imps_lapse_(imps_lapse),
      avg_imps_lapse_(avg_imps_lapse),
      clicks_done_map_(clicks_done_map),
      clicks_lapse_(clicks_lapse),
      avg_clicks_lapse_(avg_clicks_lapse)
  {}

  virtual ~ChannelTriggerImpStatsLapseChecker() noexcept
  {}

  bool check(bool throw_error = true)
    /*throw(AutoTest::CheckFailed, eh::Exception)*/
  {
    bool error = false;
    std::ostringstream ostr;
    result_.select(conn_);
    stats_value_type sum_imps_lapse = 0;
    stats_value_type sum_clicks_lapse = 0;
    ChannelTriggerStatsArray::iterator tr_it = initial_.begin();
    for(ChannelTriggerStatsArray::iterator result_tr_it =
          result_.begin();
        result_tr_it != result_.end();
        ++result_tr_it, ++tr_it)
    {
      stats_value_type imps_done = 0;
      stats_value_type clicks_done = 0;
      TriggerCounterMap::const_iterator imps_done_it = imps_done_map_.find(
        tr_it->key().channel_trigger_id());
      if(imps_done_it != imps_done_map_.end())
      {
        imps_done = imps_done_it->second;
      }

      TriggerCounterMap::const_iterator clicks_done_it = clicks_done_map_.find(
        tr_it->key().channel_trigger_id());
      if(clicks_done_it != clicks_done_map_.end())
      {
        clicks_done = clicks_done_it->second;
      }

      stats_value_type imps_low_bound =
        tr_it->imps() + imps_done - imps_lapse_;
      stats_value_type imps_up_bound =
        tr_it->imps() + imps_done + imps_lapse_;
      stats_value_type click_low_bound =
        tr_it->clicks() + clicks_done - clicks_lapse_;
      stats_value_type click_up_bound =
        tr_it->clicks() + clicks_done + clicks_lapse_;

      bool local_imps_error = (result_tr_it->imps() < imps_low_bound ||
        result_tr_it->imps() > imps_up_bound);
      bool local_clicks_error = result_tr_it->clicks() < click_low_bound ||
        result_tr_it->clicks() > click_up_bound;

      ostr << std::endl << "channel_trigger_id = " << tr_it->key().channel_trigger_id() <<
        ": "
        "imps = " << result_tr_it->imps() <<
        (local_imps_error ? " not in" : " in") << " [" <<
        result_tr_it->imps() << " + " << (imps_done - imps_lapse_) <<
        ", " <<
        result_tr_it->imps() << " + " << (imps_done + imps_lapse_) <<
        "], "
        "clicks = " << result_tr_it->clicks() <<
        (local_clicks_error ? " not in" : " in") << " [" <<
        result_tr_it->clicks() << " + " << (clicks_done - clicks_lapse_) <<
        ", " <<
        result_tr_it->clicks() << " + " << (clicks_done + clicks_lapse_) <<
        "], " <<
        (local_imps_error || local_clicks_error ? " ***" : "");
      error |= local_imps_error || local_clicks_error;

      sum_imps_lapse += ::fabs(
        result_tr_it->imps() -
        tr_it->imps() - imps_done);
      sum_clicks_lapse += ::fabs(
        result_tr_it->clicks() -
        tr_it->clicks() - clicks_done);
    }

    // check average lapses
    {
      bool local_error = (sum_imps_lapse / initial_.size() > avg_imps_lapse_);
      error |= local_error;
      ostr << std::endl << "average (by triggers) imps lapse = " <<
        (sum_imps_lapse / initial_.size()) << ", limit for expected = " <<
        avg_imps_lapse_ << (local_error ? " ***" : "");
    }

    {
      bool local_error = (sum_clicks_lapse / initial_.size() > avg_clicks_lapse_);
      error |= local_error;
      ostr << std::endl << "average (by triggers) clicks lapse = " <<
        (sum_clicks_lapse / initial_.size()) << ", limit for expected = " <<
        avg_clicks_lapse_ << (local_error ? " ***" : "");
    }

    if (error)
    {
      if (throw_error)
      {
        throw AutoTest::Exception(ostr.str());
      }

      return false;
    }

    return true;
  }

private:
  DBC::Conn& conn_;
  ChannelTriggerStatsArray initial_;
  ChannelTriggerStatsArray result_;
  TriggerCounterMap imps_done_map_;
  unsigned long imps_lapse_;
  unsigned long avg_imps_lapse_;
  TriggerCounterMap clicks_done_map_;
  unsigned long clicks_lapse_;
  unsigned long avg_clicks_lapse_;
};

void
ChannelTriggerImpStatsTest::set_up()
{ }

bool 
ChannelTriggerImpStatsTest::run()
{
  AUTOTEST_CASE(
    do_requests_for_one_trigger_case_(
      "OnlyImpsCase",
      1000,
      0),
    "OnlyImpsCase");
  
  AUTOTEST_CASE(
    do_requests_for_one_trigger_tempo_case_(
      "OnlyImpsCaseTempo",
      1000,
      0),
    "OnlyImpsCaseTempo");

  // Known bugs:
  //   ADSC-8623 (K & D channels in the profile),
  //   ADSC-8826 (R-trigger isn't logged)
  AUTOTEST_CASE(
    channel_types(),
    "Reason of impression (channel types logging)");

  return true;
}

void
ChannelTriggerImpStatsTest::tear_down()
{ }

void
ChannelTriggerImpStatsTest::fetch_stats_(
  ChannelTriggerStatsArray& triggers_stats,
  TriggerCounterMap& imps_done,
  ChannelTriggerSumStats& sum_stats,
  const char* entity_suffix,
  unsigned long impressions_count)
{
  typedef std::set<unsigned long> ChannelTriggerIdSet;

  // collect current stats state
  //   sum stats
  sum_stats[0].key().
    channel_id(
      fetch_int(std::string("ChannelId/") + entity_suffix)).
    sdate(time_);
  sum_stats.select(pq_conn_);

  //   stats by triggers
  ChannelTriggerIdSet match_triggers;
  ChannelTriggerIdSet non_match_triggers;
  fetch_int_sequence(
    std::inserter(match_triggers, match_triggers.begin()),
    std::string("MatchChannelTriggerIds/") + entity_suffix,
    ",");
  fetch_int_sequence(
    std::inserter(non_match_triggers, non_match_triggers.begin()),
    std::string("NonMatchChannelTriggerIds/") + entity_suffix,
    ",");

  for(ChannelTriggerIdSet::iterator tr_it =
        match_triggers.begin();
      tr_it != match_triggers.end(); ++tr_it)
  {
    ChannelTriggerStats one_trigger_stats;
    one_trigger_stats.key().
      channel_trigger_id(*tr_it).
      sdate(time_);
    triggers_stats.push_back(one_trigger_stats);
    imps_done[*tr_it] = impressions_count;
  }

  for(ChannelTriggerIdSet::iterator tr_it =
        non_match_triggers.begin();
      tr_it != non_match_triggers.end(); ++tr_it)
  {
    ChannelTriggerStats one_trigger_stats;
    one_trigger_stats.key().
      channel_trigger_id(*tr_it).
      sdate(time_);
    triggers_stats.push_back(one_trigger_stats);
  }

  triggers_stats.select(pq_conn_);
}

void
ChannelTriggerImpStatsTest::do_requests_for_one_trigger_case_(
  const char* entity_suffix,
  unsigned long impressions_count,
  unsigned long clicks_count)
{

  ChannelTriggerStatsArray triggers_stats;
  TriggerCounterMap imps_done;
  TriggerCounterMap clicks_done;
  ChannelTriggerSumStats sum_stats;
  
  fetch_stats_(
    triggers_stats, imps_done, sum_stats,
    entity_suffix, impressions_count);
  
  // prepare expected data
  std::list<std::string> exp_ccids;
  exp_ccids.push_back(fetch_string(std::string("CcId/") + entity_suffix));

  for(unsigned long i = 0; i < impressions_count; ++i)
  {
    AdClient client(AdClient::create_user(this));

    // one match
    {
      NSLookupRequest ad_request;
      ad_request.referer_kw(fetch_string(std::string("Keyword/") + entity_suffix));
      ad_request.debug_time(time_);
      client.process_request(ad_request);
    }

    // ad impression
    {
      NSLookupRequest ad_request;
      ad_request.tid(fetch_string(std::string("Tid/") + entity_suffix));
      ad_request.debug_time(time_);
      client.process_request(ad_request);

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          exp_ccids,
          SelectedCreativesCCID(client)).check(),
        "unexpected creatives");

      if(i < clicks_count)
      {
        client.process_request(client.debug_info.click_url, "Click request");
      }
    }
  }

  ADD_WAIT_CHECKER(
    "ChannelTriggerStats sum check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ChannelTriggerStats::Diffs().
        imps(impressions_count).
        clicks(clicks_count),
      sum_stats));

  ADD_WAIT_CHECKER(
    "ChannelTriggerStats check",
    ChannelTriggerImpStatsLapseChecker(
      pq_conn_, triggers_stats, imps_done,
      500, 200, clicks_done, 0, 0));
}

void
ChannelTriggerImpStatsTest::do_requests_for_one_trigger_tempo_case_(
  const char* entity_suffix,
  unsigned long impressions_count,
  unsigned long clicks_count)
{

  ChannelTriggerStatsArray triggers_stats;
  TriggerCounterMap imps_done;
  TriggerCounterMap clicks_done;
  ChannelTriggerSumStats sum_stats;

  fetch_stats_(
    triggers_stats, imps_done, sum_stats,
    entity_suffix, impressions_count);
  
  // prepare expected data
  std::list<std::string> exp_ccids;
  exp_ccids.push_back(fetch_string(std::string("CcId/") + entity_suffix));

  for(unsigned long i = 0; i < impressions_count; ++i)
  {
    TemporaryAdClient tclient(TemporaryAdClient::create_user(this));
    AdClient client(AdClient::create_user(this));
    // one match
    {
      NSLookupRequest ad_request;
      ad_request.referer_kw(fetch_string(std::string("Keyword/") + entity_suffix));
      ad_request.debug_time(time_);
      tclient.process_request(ad_request);
      client.merge(tclient, ad_request);
    }
    // ad impression
    {
      NSLookupRequest ad_request;
      ad_request.tid(fetch_string(std::string("Tid/") + entity_suffix));
      ad_request.debug_time(time_);
      client.process_request(ad_request);

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          exp_ccids,
          SelectedCreativesCCID(client)).check(),
        "unexpected creatives");

      if(i < clicks_count)
      {
        client.process_request(client.debug_info.click_url, "Click request");
      }
    }
  }
  
  ADD_WAIT_CHECKER(
    "ChannelTriggerStats sum check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      ChannelTriggerStats::Diffs().
        imps(impressions_count).
        clicks(clicks_count),
      sum_stats));


  ADD_WAIT_CHECKER(
    "ChannelTriggerStats check",
    ChannelTriggerImpStatsLapseChecker(
      pq_conn_, triggers_stats, imps_done,
      500, 200, clicks_done, 0, 0));
}


void
ChannelTriggerImpStatsTest::channel_types()
{
  ChannelTriggerStatsArray stats;
  std::list<ChannelTriggerStats::Diffs> diffs;

  const ExpectedStat EXPECTED[] =
  {
    // Behav channel 1-5
    { "Types/Channel/1", 'P', 1, 1 },
    { "Types/Channel/1", 0, 1, 1 },
    { "Types/Channel/2", 'P', 1, 1 },
    { "Types/Channel/2", 0, 1, 1 },
    { "Types/Channel/3", 'P', 1, 1 },
    { "Types/Channel/3", 0, 1, 1 },
    { "Types/Channel/4", 'P', 1, 1 },
    { "Types/Channel/4", 0, 1, 1 },
    { "Types/Channel/5", 0, 0, 0 },
    // Keyword channels 7,8,9
    { "Types/Channel/6", 0, 0, 0 },
    { "Types/Channel/7", 0, 0, 0 },
    { "Types/Channel/8", 0, 0, 0 },
    // Behav channel 10
    { "Types/Channel/9", 'R', 1, 1 },
    { "Types/Channel/9", 0, 1, 1 },
    // Expression channels
    { "Types/Expression/1", 0, 0, 0 },
    { "Types/Expression/2", 0, 0, 0 }
  };

  for (size_t i = 0; i < countof(EXPECTED); ++i)
  {
    ChannelTriggerStats stat;
    stat.key().
      channel_id(fetch_int(EXPECTED[i].channel)).
      sdate(time_);
    if (EXPECTED[i].trigger_type)
    {
      stat.key().
        trigger_type(std::string(1, EXPECTED[i].trigger_type));
    }
    stat.description("#" + strof(i+1));
    stat.select(pq_conn_);
    stats.push_back(stat);
    diffs.push_back(
      ChannelTriggerStats::Diffs().
        imps(EXPECTED[i].imps).
        clicks(EXPECTED[i].clicks));
  }
    
  
  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().
      debug_time(time_).
      referer_kw(fetch_string("Types/Keyword/6")).
      search(
        map_objects(
          "Types/Keyword/7 Types/Keyword/8 "
          "Types/Keyword/9", " ")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Types/Channel/6/S,"
      "Types/Channel/7/S,Types/Channel/8/S",
      client.debug_info.trigger_channels).check(),
    "Expected trigger_channels#1 check");

  client.process_request(
    NSLookupRequest().
      debug_time(time_).
      tid(fetch_int("Types/TAG")));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(fetch_int("Types/CC/Text")),
      client.debug_info.ccid).check(),
    "Check text CC");

  client.process_request(
    NSLookupRequest().
      debug_time(time_).
      referer_kw(
        map_objects(
          "Types/Keyword/1,Types/Keyword/2,"
          "Types/Keyword/3,Types/Keyword/4,"
          "Types/Keyword/5")).
      search(
        map_objects(
          "Types/Keyword/10,Types/Keyword/5")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Types/Channel/1/P,Types/Channel/2/P,"
      "Types/Channel/3/P,Types/Channel/4/P,"
      "Types/Channel/5/P,Types/Channel/9/R,"
      "Types/Channel/5/R",
      client.debug_info.trigger_channels).check(),
    "Expected trigger_channels#2 check");

  client.process_request(
    NSLookupRequest().
      debug_time(time_).
      tid(fetch_int("Types/TAG")));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      strof(fetch_int("Types/CC/Display")),
      client.debug_info.ccid).check(),
    "Check display CC");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.click_url.empty()),
    "Check display click");

  client.process_request(
    client.debug_info.click_url);

  ADD_WAIT_CHECKER(
    "ChannelTriggerStats check",
     AutoTest::stats_diff_checker(
       pq_conn_, diffs, stats));
}

