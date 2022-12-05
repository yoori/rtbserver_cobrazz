#ifndef REQUESTOPERATIONDISTRIBUTOR_HPP
#define REQUESTOPERATIONDISTRIBUTOR_HPP

#include "RequestOperationProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  class RequestOperationDistributor:
    public ReferenceCounting::AtomicImpl,
    public RequestOperationProcessor
  {
  public:
    RequestOperationDistributor(
      unsigned long distrib_count,
      unsigned long index,
      unsigned long services_count,
      RequestOperationProcessor* request_operation_processor,
      RequestOperationProcessor* other_request_operation_processor)
      noexcept;

    virtual void
    process_impression(
      const ImpressionInfo& impression_info)
      /*throw(RequestOperationProcessor::Exception)*/;

    virtual void
    process_action(
      const AdServer::Commons::UserId& new_user_id,
      RequestContainerProcessor::ActionType action_type,
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id)
      /*throw(RequestOperationProcessor::Exception)*/;

    virtual void
    process_impression_post_action(
      const AdServer::Commons::UserId& user_id,
      const AdServer::Commons::RequestId& request_id,
      const RequestPostActionInfo& request_post_action_info)
      /*throw(RequestOperationProcessor::Exception)*/;

    virtual void
    change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile)
      /*throw(Exception)*/;

    void
    request_operation_processor(
      RequestOperationProcessor* request_operation_processor)
      noexcept;

  protected:
    RequestOperationProcessor_var
    get_request_operation_processor_(
      const AdServer::Commons::UserId& user_id)
      noexcept;

  private:
    const unsigned long distrib_count_;
    const unsigned long index_;
    const unsigned long services_count_;
    RequestOperationProcessor_var request_operation_processor_;
    RequestOperationProcessor_var other_request_operation_processor_;
  };

  typedef ReferenceCounting::SmartPtr<RequestOperationDistributor>
    RequestOperationDistributor_var;
}
}

#endif /*REQUESTOPERATIONDISTRIBUTOR_HPP*/
