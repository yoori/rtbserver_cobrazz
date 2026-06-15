/// @file BiddingFrontend/BiddingFrontendStat.cpp
#include "BiddingFrontendStat.hpp"
#include "GroupLogger.hpp"

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace
{
  const Generics::Values::Key BF_REQ_COUNT        = "rtb_request_total";
  const Generics::Values::Key BF_REQ_FINISHED     = "rtb_request_finished_total";
  const Generics::Values::Key BF_REQ_BIDS         = "rtb_request_bid_total";
  const Generics::Values::Key FCGI_ACCEPT_TOTAL   = "fcgi_accept_total";
  const Generics::Values::Key FCGI_CONNECTION_IN_PROGRESS =
    "fcgi_connection_in_progress";

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
  const Generics::Values::Key BF_HISTORY_POST_MATCH_TOTAL =
    "rtb_request_history_post_match_total";
  const Generics::Values::Key BF_HISTORY_POST_MATCH_TOTAL_TIME =
    "rtb_request_history_post_match_total_time";
  const Generics::Values::Key BF_HISTORY_POST_MATCH_IN_PROGRESS =
    "rtb_request_history_post_match_in_progress";
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

  std::size_t
  timeout_counter_index(const Generics::Time& value) noexcept
  {
    const GapKey* gap =
      std::lower_bound(GAPS,
        GAPS + sizeof(GAPS)/sizeof(GAPS[0]), value);
    std::size_t index = gap - GAPS;
    return index < sizeof(GAPS)/sizeof(GAPS[0]) ?
      index : sizeof(GAPS)/sizeof(GAPS[0]) - 1;
  }

  void
  decrement_if_positive(std::atomic<unsigned long>& counter) noexcept
  {
    unsigned long value = counter.load(std::memory_order_relaxed);
    while(value != 0 && !counter.compare_exchange_weak(
      value,
      value - 1,
      std::memory_order_relaxed,
      std::memory_order_relaxed))
    {}
  }

  Generics::Time
  time_from_microseconds(long long microseconds) noexcept
  {
    return Generics::Time(
      microseconds / 1000000,
      microseconds % 1000000);
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
      history_post_match_total(0),
      history_post_match_total_time(Generics::Time::ZERO),
      history_post_match_in_progress(0),
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

    request_finished_total_.fetch_add(1, std::memory_order_relaxed);
    if(bid)
    {
      request_total_bid_.fetch_add(1, std::memory_order_relaxed);
    }
    processing_time_us_.fetch_add(
      processing_time.microseconds(),
      std::memory_order_relaxed);
  }

  void
  StatHolder::add_skipped() noexcept
  {
    skipped_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  StatHolder::add_timeout(const Generics::Time& timeout) noexcept
  {
    timeout_counters_[timeout_counter_index(
      Bidding::CellsKey::round_timeout(timeout))].fetch_add(
        1,
        std::memory_order_relaxed);
  }

  void
  StatHolder::add_fcgi_accept() noexcept
  {
    fcgi_accept_total_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  StatHolder::add_fcgi_connection() noexcept
  {
    fcgi_connection_in_progress_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  StatHolder::complete_fcgi_connection() noexcept
  {
    fcgi_connection_in_progress_.fetch_sub(1, std::memory_order_relaxed);
  }

  void
  StatHolder::add_user_bind_request() noexcept
  {
    user_bind_request_count_.fetch_add(1, std::memory_order_relaxed);
    user_bind_requests_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  StatHolder::complete_user_bind_request() noexcept
  {
    decrement_if_positive(user_bind_requests_);
  }

  void
  StatHolder::add_user_bind_rejected_request() noexcept
  {
    user_bind_rejected_requests_.fetch_add(1, std::memory_order_relaxed);
  }

#define ADD_COUNTER(method_name, member_name) \
  void \
  StatHolder::method_name() noexcept \
  { \
    member_name##_.fetch_add(1, std::memory_order_relaxed); \
  }

#define COMPLETE_COUNTER(method_name, member_name) \
  void \
  StatHolder::method_name() noexcept \
  { \
    decrement_if_positive(member_name##_); \
  }

#define ADD_ATOMIC_IN_PROGRESS_COUNTER(method_name, total_member_name, in_progress_member_name) \
  void \
  StatHolder::method_name() noexcept \
  { \
    total_member_name##_.fetch_add(1, std::memory_order_relaxed); \
    in_progress_member_name##_.fetch_add(1, std::memory_order_relaxed); \
  }

#define ADD_ATOMIC_TIME_COUNTER(method_name, member_name) \
  void \
  StatHolder::method_name(const Generics::Time& time) noexcept \
  { \
    member_name##_us_.fetch_add(time.microseconds(), std::memory_order_relaxed); \
  }

#define COMPLETE_ATOMIC_COUNTER(method_name, member_name) \
  void \
  StatHolder::method_name() noexcept \
  { \
    decrement_if_positive(member_name##_); \
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
  ADD_ATOMIC_IN_PROGRESS_COUNTER(add_rtb_request, request_total, request_in_progress)
  COMPLETE_ATOMIC_COUNTER(complete_rtb_request, request_in_progress)
  ADD_ATOMIC_IN_PROGRESS_COUNTER(
    add_rtb_request_user_resolving,
    user_resolving_total,
    user_resolving_in_progress)
  ADD_ATOMIC_TIME_COUNTER(
    add_rtb_request_user_resolving_time,
    user_resolving_total_time)
  COMPLETE_ATOMIC_COUNTER(complete_rtb_request_user_resolving, user_resolving_in_progress)
  ADD_ATOMIC_IN_PROGRESS_COUNTER(
    add_rtb_request_trigger_match,
    trigger_match_total,
    trigger_match_in_progress)
  ADD_ATOMIC_TIME_COUNTER(
    add_rtb_request_trigger_match_time,
    trigger_match_total_time)
  COMPLETE_ATOMIC_COUNTER(complete_rtb_request_trigger_match, trigger_match_in_progress)
  ADD_ATOMIC_IN_PROGRESS_COUNTER(
    add_rtb_request_history_match,
    history_match_total,
    history_match_in_progress)
  ADD_ATOMIC_TIME_COUNTER(
    add_rtb_request_history_match_time,
    history_match_total_time)
  COMPLETE_ATOMIC_COUNTER(complete_rtb_request_history_match, history_match_in_progress)
  ADD_ATOMIC_IN_PROGRESS_COUNTER(
    add_rtb_request_campaign_selection,
    campaign_selection_total,
    campaign_selection_in_progress)
  ADD_ATOMIC_TIME_COUNTER(
    add_rtb_request_campaign_selection_time,
    campaign_selection_total_time)
  COMPLETE_ATOMIC_COUNTER(complete_rtb_request_campaign_selection, campaign_selection_in_progress)
  ADD_ATOMIC_IN_PROGRESS_COUNTER(
    add_rtb_request_history_post_match,
    history_post_match_total,
    history_post_match_in_progress)
  ADD_ATOMIC_TIME_COUNTER(
    add_rtb_request_history_post_match_time,
    history_post_match_total_time)
  COMPLETE_ATOMIC_COUNTER(
    complete_rtb_request_history_post_match,
    history_post_match_in_progress)

#undef ADD_COUNTER
#undef COMPLETE_COUNTER
#undef ADD_ATOMIC_IN_PROGRESS_COUNTER
#undef ADD_ATOMIC_TIME_COUNTER
#undef COMPLETE_ATOMIC_COUNTER

  StatHolder::RtbRequestInProgressStats
  StatHolder::rtb_request_in_progress_stats() noexcept
  {
    return RtbRequestInProgressStats{
      request_in_progress_.load(std::memory_order_relaxed),
      user_resolving_in_progress_.load(std::memory_order_relaxed),
      trigger_match_in_progress_.load(std::memory_order_relaxed),
      history_match_in_progress_.load(std::memory_order_relaxed),
      campaign_selection_in_progress_.load(std::memory_order_relaxed),
      history_post_match_in_progress_.load(std::memory_order_relaxed)};
  }

  Generics::Values_var
  StatHolder::dump_stats()
  {
    StatData d;

    d.request_total = request_total_.load(std::memory_order_relaxed);
    d.request_finished_total =
      request_finished_total_.load(std::memory_order_relaxed);
    d.request_total_bid = request_total_bid_.load(std::memory_order_relaxed);
    d.skipped = skipped_.load(std::memory_order_relaxed);
    d.request_in_progress = request_in_progress_.load(std::memory_order_relaxed);
    d.user_resolving_total =
      user_resolving_total_.load(std::memory_order_relaxed);
    d.user_resolving_total_time = time_from_microseconds(
      user_resolving_total_time_us_.load(std::memory_order_relaxed));
    d.user_resolving_in_progress =
      user_resolving_in_progress_.load(std::memory_order_relaxed);
    d.trigger_match_total = trigger_match_total_.load(std::memory_order_relaxed);
    d.trigger_match_total_time = time_from_microseconds(
      trigger_match_total_time_us_.load(std::memory_order_relaxed));
    d.trigger_match_in_progress =
      trigger_match_in_progress_.load(std::memory_order_relaxed);
    d.history_match_total = history_match_total_.load(std::memory_order_relaxed);
    d.history_match_total_time = time_from_microseconds(
      history_match_total_time_us_.load(std::memory_order_relaxed));
    d.history_match_in_progress =
      history_match_in_progress_.load(std::memory_order_relaxed);
    d.campaign_selection_total =
      campaign_selection_total_.load(std::memory_order_relaxed);
    d.campaign_selection_total_time = time_from_microseconds(
      campaign_selection_total_time_us_.load(std::memory_order_relaxed));
    d.campaign_selection_in_progress =
      campaign_selection_in_progress_.load(std::memory_order_relaxed);
    d.history_post_match_total =
      history_post_match_total_.load(std::memory_order_relaxed);
    d.history_post_match_total_time = time_from_microseconds(
      history_post_match_total_time_us_.load(std::memory_order_relaxed));
    d.history_post_match_in_progress =
      history_post_match_in_progress_.load(std::memory_order_relaxed);
    d.processing_time = time_from_microseconds(
      processing_time_us_.load(std::memory_order_relaxed));
    d.user_bind_request_count =
      user_bind_request_count_.load(std::memory_order_relaxed);
    d.user_bind_requests =
      user_bind_requests_.load(std::memory_order_relaxed);
    d.user_bind_rejected_requests =
      user_bind_rejected_requests_.load(std::memory_order_relaxed);
    d.user_bind_match_requests =
      user_bind_match_requests_.load(std::memory_order_relaxed);
    d.user_bind_match_rejected_requests =
      user_bind_match_rejected_requests_.load(std::memory_order_relaxed);
    d.user_bind_match_channel_requests =
      user_bind_match_channel_requests_.load(std::memory_order_relaxed);
    d.user_bind_match_get_profile_requests =
      user_bind_match_get_profile_requests_.load(std::memory_order_relaxed);
    d.user_bind_match_merge_requests =
      user_bind_match_merge_requests_.load(std::memory_order_relaxed);
    d.user_bind_match_remove_requests =
      user_bind_match_remove_requests_.load(std::memory_order_relaxed);
    d.user_bind_match_history_requests =
      user_bind_match_history_requests_.load(std::memory_order_relaxed);
    d.user_bind_match_campaign_requests =
      user_bind_match_campaign_requests_.load(std::memory_order_relaxed);

    Generics::Values_var v(new Generics::Values);

    v->set(BF_REQ_COUNT, d.request_total);
    v->set(BF_REQ_FINISHED, d.request_finished_total);
    v->set(BF_REQ_BIDS, d.request_total_bid);
    v->set(FCGI_ACCEPT_TOTAL, fcgi_accept_total_.load(std::memory_order_relaxed));
    v->set(
      FCGI_CONNECTION_IN_PROGRESS,
      fcgi_connection_in_progress_.load(std::memory_order_relaxed));

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
    v->set(BF_HISTORY_POST_MATCH_TOTAL, d.history_post_match_total);
    v->set(BF_HISTORY_POST_MATCH_TOTAL_TIME, d.history_post_match_total_time.as_double());
    v->set(BF_HISTORY_POST_MATCH_IN_PROGRESS, d.history_post_match_in_progress);
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
    for (std::size_t index = 0;
      index < sizeof(GAPS)/sizeof(GAPS[0]); ++index)
    {
      const auto value = timeout_counters_[index].load(std::memory_order_relaxed);
      timeout_counter += value;
      if(value)
      {
        v->set(GAPS[index].key, value);
      }
    }
    v->set(BF_TIMEOUTS, timeout_counter);
    v->set(BF_TIME_COUNTER, static_cast<unsigned long>(d.processing_time.microseconds()));

    return v;
  }

}
