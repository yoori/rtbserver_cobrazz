/// @file BiddingFrontend/BiddingFrontendStat.hpp
#pragma once

#include <eh/Exception.hpp>
#include <atomic>
#include <array>
#include <map>
#include <mutex>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/MetricsProvider.hpp>
#include <Generics/Values.hpp>
#include "CampaignManagerTypes.hpp"


namespace AdServer
{
  class BiddingStatInfo;

  namespace Bidding
  {
    struct RequestInfo;
  }

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
      unsigned long history_post_match = 0;
    };

    struct StatData
    {
      StatData();

      unsigned long request_total;
      unsigned long request_finished_total;
      unsigned long request_total_bid;
      unsigned long skipped;
      unsigned long request_in_progress;
      unsigned long user_resolving_total;
      Generics::Time user_resolving_total_time;
      unsigned long user_resolving_in_progress;
      unsigned long trigger_match_total;
      Generics::Time trigger_match_total_time;
      unsigned long trigger_match_in_progress;
      unsigned long history_match_total;
      Generics::Time history_match_total_time;
      unsigned long history_match_in_progress;
      unsigned long campaign_selection_total;
      Generics::Time campaign_selection_total_time;
      unsigned long campaign_selection_in_progress;
      unsigned long history_post_match_total;
      Generics::Time history_post_match_total_time;
      unsigned long history_post_match_in_progress;
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
    };

    void
    flush(
      const Bidding::RequestInfo&,
      AdServer::Bidding::CampaignManager::RequestCreativeResult*,
      const Generics::Time& processing_time)
      noexcept;

    void
    add_skipped() noexcept;

    void
    add_timeout(const Generics::Time& timeout) noexcept;

    void
    add_selected_bid(unsigned long ccg_id) noexcept;

    std::map<unsigned long, unsigned long>
    selected_bids() noexcept;

    void
    add_rtb_request() noexcept;

    void
    complete_rtb_request() noexcept;

    void
    add_rtb_request_user_resolving() noexcept;

    void
    complete_rtb_request_user_resolving() noexcept;

    void
    add_rtb_request_user_resolving_time(const Generics::Time& time) noexcept;

    void
    add_rtb_request_trigger_match() noexcept;

    void
    complete_rtb_request_trigger_match() noexcept;

    void
    add_rtb_request_trigger_match_time(const Generics::Time& time) noexcept;

    void
    add_rtb_request_history_match() noexcept;

    void
    complete_rtb_request_history_match() noexcept;

    void
    add_rtb_request_history_match_time(const Generics::Time& time) noexcept;

    void
    add_rtb_request_campaign_selection() noexcept;

    void
    complete_rtb_request_campaign_selection() noexcept;

    void
    add_rtb_request_campaign_selection_time(const Generics::Time& time) noexcept;

    void
    add_rtb_request_history_post_match() noexcept;

    void
    complete_rtb_request_history_post_match() noexcept;

    void
    add_rtb_request_history_post_match_time(const Generics::Time& time) noexcept;

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
    static constexpr std::size_t TIMEOUT_COUNTERS_SIZE = 28;

    std::atomic<unsigned long> request_total_{0};
    std::atomic<unsigned long> request_finished_total_{0};
    std::atomic<unsigned long> request_total_bid_{0};
    std::atomic<unsigned long> skipped_{0};
    std::atomic<unsigned long> request_in_progress_{0};
    std::atomic<unsigned long> user_resolving_total_{0};
    std::atomic<long long> user_resolving_total_time_us_{0};
    std::atomic<unsigned long> user_resolving_in_progress_{0};
    std::atomic<unsigned long> trigger_match_total_{0};
    std::atomic<long long> trigger_match_total_time_us_{0};
    std::atomic<unsigned long> trigger_match_in_progress_{0};
    std::atomic<unsigned long> history_match_total_{0};
    std::atomic<long long> history_match_total_time_us_{0};
    std::atomic<unsigned long> history_match_in_progress_{0};
    std::atomic<unsigned long> campaign_selection_total_{0};
    std::atomic<long long> campaign_selection_total_time_us_{0};
    std::atomic<unsigned long> campaign_selection_in_progress_{0};
    std::atomic<unsigned long> history_post_match_total_{0};
    std::atomic<long long> history_post_match_total_time_us_{0};
    std::atomic<unsigned long> history_post_match_in_progress_{0};
    std::atomic<long long> processing_time_us_{0};
    std::atomic<unsigned long> user_bind_request_count_{0};
    std::atomic<unsigned long> user_bind_requests_{0};
    std::atomic<unsigned long> user_bind_rejected_requests_{0};
    std::atomic<unsigned long> user_bind_match_requests_{0};
    std::atomic<unsigned long> user_bind_match_rejected_requests_{0};
    std::atomic<unsigned long> user_bind_match_channel_requests_{0};
    std::atomic<unsigned long> user_bind_match_get_profile_requests_{0};
    std::atomic<unsigned long> user_bind_match_merge_requests_{0};
    std::atomic<unsigned long> user_bind_match_remove_requests_{0};
    std::atomic<unsigned long> user_bind_match_history_requests_{0};
    std::atomic<unsigned long> user_bind_match_campaign_requests_{0};
    std::array<std::atomic<unsigned long>, TIMEOUT_COUNTERS_SIZE>
      timeout_counters_{};
    std::mutex selected_bids_lock_;
    std::map<unsigned long, unsigned long> selected_bids_;
  };

  typedef ReferenceCounting::SmartPtr<StatHolder>
    StatHolder_var;

  class StatHolderMetricsProvider final : public Generics::MetricsProvider
  {
  public:
    explicit
    StatHolderMetricsProvider(StatHolder* stats);

    MetricArray
    get_values() override;

  private:
    StatHolder_var stats_;
  };
}
