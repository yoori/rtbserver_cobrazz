#pragma once

#include <atomic>

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/Time.hpp>

#include "BiddingFrontend.hpp"
#include "Stage.hpp"

namespace AdServer::Bidding
{
  // RequestParamsHolder
  class RequestParamsHolder:
     public AdServer::Bidding::CampaignManager::RequestParams,
     public ReferenceCounting::AtomicImpl
  {
  protected:
    virtual
    ~RequestParamsHolder() noexcept = default;
  };

  typedef ReferenceCounting::SmartPtr<RequestParamsHolder>
    RequestParamsHolder_var;

  typedef ReferenceCounting::ConstPtr<RequestParamsHolder>
    ConstRequestParamsHolder_var;

  //
  // BidRequestState
  //
  class BidRequestState:
    public ReferenceCounting::AtomicImpl
  {
    friend class Frontend;

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(Invalid, Exception);

  public:
    BidRequestState(
      Frontend* bid_frontend,
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time)
      /*throw(Invalid)*/;

    // delegate self to Bidding::Frontend
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

    RequestParamsHolder_var&
    request_params() noexcept;

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

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

  protected:
    virtual
    ~BidRequestState() noexcept
    {
      assert(to_interrupt_ > 0);
      assert(response_sent_);
    }

    bool
    parse_request_() noexcept;

    void
    complete_request_(
      bool not_interrupted,
      AdServer::Bidding::CampaignManager::RequestCreativeResult&
        campaign_match_result) noexcept;

    bool
    complete_request_impl_(
      bool not_interrupted,
      AdServer::Bidding::CampaignManager::RequestCreativeResult&
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
    print_available_request_debug_info_() noexcept;

    void
    print_time_metering_debug_info_() noexcept;

  protected:
    Frontend* bid_frontend_;
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
    RequestParamsHolder_var request_params_;
    std::string keywords_;

    Stage current_stage_ = Stage::Initial;
    std::mutex mutex_current_stage_;

  private:
    FCGI::BaseHttpResponseWriter_var response_writer_;
    bool response_sent_;
    bool request_debug_info_printed_ = false;
    bool time_metering_debug_info_printed_ = false;
  };

  typedef ReferenceCounting::SmartPtr<BidRequestState>
    BidRequestState_var;
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
  RequestParamsHolder_var&
  BidRequestState::request_params() noexcept
  {
    return request_params_;
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
