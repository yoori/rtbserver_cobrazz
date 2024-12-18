#ifndef BIDDINGFRONTENDGRPC_BIDREQUESTTASK_HPP
#define BIDDINGFRONTENDGRPC_BIDREQUESTTASK_HPP

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/TaskRunner.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <Frontends/FrontendCommons/GrpcCampaignManagerPool.hpp>
#include <Frontends/Modules/BiddingFrontend/Stage.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/BiddingFrontend.hpp>

namespace AdServer::Bidding::Grpc
{
  class RequestParamsHolder final:
     public FrontendCommons::GrpcCampaignManagerPool::RequestParams,
     public ReferenceCounting::AtomicImpl
  {
  protected:
    ~RequestParamsHolder() override = default;
  };

  using RequestParamsHolder_var = ReferenceCounting::SmartPtr<RequestParamsHolder>;
  using ConstRequestParamsHolder_var = ReferenceCounting::ConstPtr<RequestParamsHolder>;

  class BidRequestTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(Invalid, Exception);

  public:
    BidRequestTask(
      Frontend* bid_frontend,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time);

    // delegate self to Bidding::Frontend
    void execute() noexcept override;

    void interrupt() noexcept;

    bool interrupted() const noexcept;

    const Generics::Time& start_processing_time() const noexcept;

    const RequestInfo& request_info() const noexcept;

    RequestParamsHolder_var& request_params() noexcept;

    std::string& hostname() noexcept;

    const std::string& hostname() const noexcept;

    void set_current_stage(const Stage stage) noexcept;

    Stage get_current_stage() noexcept;

    virtual void print_request(std::ostream& out) const noexcept = 0;

    // processing stages
    // read request & transform it to holder
    virtual bool read_request() noexcept = 0;

    // write response (convert holder to response)
    // return true if response sent, false otherwise
    virtual bool write_response(
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
        campaign_match_result) noexcept = 0;

    virtual void write_empty_response(unsigned int code) noexcept = 0;

    virtual void clear() noexcept;

  protected:
    virtual ~BidRequestTask() noexcept
    {
      assert(to_interrupt_ > 0);
      assert(response_sent_);
    }

    // return true if response sent
    bool execute_() noexcept;

    bool check_interrupt_(const Stage stage) noexcept;

    void write_response_(
      int code,
      FrontendCommons::HttpResponse_var response) noexcept;

  protected:
    Frontend* bid_frontend_;

    FrontendCommons::HttpRequestHolder_var request_holder_;

    const Generics::Time start_processing_time_;

    RequestInfo request_info_;

    mutable Generics::AtomicInt to_interrupt_;
    /// The host performed last unbreakable operation.
    std::string hostname_;

    RequestParamsHolder_var request_params_;

    std::string keywords_;

    Stage current_stage_ = Stage::Initial;

    userver::engine::Mutex mutex_current_stage_;

  private:
    FrontendCommons::HttpResponseWriter_var response_writer_;

    bool response_sent_;
  };

  using BidRequestTask_var = ReferenceCounting::SmartPtr<BidRequestTask>;

} // namespace AdServer::Bidding::Grpc

namespace AdServer::Bidding::Grpc
{
  inline const Generics::Time& BidRequestTask::start_processing_time() const noexcept
  {
    return start_processing_time_;
  }

  inline const RequestInfo& BidRequestTask::request_info() const noexcept
  {
    return request_info_;
  }

  inline RequestParamsHolder_var& BidRequestTask::request_params() noexcept
  {
    return request_params_;
  }

  inline std::string& BidRequestTask::hostname() noexcept
  {
    return hostname_;
  }

  inline const std::string& BidRequestTask::hostname() const noexcept
  {
    return hostname_;
  }

  inline void BidRequestTask::set_current_stage(
    const Stage stage) noexcept
  {
    std::lock_guard lock(mutex_current_stage_);
    current_stage_ = stage;
  }

  inline Stage BidRequestTask::get_current_stage() noexcept
  {
    std::lock_guard lock(mutex_current_stage_);
    return current_stage_;
  }

  inline bool BidRequestTask::interrupted() const noexcept
  {
    return to_interrupt_ != 0;
  }
} // namespace AdServer::Bidding::Grpc

#endif /*BIDDINGFRONTENDGRPC_BIDREQUESTTASK_HPP*/