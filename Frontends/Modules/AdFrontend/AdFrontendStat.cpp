/// @file AdFrontend/AdFrontendStat.cpp
#include "AdFrontendStat.hpp"

namespace
{
  const char DEFINED_TID[] = "adRequest-Count";
  const char AD_OPT_IN_COUNT[] = "adRequestWithImpression-OptInUser-Count";
  const char AD_OPT_OUT_COUNT[] = "adRequestWithImpression-nonOptInUser-Count";
  const char AD_OPT_IN_PASSBACK[] = "adRequestWithPassback-OptInUser-Count";
  const char AD_OPT_OUT_PASSBACK[] = "adRequestWithPassback-nonOptInUser-Count";
  const char OPT_OUT_COUNT[] = "campaignSelect-OptOutUser-Count";
  const char OPT_IN_COUNT[] = "campaignSelect-OptInUser-Count";
  const char PROBE_COUNT[] =  "campaignSelect-ProbeUser-Count";
  const char TEMPORARY_COUNT[] = "campaignSelect-TemporaryUser-Count";
  const char UNDEFINED_COUNT[] = "campaignSelect-UndefinedUser-Count";
  const char PASSBACK_COUNT[] = "passbackRequest-Count";
  const char TRIGGER_RECIVED[] = "profilingRequest-ReceivedTriggers-Count";
  const char TRIGGER_MATCHED[] =  "profilingRequest-MatchedTriggers-Count";
  const char CHANNELS_MATCHED[] = "profilingRequest-TriggeredChannels-Count";
  const char PROFILING_REQUEST[] = "profilingRequest-Count";

  // time stats
  const std::string USER_INFO_MATCH_TIME = "historyMatch";
  const std::string USER_INFO_MERGE_TIME = "usersMerge";
  const std::string TRIGGER_MATCH_TIME = "triggersMatch";
  const std::string REQUEST_FILLING_TIME = "requestParsing";
  const std::string CREATIVE_SELECTION_TIME = "creativeSelection";
  const std::string CREATIVE_SELECTION_LOCAL_TIME = "creativeSelectionLocal";

  const char AD_REQUEST_TOTAL[] = "ad_request_total";
  const char AD_REQUEST_IN_PROGRESS[] = "ad_request_in_progress";

  const char AD_REQUEST_TRIGGER_MATCH_TOTAL[] = "ad_request_trigger_match_total";
  const char AD_REQUEST_TRIGGER_MATCH_IN_PROGRESS[] = "ad_request_trigger_match_in_progress";
  const char AD_REQUEST_TRIGGER_MATCH_ERROR_TOTAL[] = "ad_request_trigger_match_error_total";

  const char AD_REQUEST_HISTORY_MATCH_TOTAL[] = "ad_request_history_match_total";
  const char AD_REQUEST_HISTORY_MATCH_TOTAL_TIME[] = "ad_request_history_match_total_time";
  const char AD_REQUEST_HISTORY_MATCH_IN_PROGRESS[] = "ad_request_history_match_in_progress";
  const char AD_REQUEST_HISTORY_MATCH_ERROR_TOTAL[] = "ad_request_history_match_error_total";

  const char AD_REQUEST_CAMPAIGN_SELECTION_TOTAL[] = "ad_request_campaign_selection_total";
  const char AD_REQUEST_CAMPAIGN_SELECTION_TOTAL_TIME[] =
    "ad_request_campaign_selection_total_time";
  const char AD_REQUEST_CAMPAIGN_SELECTION_IN_PROGRESS[] =
    "ad_request_campaign_selection_in_progress";
  const char AD_REQUEST_CAMPAIGN_SELECTION_ERROR_TOTAL[] =
    "ad_request_campaign_selection_error_total";

  const char AD_REQUEST_HISTORY_POST_MATCH_TOTAL[] = "ad_request_history_post_match_total";
  const char AD_REQUEST_HISTORY_POST_MATCH_TOTAL_TIME[] =
    "ad_request_history_post_match_total_time";
  const char AD_REQUEST_HISTORY_POST_MATCH_IN_PROGRESS[] =
    "ad_request_history_post_match_in_progress";
  const char AD_REQUEST_HISTORY_POST_MATCH_ERROR_TOTAL[] =
    "ad_request_history_post_match_error_total";

  void
  add_stage_stats(
    Generics::Values& values,
    const AdServer::AdFrontendStat::StageStat& stage,
    const char* total_key,
    const char* total_time_key,
    const char* in_progress_key,
    const char* error_total_key)
  {
    values.set(total_key, stage.total);
    if (total_time_key)
    {
      values.set(total_time_key, stage.total_time.as_double());
    }
    values.set(in_progress_key, stage.in_progress);
    values.set(error_total_key, stage.error_total);
  }
}

namespace Aspect
{
  extern const char AD_FRONTEND[];
}

namespace AdServer
{
  AdFrontendStat::BaseStatData::BaseStatData()
    : adRequest(0),
      adRequestWithImpressionOptInUser(0),
      adRequestWithImpressionnonOptInUser(0),
      adRequestWithPassbackOptInUser(0),
      adRequestWithPassbacknonOptInUser(0),
      campaignSelectOptOutUser(0),
      campaignSelectOptInUser(0),
      campaignSelectProbeUser(0),
      campaignSelectTemporaryUser(0),
      campaignSelectUndefinedUser(0),
      passbackRequest(0),
      profilingRequestReceivedTriggers(0),
      profilingRequestMatchedTriggers(0),
      profilingRequestTriggeredChannels(0),
      profilingRequest(0) {}

  AdFrontendStat::BaseStatData::BaseStatData(
    unsigned long adRequest_,
    unsigned long adRequestWithImpressionOptInUser_,
    unsigned long adRequestWithImpressionnonOptInUser_,
    unsigned long adRequestWithPassbackOptInUser_,
    unsigned long adRequestWithPassbacknonOptInUser_,
    unsigned long campaignSelectOptOutUser_,
    unsigned long campaignSelectOptInUser_,
    unsigned long campaignSelectProbeUser_,
    unsigned long campaignSelectTemporaryUser_,
    unsigned long campaignSelectUndefinedUser_,
    unsigned long passbackRequest_,
    unsigned long profilingRequestReceivedTriggers_,
    unsigned long profilingRequestMatchedTriggers_,
    unsigned long profilingRequestTriggeredChannels_,
    unsigned long profilingRequest_)
    : adRequest(adRequest_),
      adRequestWithImpressionOptInUser(adRequestWithImpressionOptInUser_),
      adRequestWithImpressionnonOptInUser(adRequestWithImpressionnonOptInUser_),
      adRequestWithPassbackOptInUser(adRequestWithPassbackOptInUser_),
      adRequestWithPassbacknonOptInUser(adRequestWithPassbacknonOptInUser_),
      campaignSelectOptOutUser(campaignSelectOptOutUser_),
      campaignSelectOptInUser(campaignSelectOptInUser_),
      campaignSelectProbeUser(campaignSelectProbeUser_),
      campaignSelectTemporaryUser(campaignSelectTemporaryUser_),
      campaignSelectUndefinedUser(campaignSelectUndefinedUser_),
      passbackRequest(passbackRequest_),
      profilingRequestReceivedTriggers(profilingRequestReceivedTriggers_),
      profilingRequestMatchedTriggers(profilingRequestMatchedTriggers_),
      profilingRequestTriggeredChannels(profilingRequestTriggeredChannels_),
      profilingRequest(profilingRequest_) {}

  AdFrontendStat::BaseStatData&
  AdFrontendStat::BaseStatData::operator +=(const AdFrontendStat::BaseStatData& rhs) noexcept {
    adRequest += rhs.adRequest;
    adRequestWithImpressionOptInUser += rhs.adRequestWithImpressionOptInUser;
    adRequestWithImpressionnonOptInUser += rhs.adRequestWithImpressionnonOptInUser;
    adRequestWithPassbackOptInUser += rhs.adRequestWithPassbackOptInUser;
    adRequestWithPassbacknonOptInUser += rhs.adRequestWithPassbacknonOptInUser;
    campaignSelectOptOutUser += rhs.campaignSelectOptOutUser;
    campaignSelectOptInUser += rhs.campaignSelectOptInUser;
    campaignSelectProbeUser += rhs.campaignSelectProbeUser;
    campaignSelectTemporaryUser += rhs.campaignSelectTemporaryUser;
    campaignSelectUndefinedUser += rhs.campaignSelectUndefinedUser;
    passbackRequest += rhs.passbackRequest;
    profilingRequestReceivedTriggers += rhs.profilingRequestReceivedTriggers;
    profilingRequestMatchedTriggers += rhs.profilingRequestMatchedTriggers;
    profilingRequestTriggeredChannels += rhs.profilingRequestTriggeredChannels;
    profilingRequest += rhs.profilingRequest;

    return *this;
  }

  void
  AdFrontendStat::StatData::process_time(const Generics::Time& time, Generics::Time& total, Generics::Time& min, Generics::Time& max) noexcept
  {
    total += time;
    if (time < min)
    {
      min = time;
    }

    if (time > max)
    {
      max = time;
    }
  }

  AdFrontendStat::AdFrontendStat()
    /*throw(Exception)*/
  {
  }

  AdFrontendStat::StageStat&
  AdFrontendStat::select_stage_(RealtimeStatData& data, Stage stage)
    noexcept
  {
    switch(stage)
    {
    case Stage::TriggerMatch:
      return data.trigger_match;
    case Stage::HistoryMatch:
      return data.history_match;
    case Stage::CampaignSelection:
      return data.campaign_selection;
    case Stage::HistoryPostMatch:
      return data.history_post_match;
    }

    return data.history_post_match;
  }

  void
  AdFrontendStat::consider_request(
    const RequestInfo& request_info,
    const RequestTimeMetering& request_times)
    noexcept
  {
    BaseStatData b(
      request_info.tag_id != 0 ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_OPTIN && request_times.creative_count ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT && request_times.creative_count ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_OPTIN && request_times.passback ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT && request_times.passback ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_OPTIN ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_PROBE ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY ? 1UL : 0UL,
      request_info.user_status == AdServer::CampaignSvcs::US_UNDEFINED ? 1UL : 0UL,
      !request_info.original_url.empty() || !request_info.passback_url.empty() ? 1UL : 0UL,
      request_times.recived_triggers,
      request_times.matched_triggers,
      request_times.matched_channels,
      request_times.profiling ? 1UL : 0UL);

    Sync::PosixGuard lock(mutex_);

    stat_data_ += b;

    stat_data_.timeStatCounter += 1;
    StatData::process_time(request_times.history_match_time,
      stat_data_.historyMatchTotal, stat_data_.historyMatchMin, stat_data_.historyMatchMax);
    StatData::process_time(request_times.merge_users_time,
      stat_data_.usersMergeTotal, stat_data_.usersMergeMin, stat_data_.usersMergeMax);
    StatData::process_time(request_times.trigger_match_time,
      stat_data_.triggersMatchTotal, stat_data_.triggersMatchMin, stat_data_.triggersMatchMax);
    StatData::process_time(request_times.request_fill_time,
      stat_data_.requestParsingTotal, stat_data_.requestParsingMin, stat_data_.requestParsingMax);
    StatData::process_time(request_times.creative_selection_time,
      stat_data_.creativeSelectionTotal, stat_data_.creativeSelectionMin, stat_data_.creativeSelectionMax);
    StatData::process_time(request_times.creative_selection_local_time,
      stat_data_.creativeSelectionLocalTotal, stat_data_.creativeSelectionLocalMin, stat_data_.creativeSelectionLocalMax);
  }

  void
  AdFrontendStat::add_request() noexcept
  {
    Sync::PosixGuard lock(mutex_);
    ++realtime_stat_data_.ad_request_total;
    ++realtime_stat_data_.ad_request_in_progress;
  }

  void
  AdFrontendStat::complete_request() noexcept
  {
    Sync::PosixGuard lock(mutex_);
    if (realtime_stat_data_.ad_request_in_progress)
    {
      --realtime_stat_data_.ad_request_in_progress;
    }
  }

  void
  AdFrontendStat::add_stage(Stage stage) noexcept
  {
    Sync::PosixGuard lock(mutex_);
    StageStat& stage_stat = select_stage_(realtime_stat_data_, stage);
    ++stage_stat.total;
    ++stage_stat.in_progress;
  }

  void
  AdFrontendStat::complete_stage(Stage stage) noexcept
  {
    Sync::PosixGuard lock(mutex_);
    StageStat& stage_stat = select_stage_(realtime_stat_data_, stage);
    if (stage_stat.in_progress)
    {
      --stage_stat.in_progress;
    }
  }

  void
  AdFrontendStat::add_stage_time(Stage stage, const Generics::Time& time)
    noexcept
  {
    Sync::PosixGuard lock(mutex_);
    select_stage_(realtime_stat_data_, stage).total_time += time;
  }

  void
  AdFrontendStat::add_stage_error(Stage stage) noexcept
  {
    Sync::PosixGuard lock(mutex_);
    ++select_stage_(realtime_stat_data_, stage).error_total;
  }

  Generics::Values_var
  AdFrontendStat::extract_stats_values()
  {
    StatData d;
    RealtimeStatData realtime;

    {
      Sync::PosixGuard lock(mutex_);
      std::swap(d, stat_data_);
      realtime = realtime_stat_data_;
    }

    Generics::Values_var v(new Generics::Values);

    v->set(DEFINED_TID, d.adRequest);
    v->set(AD_OPT_IN_COUNT, d.adRequestWithImpressionOptInUser);
    v->set(AD_OPT_OUT_COUNT, d.adRequestWithImpressionnonOptInUser);
    v->set(AD_OPT_IN_PASSBACK, d.adRequestWithPassbackOptInUser);
    v->set(AD_OPT_OUT_PASSBACK, d.adRequestWithPassbacknonOptInUser);
    v->set(OPT_OUT_COUNT, d.campaignSelectOptOutUser);
    v->set(OPT_IN_COUNT, d.campaignSelectOptInUser);
    v->set(PROBE_COUNT, d.campaignSelectProbeUser);
    v->set(TEMPORARY_COUNT, d.campaignSelectTemporaryUser);
    v->set(UNDEFINED_COUNT, d.campaignSelectUndefinedUser);
    v->set(PASSBACK_COUNT, d.passbackRequest);
    v->set(TRIGGER_RECIVED, d.profilingRequestReceivedTriggers);
    v->set(TRIGGER_MATCHED, d.profilingRequestMatchedTriggers);
    v->set(CHANNELS_MATCHED, d.profilingRequestTriggeredChannels);
    v->set(PROFILING_REQUEST, d.profilingRequest);
    v->set(USER_INFO_MATCH_TIME + "-Count", d.timeStatCounter);
    v->set(USER_INFO_MATCH_TIME + "-Total", d.historyMatchTotal.as_double());
    v->set(USER_INFO_MATCH_TIME + "-Min", d.historyMatchMin.as_double());
    v->set(USER_INFO_MATCH_TIME + "-Max", d.historyMatchMax.as_double());
    v->set(USER_INFO_MERGE_TIME + "-Count", d.timeStatCounter);
    v->set(USER_INFO_MERGE_TIME + "-Total", d.usersMergeTotal.as_double());
    v->set(USER_INFO_MERGE_TIME + "-Min", d.usersMergeMin.as_double());
    v->set(USER_INFO_MERGE_TIME + "-Max", d.usersMergeMax.as_double());
    v->set(TRIGGER_MATCH_TIME + "-Count", d.timeStatCounter);
    v->set(TRIGGER_MATCH_TIME + "-Total", d.triggersMatchTotal.as_double());
    v->set(TRIGGER_MATCH_TIME + "-Min", d.triggersMatchMin.as_double());
    v->set(TRIGGER_MATCH_TIME + "-Max", d.triggersMatchMax.as_double());
    v->set(REQUEST_FILLING_TIME + "-Count", d.timeStatCounter);
    v->set(REQUEST_FILLING_TIME + "-Total", d.requestParsingTotal.as_double());
    v->set(REQUEST_FILLING_TIME + "-Min", d.requestParsingMin.as_double());
    v->set(REQUEST_FILLING_TIME + "-Max", d.requestParsingMax.as_double());
    v->set(CREATIVE_SELECTION_TIME + "-Count", d.timeStatCounter);
    v->set(CREATIVE_SELECTION_TIME + "-Total", d.creativeSelectionTotal.as_double());
    v->set(CREATIVE_SELECTION_TIME + "-Min", d.creativeSelectionMin.as_double());
    v->set(CREATIVE_SELECTION_TIME + "-Max", d.creativeSelectionMax.as_double());
    v->set(CREATIVE_SELECTION_LOCAL_TIME + "-Count", d.timeStatCounter);
    v->set(CREATIVE_SELECTION_LOCAL_TIME + "-Total", d.creativeSelectionLocalTotal.as_double());
    v->set(CREATIVE_SELECTION_LOCAL_TIME + "-Min", d.creativeSelectionLocalMin.as_double());
    v->set(CREATIVE_SELECTION_LOCAL_TIME + "-Max", d.creativeSelectionLocalMax.as_double());

    v->set(AD_REQUEST_TOTAL, realtime.ad_request_total);
    v->set(AD_REQUEST_IN_PROGRESS, realtime.ad_request_in_progress);
    add_stage_stats(
      *v,
      realtime.trigger_match,
      AD_REQUEST_TRIGGER_MATCH_TOTAL,
      nullptr,
      AD_REQUEST_TRIGGER_MATCH_IN_PROGRESS,
      AD_REQUEST_TRIGGER_MATCH_ERROR_TOTAL);
    add_stage_stats(
      *v,
      realtime.history_match,
      AD_REQUEST_HISTORY_MATCH_TOTAL,
      AD_REQUEST_HISTORY_MATCH_TOTAL_TIME,
      AD_REQUEST_HISTORY_MATCH_IN_PROGRESS,
      AD_REQUEST_HISTORY_MATCH_ERROR_TOTAL);
    add_stage_stats(
      *v,
      realtime.campaign_selection,
      AD_REQUEST_CAMPAIGN_SELECTION_TOTAL,
      AD_REQUEST_CAMPAIGN_SELECTION_TOTAL_TIME,
      AD_REQUEST_CAMPAIGN_SELECTION_IN_PROGRESS,
      AD_REQUEST_CAMPAIGN_SELECTION_ERROR_TOTAL);
    add_stage_stats(
      *v,
      realtime.history_post_match,
      AD_REQUEST_HISTORY_POST_MATCH_TOTAL,
      AD_REQUEST_HISTORY_POST_MATCH_TOTAL_TIME,
      AD_REQUEST_HISTORY_POST_MATCH_IN_PROGRESS,
      AD_REQUEST_HISTORY_POST_MATCH_ERROR_TOTAL);

    return v;
  }
}
