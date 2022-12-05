#ifndef REQUESTOPERATIONPROCESSOR_HPP
#define REQUESTOPERATIONPROCESSOR_HPP

#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/MemBuf.hpp>

#include <Commons/UserInfoManip.hpp>

#include "RequestActionProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  struct RequestOperationProcessor:
    public virtual ReferenceCounting::Interface
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    virtual void
    process_impression(
      const ImpressionInfo& impression_info)
      /*throw(Exception)*/ = 0;

    virtual void
    process_action(
      const AdServer::Commons::UserId& new_user_id,
      RequestContainerProcessor::ActionType action_type,
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id)
      /*throw(Exception)*/ = 0;

    virtual void
    process_impression_post_action(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const RequestPostActionInfo& request_post_action_info)
      /*throw(Exception)*/ = 0;

    virtual void
    change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile)
      /*throw(Exception)*/ = 0;

  protected:
    virtual ~RequestOperationProcessor() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<RequestOperationProcessor>
    RequestOperationProcessor_var;
}
}

#endif /*REQUESTOPERATIONPROCESSOR_HPP*/
