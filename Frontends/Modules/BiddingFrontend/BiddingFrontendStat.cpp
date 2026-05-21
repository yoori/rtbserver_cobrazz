/// @file BiddingFrontend/BiddingFrontendStat.cpp
#include "BiddingFrontendStat.hpp"
#include "GroupLogger.hpp"

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace
{
  const Generics::Values::Key BF_REQ_COUNT        = "rtbRequestCount";
  const Generics::Values::Key BF_REQ_BIDS         = "rtbRequestBidCount";
  const Generics::Values::Key BF_REQ_GOOGLE       = "rtbRequestGoogleCount";
  const Generics::Values::Key BF_REQ_GOOGLE_BIDS  = "rtbRequestGoogleBidCount";
  const Generics::Values::Key BF_REQ_OPENRTB      = "rtbRequestOpenRTBCount";
  const Generics::Values::Key BF_REQ_OPENRTB_BIDS = "rtbRequestOpenRTBBidCount";
  const Generics::Values::Key BF_REQ_OTHER        = "rtbRequestOtherCount";
  const Generics::Values::Key BF_REQ_OTHER_BIDS   = "rtbRequestOtherBidCount";

  const Generics::Values::Key BF_SKIPPED          = "rtbRequestSkipCount";
  const Generics::Values::Key BF_TIMEOUTS         = "rtbRequestTimeoutCount";
  const Generics::Values::Key BF_TIME_COUNTER     = "rtbRequestTimeCounter";
  const Generics::Values::Key UB_BIND_REQUESTS    = "userBindRequestInProgressCount";
  const Generics::Values::Key UB_BIND_REJECTED    = "userBindRequestRejectedCount";
  const Generics::Values::Key UB_MATCH_REQUESTS   = "userBindMatchRequestInProgressCount";
  const Generics::Values::Key UB_MATCH_REJECTED   = "userBindMatchRequestRejectedCount";
  const Generics::Values::Key UB_MATCH_CHANNEL    = "userBindMatchChannelInProgressCount";
  const Generics::Values::Key UB_MATCH_GET_PROFILE = "userBindMatchGetProfileInProgressCount";
  const Generics::Values::Key UB_MATCH_MERGE      = "userBindMatchMergeInProgressCount";
  const Generics::Values::Key UB_MATCH_REMOVE     = "userBindMatchRemoveInProgressCount";
  const Generics::Values::Key UB_MATCH_HISTORY    = "userBindMatchHistoryInProgressCount";
  const Generics::Values::Key UB_MATCH_CAMPAIGN   = "userBindMatchCampaignInProgressCount";

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
    {Generics::Time(0, 10000), "timeoutCount.time10ms"},
    {Generics::Time(0, 20000), "timeoutCount.time20ms"},
    {Generics::Time(0, 30000), "timeoutCount.time30ms"},
    {Generics::Time(0, 40000), "timeoutCount.time40ms"},
    {Generics::Time(0, 50000), "timeoutCount.time50ms"},
    {Generics::Time(0, 60000), "timeoutCount.time60ms"},
    {Generics::Time(0, 70000), "timeoutCount.time70ms"},
    {Generics::Time(0, 80000), "timeoutCount.time80ms"},
    {Generics::Time(0, 90000), "timeoutCount.time90ms"},
    {Generics::Time(0, 100000), "timeoutCount.time100ms"},
    {Generics::Time(0, 200000), "timeoutCount.time200ms"},
    {Generics::Time(0, 300000), "timeoutCount.time300ms"},
    {Generics::Time(0, 400000), "timeoutCount.time400ms"},
    {Generics::Time(0, 500000), "timeoutCount.time500ms"},
    {Generics::Time(0, 600000), "timeoutCount.time600ms"},
    {Generics::Time(0, 700000), "timeoutCount.time700ms"},
    {Generics::Time(0, 800000), "timeoutCount.time800ms"},
    {Generics::Time(0, 900000), "timeoutCount.time900ms"},
    {Generics::Time(1, 0), "timeoutCount.time1sec"},
    {Generics::Time(2, 0), "timeoutCount.time2sec"},
    {Generics::Time(3, 0), "timeoutCount.time3sec"},
    {Generics::Time(4, 0), "timeoutCount.time4sec"},
    {Generics::Time(5, 0), "timeoutCount.time5sec"},
    {Generics::Time(6, 0), "timeoutCount.time6sec"},
    {Generics::Time(7, 0), "timeoutCount.time7sec"},
    {Generics::Time(8, 0), "timeoutCount.time8sec"},
    {Generics::Time(9, 0), "timeoutCount.time9sec"},
    {Generics::Time(10, 0), "timeoutCount.time10sec"},
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
    : request_google(0),
      request_google_bid(0),
      request_openrtb(0),
      request_openrtb_bid(0),
      request_other(0),
      request_other_bid(0),
      skipped(0),
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
    unsigned long request_google_,
    unsigned long request_google_bid_,
    unsigned long request_openrtb_,
    unsigned long request_openrtb_bid_,
    unsigned long request_other_,
    unsigned long request_other_bid_,
    const Generics::Time& processing_time_val)
    : request_google(request_google_),
      request_google_bid(request_google_bid_),
      request_openrtb(request_openrtb_),
      request_openrtb_bid(request_openrtb_bid_),
      request_other(request_other_),
      request_other_bid(request_other_bid_),
      skipped(0),
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
    request_google += rhs.request_google;
    request_google_bid += rhs.request_google_bid;
    request_openrtb += rhs.request_openrtb;
    request_openrtb_bid += rhs.request_openrtb_bid;
    request_other += rhs.request_other;
    request_other_bid += rhs.request_other_bid;
    skipped += rhs.skipped;
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
    bool google = request_params.common_info.request_type == AR_GOOGLE;
    bool openrtb = request_params.common_info.request_type == AR_OPENRTB ||
                   request_params.common_info.request_type == AR_OPENRTB_WITH_CLICKURL;
    bool other = !openrtb && !google;

    StatData b(google ? 1UL : 0UL,
               google && bid ? 1UL : 0UL,
               openrtb ? 1UL : 0UL,
               openrtb && bid ? 1UL : 0UL,
               other ? 1UL : 0UL,
               other && bid ? 1UL : 0UL,
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

#undef ADD_COUNTER
#undef COMPLETE_COUNTER

  Generics::Values_var
  StatHolder::dump_stats()
  {
    StatData d;

    {
      Sync::PosixGuard lock(mutex_);
      d += stat_data_;
    }

    Generics::Values_var v(new Generics::Values);

    std::size_t request_count =
        d.request_openrtb + d.request_other + d.request_google;

    v->set(BF_REQ_COUNT, request_count);
    v->set(
      BF_REQ_BIDS,
        d.request_openrtb_bid + d.request_other_bid +
          d.request_google_bid);
    v->set(BF_REQ_GOOGLE, d.request_google);
    v->set(BF_REQ_GOOGLE_BIDS, d.request_google_bid);
    v->set(BF_REQ_OPENRTB, d.request_openrtb);
    v->set(BF_REQ_OPENRTB_BIDS, d.request_openrtb_bid);
    v->set(BF_REQ_OTHER, d.request_other);
    v->set(BF_REQ_OTHER_BIDS, d.request_other_bid);

    v->set(BF_SKIPPED, d.skipped);
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
