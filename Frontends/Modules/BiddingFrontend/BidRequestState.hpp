#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string_view>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>

#include "BiddingFrontendCore.hpp"
#include "DebugSink.hpp"
#include "Stage.hpp"

namespace AdServer::Bidding
{
  //
  // BidRequestState
  //
  class BidRequestState:
    public std::enable_shared_from_this<BidRequestState>
  {
    friend class BiddingFrontendCore;

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(Invalid, Exception);

  public:
    BidRequestState(
      BiddingFrontendCore* bid_frontend,
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time)
      /*throw(Invalid)*/;

    // delegate self to BiddingFrontendCore
    void
    execute() noexcept;

    void
    interrupt() noexcept;

    void
    write_interrupted_empty_response(
      const String::SubString& interrupted_step) noexcept;

    void
    init_debug_info() noexcept;

    bool
    interrupted() const noexcept;

    const Generics::Time&
    start_processing_time() const noexcept;

    const RequestInfo&
    request_info() const noexcept;

    virtual std::string_view
    channel_keywords() const noexcept;

    const std::string&
    hostname() const noexcept;

    void set_current_stage(
      const Stage stage) noexcept;

    Stage get_current_stage();

    virtual void
    print_request(std::ostream& out) const noexcept = 0;

    // processing stages
    // read request & transform it to holder
    virtual bool
    read_request() noexcept = 0;

    // fill parameters by request
    /*
    virtual bool
    fill_request_info(std::string& keywords)
      noexcept = 0;
    */

    // write response (convert holder to response)
    // return true if response sent, false otherwise
    virtual bool
    write_response(
      const AdServer::Bidding::CampaignManager::RequestCreativeResult&
        campaign_match_result)
      noexcept = 0;

    virtual void
    write_empty_response(
      unsigned int code,
      bool response_claimed = false)
      noexcept = 0;

    virtual void
    clear() noexcept;

    virtual
    ~BidRequestState() noexcept
    {
      assert(to_interrupt_ > 0);
      assert(response_sent_);
    }

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

  protected:
    bool
    parse_request_() noexcept;

    void
    complete_request_(
      bool not_interrupted,
      std::shared_ptr<
        const AdServer::Bidding::CampaignManager::RequestCreativeResult>
        campaign_match_result) noexcept;

    bool
    complete_request_impl_(
      bool not_interrupted,
      std::shared_ptr<
        const AdServer::Bidding::CampaignManager::RequestCreativeResult>
        campaign_match_result) noexcept;

    void
    finish_(bool write_empty_response) noexcept;

    bool
    check_interrupt_(const Stage stage)
      noexcept;

    void
    write_response_(
      int code,
      FCGI::HttpResponse_var response,
      bool response_claimed = false)
      noexcept;

    bool
    claim_response_() noexcept;

    void
    write_interrupted_empty_response_(
      const String::SubString& interrupted_step) noexcept;

    bool
    require_debug_info_() noexcept;

    void
    print_user_resolving_debug_info_(
      const DebugSink::UserResolvingDebugInfo& user_resolving_debug_info,
      const StageResult* stage) noexcept;

    void
    print_channel_matching_debug_info_(
      const adserver::channel_svcs::channel_server::MatchResponse& response,
      const StageResult* stage) noexcept;

    void
    print_history_matching_debug_info_(
      const adserver::user_info_svcs::user_info_manager::MatchResult&
        match_result,
      const StageResult* stage) noexcept;

    void
    print_available_request_debug_info_() noexcept;

    void
    print_time_metering_debug_info_() noexcept;

  protected:
    BiddingFrontendCore* bid_frontend_;
    FCGI::HttpRequestHolder_var request_holder_;
    const Generics::Time start_processing_time_;

    RequestInfo request_info_;
    DebugSink debug_sink_;
    RequestTimeMetering request_time_metering_;
    AdServer::Commons::UserId resolved_user_id_;

    mutable Generics::AtomicInt to_interrupt_;
    std::atomic<bool> timeout_interrupted_;

    /// The host performed last unbreakable operation.
    std::string hostname_;

    Stage current_stage_ = Stage::Initial;
    std::mutex mutex_current_stage_;
    std::mutex debug_sink_mutex_;

  private:
    FCGI::BaseHttpResponseWriter_var response_writer_;
    bool response_sent_;
    bool request_debug_info_printed_ = false;
    bool time_metering_debug_info_printed_ = false;
  };

  using BidRequestState_var = std::shared_ptr<BidRequestState>;
}

namespace AdServer::Bidding
{
  inline
  const Generics::Time&
  BidRequestState::start_processing_time() const noexcept
  {
    return start_processing_time_;
  }

  inline
  const RequestInfo&
  BidRequestState::request_info() const noexcept
  {
    return request_info_;
  }

  inline
  const std::string&
  BidRequestState::hostname() const noexcept
  {
    return hostname_;
  }

  inline
  void BidRequestState::set_current_stage(
    const Stage stage) noexcept
  {
    try
    {
      std::lock_guard lock(mutex_current_stage_);
      current_stage_ = stage;
    }
    catch (...)
    {
    }
  }

  inline
  Stage
  BidRequestState::get_current_stage()
  {
    std::lock_guard lock(mutex_current_stage_);
    return current_stage_;
  }

  inline
  bool
  BidRequestState::interrupted() const noexcept
  {
    return to_interrupt_ != 0 ||
      timeout_interrupted_.load(std::memory_order_relaxed);
  }
}
