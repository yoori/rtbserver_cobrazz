/// @file BiddingFrontend/BiddingFrontendStat.cpp
#include "BiddingFrontendStat.hpp"
#include "GroupLogger.hpp"

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace
{
  const Generics::Values::Key BF_REQ_COUNT        = "rtb_request_total";
  const Generics::Values::Key BF_REQ_FINISHED     = "rtb_request_finished_total";
  const Generics::Values::Key BF_REQ_BIDS         = "rtb_request_bid_total";

  const Generics::Values::Key BF_SKIPPED          = "rtb_request_skip_total";
  const Generics::Values::Key BF_TIMEOUTS         = "rtb_request_timeout_total";
  const Generics::Values::Key BF_TIME_COUNTER     = "rtb_request_time_counter";
  const Generics::Values::Key BF_REQ_IN_PROGRESS  = "rtb_request_in_progress";
  const Generics::Values::Key BF_USER_RESOLVING_TOTAL =
    "rtb_request_user_resolving_total";
  const Generics::Values::Key BF_USER_RESOLVING_TOTAL_TIME =
    "rtb_request_user_resolving_total_time";
  const Generics::Values::Key BF_USER_RESOLVING_IN_PROGRESS =
    "rtb_request_user_resolving_in_progress";
  const Generics::Values::Key BF_TRIGGER_MATCH_TOTAL =
    "rtb_request_trigger_match_total";
  const Generics::Values::Key BF_TRIGGER_MATCH_TOTAL_TIME =
    "rtb_request_trigger_match_total_time";
  const Generics::Values::Key BF_TRIGGER_MATCH_IN_PROGRESS =
    "rtb_request_trigger_match_in_progress";
  const Generics::Values::Key BF_HISTORY_MATCH_TOTAL =
    "rtb_request_history_match_total";
  const Generics::Values::Key BF_HISTORY_MATCH_TOTAL_TIME =
    "rtb_request_history_match_total_time";
  const Generics::Values::Key BF_HISTORY_MATCH_IN_PROGRESS =
    "rtb_request_history_match_in_progress";
  const Generics::Values::Key BF_CAMPAIGN_SELECTION_TOTAL =
    "rtb_request_campaign_selection_total";
  const Generics::Values::Key BF_CAMPAIGN_SELECTION_TOTAL_TIME =
    "rtb_request_campaign_selection_total_time";
  const Generics::Values::Key BF_CAMPAIGN_SELECTION_IN_PROGRESS =
    "rtb_request_campaign_selection_in_progress";
  const Generics::Values::Key UB_BIND_REQUEST_COUNT = "user_bind_request_total";
  const Generics::Values::Key UB_BIND_REQUESTS    = "user_bind_request_in_progress";
  const Generics::Values::Key UB_BIND_REJECTED    = "user_bind_request_rejected_total";
  const Generics::Values::Key UB_MATCH_REQUESTS   = "user_bind_match_request_in_progress";
  const Generics::Values::Key UB_MATCH_REJECTED   = "user_bind_match_request_rejected_total";
  const Generics::Values::Key UB_MATCH_CHANNEL    = "user_bind_match_channel_in_progress";
  const Generics::Values::Key UB_MATCH_GET_PROFILE = "user_bind_match_get_profile_in_progress";
  const Generics::Values::Key UB_MATCH_MERGE      = "user_bind_match_merge_in_progress";
  const Generics::Values::Key UB_MATCH_REMOVE     = "user_bind_match_remove_in_progress";
  const Generics::Values::Key UB_MATCH_HISTORY    = "user_bind_match_history_in_progress";
  const Generics::Values::Key UB_MATCH_CAMPAIGN   = "user_bind_match_campaign_in_progress";

  struct GapKey
  {
    Generics::Time t;
    Generics::Values::Key key;
    bool
    operator <(const Generics::Time& rhs) const noexcept
    {
      return t < rhs;
    }
  };

  const GapKey GAPS[] =
  {
    {Generics::Time(0, 10000), "timeout_count_time_10ms"},
    {Generics::Time(0, 20000), "timeout_count_time_20ms"},
    {Generics::Time(0, 30000), "timeout_count_time_30ms"},
    {Generics::Time(0, 40000), "timeout_count_time_40ms"},
    {Generics::Time(0, 50000), "timeout_count_time_50ms"},
    {Generics::Time(0, 60000), "timeout_count_time_60ms"},
    {Generics::Time(0, 70000), "timeout_count_time_70ms"},
    {Generics::Time(0, 80000), "timeout_count_time_80ms"},
    {Generics::Time(0, 90000), "timeout_count_time_90ms"},
    {Generics::Time(0, 100000), "timeout_count_time_100ms"},
    {Generics::Time(0, 200000), "timeout_count_time_200ms"},
    {Generics::Time(0, 300000), "timeout_count_time_300ms"},
    {Generics::Time(0, 400000), "timeout_count_time_400ms"},
    {Generics::Time(0, 500000), "timeout_count_time_500ms"},
    {Generics::Time(0, 600000), "timeout_count_time_600ms"},
    {Generics::Time(0, 700000), "timeout_count_time_700ms"},
    {Generics::Time(0, 800000), "timeout_count_time_800ms"},
    {Generics::Time(0, 900000), "timeout_count_time_900ms"},
    {Generics::Time(1, 0), "timeout_count_time_1sec"},
    {Generics::Time(2, 0), "timeout_count_time_2sec"},
    {Generics::Time(3, 0), "timeout_count_time_3sec"},
    {Generics::Time(4, 0), "timeout_count_time_4sec"},
    {Generics::Time(5, 0), "timeout_count_time_5sec"},
    {Generics::Time(6, 0), "timeout_count_time_6sec"},
    {Generics::Time(7, 0), "timeout_count_time_7sec"},
    {Generics::Time(8, 0), "timeout_count_time_8sec"},
    {Generics::Time(9, 0), "timeout_count_time_9sec"},
    {Generics::Time(10, 0), "timeout_count_time_10sec"},
  };

  const Generics::Values::Key&
  convert_to_key(const Generics::Time& value) noexcept
  {
    const GapKey* gap =
      std::lower_bound(GAPS,
        GAPS + sizeof(GAPS)/sizeof(GAPS[0]), value);
    std::size_t index = gap - GAPS;
    return index < sizeof(GAPS)/sizeof(GAPS[0]) ? GAPS[index].key :
      GAPS[sizeof(GAPS)/sizeof(GAPS[0]) - 1].key;
  }
}

namespace Aspect
{
  extern const char BIDDING_FRONTEND[];
}

using namespace AdServer::CampaignSvcs;

namespace AdServer
{
  // StatHolder::StatData

  StatHolder::StatData::StatData()
    : request_total(0),
      request_finished_total(0),
      request_total_bid(0),
      skipped(0),
      request_in_progress(0),
      user_resolving_total(0),
      user_resolving_total_time(Generics::Time::ZERO),
      user_resolving_in_progress(0),
      trigger_match_total(0),
      trigger_match_total_time(Generics::Time::ZERO),
      trigger_match_in_progress(0),
      history_match_total(0),
      history_match_total_time(Generics::Time::ZERO),
      history_match_in_progress(0),
      campaign_selection_total(0),
      campaign_selection_total_time(Generics::Time::ZERO),
      campaign_selection_in_progress(0),
      user_bind_request_count(0),
      user_bind_requests(0),
      user_bind_rejected_requests(0),
      user_bind_match_requests(0),
      user_bind_match_rejected_requests(0),
      user_bind_match_channel_requests(0),
      user_bind_match_get_profile_requests(0),
      user_bind_match_merge_requests(0),
      user_bind_match_remove_requests(0),
      user_bind_match_history_requests(0),
      user_bind_match_campaign_requests(0)
  {}

  StatHolder::StatData::StatData(
    unsigned long request_finished_total_,
    unsigned long request_total_bid_,
    const Generics::Time& processing_time_val)
    : request_total(0),
      request_finished_total(request_finished_total_),
      request_total_bid(request_total_bid_),
      skipped(0),
      request_in_progress(0),
      user_resolving_total(0),
      user_resolving_total_time(Generics::Time::ZERO),
      user_resolving_in_progress(0),
      trigger_match_total(0),
      trigger_match_total_time(Generics::Time::ZERO),
      trigger_match_in_progress(0),
      history_match_total(0),
      history_match_total_time(Generics::Time::ZERO),
      history_match_in_progress(0),
      campaign_selection_total(0),
      campaign_selection_total_time(Generics::Time::ZERO),
      campaign_selection_in_progress(0),
      user_bind_request_count(0),
      user_bind_requests(0),
      user_bind_rejected_requests(0),
      user_bind_match_requests(0),
      user_bind_match_rejected_requests(0),
      user_bind_match_channel_requests(0),
      user_bind_match_get_profile_requests(0),
      user_bind_match_merge_requests(0),
      user_bind_match_remove_requests(0),
      user_bind_match_history_requests(0),
      user_bind_match_campaign_requests(0),
      processing_time(processing_time_val)
  {}

  StatHolder::StatData&
  StatHolder::StatData::operator +=(const StatHolder::StatData& rhs) noexcept
  {
    request_total += rhs.request_total;
    request_finished_total += rhs.request_finished_total;
    request_total_bid += rhs.request_total_bid;
    skipped += rhs.skipped;
    request_in_progress += rhs.request_in_progress;
    user_resolving_total += rhs.user_resolving_total;
    user_resolving_total_time += rhs.user_resolving_total_time;
    user_resolving_in_progress += rhs.user_resolving_in_progress;
    trigger_match_total += rhs.trigger_match_total;
    trigger_match_total_time += rhs.trigger_match_total_time;
    trigger_match_in_progress += rhs.trigger_match_in_progress;
    history_match_total += rhs.history_match_total;
    history_match_total_time += rhs.history_match_total_time;
    history_match_in_progress += rhs.history_match_in_progress;
    campaign_selection_total += rhs.campaign_selection_total;
    campaign_selection_total_time += rhs.campaign_selection_total_time;
    campaign_selection_in_progress += rhs.campaign_selection_in_progress;
    user_bind_request_count += rhs.user_bind_request_count;
    user_bind_requests += rhs.user_bind_requests;
    user_bind_rejected_requests += rhs.user_bind_rejected_requests;
    user_bind_match_requests += rhs.user_bind_match_requests;
    user_bind_match_rejected_requests += rhs.user_bind_match_rejected_requests;
    user_bind_match_channel_requests += rhs.user_bind_match_channel_requests;
    user_bind_match_get_profile_requests += rhs.user_bind_match_get_profile_requests;
    user_bind_match_merge_requests += rhs.user_bind_match_merge_requests;
    user_bind_match_remove_requests += rhs.user_bind_match_remove_requests;
    user_bind_match_history_requests += rhs.user_bind_match_history_requests;
    user_bind_match_campaign_requests += rhs.user_bind_match_campaign_requests;
    processing_time += rhs.processing_time;
    for (TimeoutsMap::const_iterator it = rhs.timeout_counters.begin();
      it != rhs.timeout_counters.end(); ++it)
    {
      timeout_counters[it->first] += it->second;
    }

    return *this;
  }

  // StatHolder

  void
  StatHolder::flush(
    const AdServer::Bidding::CampaignManager::RequestParams& request_params,
    AdServer::Bidding::CampaignManager::RequestCreativeResult*
      campaign_match_result,
    const Generics::Time& processing_time)
    noexcept
  {
    bool bid = campaign_match_result && campaign_match_result->ad_slots.size();
    (void)request_params;

    StatData b(1UL,
               bid ? 1UL : 0UL,
               processing_time);

    Sync::PosixGuard lock(mutex_);

    stat_data_ += b;
  }

  void
  StatHolder::add_skipped() noexcept
  {
    Sync::PosixGuard lock(mutex_);
    ++stat_data_.skipped;
  }

  void
  StatHolder::add_timeout(const Generics::Time& timeout) noexcept
  {
    Generics::Time time_round = Bidding::CellsKey::round_timeout(timeout);
    Sync::PosixGuard lock(mutex_);
    ++stat_data_.timeout_counters[time_round];
  }

  void
  StatHolder::add_user_bind_request() noexcept
  {
    Sync::PosixGuard lock(mutex_);
    ++stat_data_.user_bind_request_count;
    ++stat_data_.user_bind_requests;
  }

  void
  StatHolder::complete_user_bind_request() noexcept
  {
    Sync::PosixGuard lock(mutex_);
    if(stat_data_.user_bind_requests)
    {
      --stat_data_.user_bind_requests;
    }
  }

  void
  StatHolder::add_user_bind_rejected_request() noexcept
  {
    Sync::PosixGuard lock(mutex_);
    ++stat_data_.user_bind_rejected_requests;
  }

#define ADD_COUNTER(method_name, member_name) \
  void \
  StatHolder::method_name() noexcept \
  { \
    Sync::PosixGuard lock(mutex_); \
    ++stat_data_.member_name; \
  }

#define ADD_IN_PROGRESS_COUNTER(method_name, total_member_name, in_progress_member_name) \
  void \
  StatHolder::method_name() noexcept \
  { \
    Sync::PosixGuard lock(mutex_); \
    ++stat_data_.total_member_name; \
    ++stat_data_.in_progress_member_name; \
  }

#define ADD_TIME_COUNTER(method_name, member_name) \
  void \
  StatHolder::method_name(const Generics::Time& time) noexcept \
  { \
    Sync::PosixGuard lock(mutex_); \
    stat_data_.member_name += time; \
  }

#define COMPLETE_COUNTER(method_name, member_name) \
  void \
  StatHolder::method_name() noexcept \
  { \
    Sync::PosixGuard lock(mutex_); \
    if(stat_data_.member_name) \
    { \
      --stat_data_.member_name; \
    } \
  }

  ADD_COUNTER(add_user_bind_match_request, user_bind_match_requests)
  COMPLETE_COUNTER(complete_user_bind_match_request, user_bind_match_requests)
  ADD_COUNTER(add_user_bind_match_rejected_request, user_bind_match_rejected_requests)
  ADD_COUNTER(add_user_bind_match_channel_request, user_bind_match_channel_requests)
  COMPLETE_COUNTER(complete_user_bind_match_channel_request, user_bind_match_channel_requests)
  ADD_COUNTER(add_user_bind_match_get_profile_request, user_bind_match_get_profile_requests)
  COMPLETE_COUNTER(complete_user_bind_match_get_profile_request, user_bind_match_get_profile_requests)
  ADD_COUNTER(add_user_bind_match_merge_request, user_bind_match_merge_requests)
  COMPLETE_COUNTER(complete_user_bind_match_merge_request, user_bind_match_merge_requests)
  ADD_COUNTER(add_user_bind_match_remove_request, user_bind_match_remove_requests)
  COMPLETE_COUNTER(complete_user_bind_match_remove_request, user_bind_match_remove_requests)
  ADD_COUNTER(add_user_bind_match_history_request, user_bind_match_history_requests)
  COMPLETE_COUNTER(complete_user_bind_match_history_request, user_bind_match_history_requests)
  ADD_COUNTER(add_user_bind_match_campaign_request, user_bind_match_campaign_requests)
  COMPLETE_COUNTER(complete_user_bind_match_campaign_request, user_bind_match_campaign_requests)
  ADD_IN_PROGRESS_COUNTER(add_rtb_request, request_total, request_in_progress)
  COMPLETE_COUNTER(complete_rtb_request, request_in_progress)
  ADD_IN_PROGRESS_COUNTER(
    add_rtb_request_user_resolving,
    user_resolving_total,
    user_resolving_in_progress)
  ADD_TIME_COUNTER(
    add_rtb_request_user_resolving_time,
    user_resolving_total_time)
  COMPLETE_COUNTER(complete_rtb_request_user_resolving, user_resolving_in_progress)
  ADD_IN_PROGRESS_COUNTER(
    add_rtb_request_trigger_match,
    trigger_match_total,
    trigger_match_in_progress)
  ADD_TIME_COUNTER(
    add_rtb_request_trigger_match_time,
    trigger_match_total_time)
  COMPLETE_COUNTER(complete_rtb_request_trigger_match, trigger_match_in_progress)
  ADD_IN_PROGRESS_COUNTER(
    add_rtb_request_history_match,
    history_match_total,
    history_match_in_progress)
  ADD_TIME_COUNTER(
    add_rtb_request_history_match_time,
    history_match_total_time)
  COMPLETE_COUNTER(complete_rtb_request_history_match, history_match_in_progress)
  ADD_IN_PROGRESS_COUNTER(
    add_rtb_request_campaign_selection,
    campaign_selection_total,
    campaign_selection_in_progress)
  ADD_TIME_COUNTER(
    add_rtb_request_campaign_selection_time,
    campaign_selection_total_time)
  COMPLETE_COUNTER(complete_rtb_request_campaign_selection, campaign_selection_in_progress)

#undef ADD_COUNTER
#undef ADD_IN_PROGRESS_COUNTER
#undef ADD_TIME_COUNTER
#undef COMPLETE_COUNTER

  StatHolder::RtbRequestInProgressStats
  StatHolder::rtb_request_in_progress_stats() noexcept
  {
    Sync::PosixGuard lock(mutex_);
    return RtbRequestInProgressStats{
      stat_data_.request_in_progress,
      stat_data_.user_resolving_in_progress,
      stat_data_.trigger_match_in_progress,
      stat_data_.history_match_in_progress,
      stat_data_.campaign_selection_in_progress};
  }

  Generics::Values_var
  StatHolder::dump_stats()
  {
    StatData d;

    {
      Sync::PosixGuard lock(mutex_);
      d += stat_data_;
    }

    Generics::Values_var v(new Generics::Values);

    v->set(BF_REQ_COUNT, d.request_total);
    v->set(BF_REQ_FINISHED, d.request_finished_total);
    v->set(BF_REQ_BIDS, d.request_total_bid);

    v->set(BF_SKIPPED, d.skipped);
    v->set(BF_REQ_IN_PROGRESS, d.request_in_progress);
    v->set(BF_USER_RESOLVING_TOTAL, d.user_resolving_total);
    v->set(BF_USER_RESOLVING_TOTAL_TIME, d.user_resolving_total_time.as_double());
    v->set(BF_USER_RESOLVING_IN_PROGRESS, d.user_resolving_in_progress);
    v->set(BF_TRIGGER_MATCH_TOTAL, d.trigger_match_total);
    v->set(BF_TRIGGER_MATCH_TOTAL_TIME, d.trigger_match_total_time.as_double());
    v->set(BF_TRIGGER_MATCH_IN_PROGRESS, d.trigger_match_in_progress);
    v->set(BF_HISTORY_MATCH_TOTAL, d.history_match_total);
    v->set(BF_HISTORY_MATCH_TOTAL_TIME, d.history_match_total_time.as_double());
    v->set(BF_HISTORY_MATCH_IN_PROGRESS, d.history_match_in_progress);
    v->set(BF_CAMPAIGN_SELECTION_TOTAL, d.campaign_selection_total);
    v->set(BF_CAMPAIGN_SELECTION_TOTAL_TIME, d.campaign_selection_total_time.as_double());
    v->set(BF_CAMPAIGN_SELECTION_IN_PROGRESS, d.campaign_selection_in_progress);
    v->set(UB_BIND_REQUEST_COUNT, d.user_bind_request_count);
    v->set(UB_BIND_REQUESTS, d.user_bind_requests);
    v->set(UB_BIND_REJECTED, d.user_bind_rejected_requests);
    v->set(UB_MATCH_REQUESTS, d.user_bind_match_requests);
    v->set(UB_MATCH_REJECTED, d.user_bind_match_rejected_requests);
    v->set(UB_MATCH_CHANNEL, d.user_bind_match_channel_requests);
    v->set(UB_MATCH_GET_PROFILE, d.user_bind_match_get_profile_requests);
    v->set(UB_MATCH_MERGE, d.user_bind_match_merge_requests);
    v->set(UB_MATCH_REMOVE, d.user_bind_match_remove_requests);
    v->set(UB_MATCH_HISTORY, d.user_bind_match_history_requests);
    v->set(UB_MATCH_CAMPAIGN, d.user_bind_match_campaign_requests);
    std::size_t timeout_counter = 0;
    for (StatData::TimeoutsMap::const_iterator cit = d.timeout_counters.begin();
      cit != d.timeout_counters.end(); ++cit)
    {
      timeout_counter += cit->second;
      v->set(convert_to_key(cit->first), cit->second);
    }
    v->set(BF_TIMEOUTS, timeout_counter);
    v->set(BF_TIME_COUNTER, static_cast<unsigned long>(d.processing_time.microseconds()));

    return v;
  }

}
