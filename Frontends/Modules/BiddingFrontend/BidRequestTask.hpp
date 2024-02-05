#ifndef BIDREQUESTTASK_HPP_
#define BIDREQUESTTASK_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/Time.hpp>
#include <Generics/TaskRunner.hpp>

#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>

#include "BiddingFrontend.hpp"

namespace AdServer
{
namespace Bidding
{
  // RequestParamsHolder
  class RequestParamsHolder:
     public AdServer::CampaignSvcs::CampaignManager::RequestParams,   
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
  // BidRequestTask
  //
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
      const Generics::Time& start_processing_time)
      /*throw(Invalid)*/;

    // delegate self to Bidding::Frontend
    virtual void
    execute() noexcept;

    void
    interrupt() noexcept;

    bool
    interrupted() const noexcept;

    const Generics::Time&
    start_processing_time() const noexcept;

    const RequestInfo&
    request_info() const noexcept;

    RequestParamsHolder_var&
    request_params() noexcept;

    CORBA::String_var&
    hostname() noexcept;

    const CORBA::String_var&
    hostname() const noexcept;

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
      const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
        campaign_match_result)
      noexcept = 0;

    virtual void
    write_empty_response(unsigned int code)
      noexcept = 0;

    virtual void
    clear() noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

  protected:
    virtual
    ~BidRequestTask() noexcept
    {
      assert(to_interrupt_ > 0);
      assert(response_sent_);
    }

    // return true if response sent
    bool
    execute_() noexcept;

    bool
    check_interrupt_(const char* stage)
      noexcept;

    void
    write_response_(
      int code,
      FrontendCommons::HttpResponse_var response)
      noexcept;

  protected:
    Frontend* bid_frontend_;
    FrontendCommons::HttpRequestHolder_var request_holder_;
    const Generics::Time start_processing_time_;

    RequestInfo request_info_;

    //mutable SyncPolicy::Mutex lock_;
    mutable Generics::AtomicInt to_interrupt_;
    //bool finished;
    //bool bad_request;

    /// The host performed last unbreakable operation.
    CORBA::String_var hostname_;
    RequestParamsHolder_var request_params_;
    std::string keywords_;

  private:
    FrontendCommons::HttpResponseWriter_var response_writer_;
    bool response_sent_;
  };

  typedef ReferenceCounting::SmartPtr<BidRequestTask>
    BidRequestTask_var;
}
}

namespace AdServer
{
namespace Bidding
{
  inline
  const Generics::Time&
  BidRequestTask::start_processing_time() const noexcept
  {
    return start_processing_time_;
  }

  inline
  const RequestInfo&
  BidRequestTask::request_info() const noexcept
  {
    return request_info_;
  }

  inline
  RequestParamsHolder_var&
  BidRequestTask::request_params() noexcept
  {
    return request_params_;
  }

  inline
  CORBA::String_var&
  BidRequestTask::hostname() noexcept
  {
    return hostname_;
  }

  inline
  const CORBA::String_var&
  BidRequestTask::hostname() const noexcept
  {
    return hostname_;
  }

  inline
  bool
  BidRequestTask::interrupted() const noexcept
  {
    return to_interrupt_ != 0;
  }
}
}

#endif /*BIDREQUESTTASK_HPP_*/
