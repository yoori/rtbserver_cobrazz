/// @file BiddingFrontend/BiddingFrontendStat.hpp
#pragma once

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Controlling/StatsDumper/StatsDumper.hpp>
#include "CampaignManagerTypes.hpp"


namespace AdServer
{
  class BiddingStatInfo;

  class StatHolder : public ReferenceCounting::AtomicImpl
  {
  public:
    struct RtbRequestInProgressStats
    {
      unsigned long request = 0;
      unsigned long user_resolving = 0;
      unsigned long trigger_match = 0;
      unsigned long history_match = 0;
      unsigned long campaign_selection = 0;
    };

    struct StatData
    {
      StatData();
      StatData(
        unsigned long request_total_,
        unsigned long request_total_bid_,
        const Generics::Time& processing_time);

      unsigned long request_total;
      unsigned long request_total_bid;
      unsigned long skipped;
      unsigned long request_in_progress;
      unsigned long user_resolving_in_progress;
      unsigned long trigger_match_in_progress;
      unsigned long history_match_in_progress;
      unsigned long campaign_selection_in_progress;
      unsigned long user_bind_request_count;
      unsigned long user_bind_requests;
      unsigned long user_bind_rejected_requests;
      unsigned long user_bind_match_requests;
      unsigned long user_bind_match_rejected_requests;
      unsigned long user_bind_match_channel_requests;
      unsigned long user_bind_match_get_profile_requests;
      unsigned long user_bind_match_merge_requests;
      unsigned long user_bind_match_remove_requests;
      unsigned long user_bind_match_history_requests;
      unsigned long user_bind_match_campaign_requests;
      Generics::Time processing_time;
      typedef std::map<Generics::Time, std::size_t> TimeoutsMap;
      TimeoutsMap timeout_counters;

      StatData&
      operator +=(const StatData& rhs) noexcept;
    };

    void
    flush(
      const AdServer::Bidding::CampaignManager::RequestParams&,
      AdServer::Bidding::CampaignManager::RequestCreativeResult*,
      const Generics::Time& processing_time)
      noexcept;

    void
    add_skipped() noexcept;

    void
    add_timeout(const Generics::Time& timeout) noexcept;

    void
    add_rtb_request() noexcept;

    void
    complete_rtb_request() noexcept;

    void
    add_rtb_request_user_resolving() noexcept;

    void
    complete_rtb_request_user_resolving() noexcept;

    void
    add_rtb_request_trigger_match() noexcept;

    void
    complete_rtb_request_trigger_match() noexcept;

    void
    add_rtb_request_history_match() noexcept;

    void
    complete_rtb_request_history_match() noexcept;

    void
    add_rtb_request_campaign_selection() noexcept;

    void
    complete_rtb_request_campaign_selection() noexcept;

    RtbRequestInProgressStats
    rtb_request_in_progress_stats() noexcept;

    void
    add_user_bind_request() noexcept;

    void
    complete_user_bind_request() noexcept;

    void
    add_user_bind_rejected_request() noexcept;

    void
    add_user_bind_match_request() noexcept;

    void
    complete_user_bind_match_request() noexcept;

    void
    add_user_bind_match_rejected_request() noexcept;

    void
    add_user_bind_match_channel_request() noexcept;

    void
    complete_user_bind_match_channel_request() noexcept;

    void
    add_user_bind_match_get_profile_request() noexcept;

    void
    complete_user_bind_match_get_profile_request() noexcept;

    void
    add_user_bind_match_merge_request() noexcept;

    void
    complete_user_bind_match_merge_request() noexcept;

    void
    add_user_bind_match_remove_request() noexcept;

    void
    complete_user_bind_match_remove_request() noexcept;

    void
    add_user_bind_match_history_request() noexcept;

    void
    complete_user_bind_match_history_request() noexcept;

    void
    add_user_bind_match_campaign_request() noexcept;

    void
    complete_user_bind_match_campaign_request() noexcept;

    virtual Generics::Values_var
    dump_stats();

  protected:
    virtual
    ~StatHolder() noexcept = default;

  private:
    StatData stat_data_;
    Sync::PosixMutex mutex_;
  };

  typedef ReferenceCounting::SmartPtr<StatHolder>
    StatHolder_var;
}
