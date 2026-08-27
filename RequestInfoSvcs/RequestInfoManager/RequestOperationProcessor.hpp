#pragma once

#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/MemBuf.hpp>

#include <Commons/Coro/Awaitable.hpp>
#include <Commons/UserInfoManip.hpp>

#include "RequestActionProcessor.hpp"

namespace AdServer::RequestInfoSvcs
{
  struct RequestOperationProcessor:
    public virtual ReferenceCounting::Interface
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    virtual void
    process_impression(const ImpressionInfo& impression_info)
      /*throw(Exception)*/ = 0;

    virtual AdServer::Commons::Awaitable<void>
    co_process_impression(const ImpressionInfo& impression_info);

    virtual void
    process_action(
      const AdServer::Commons::UserId& new_user_id,
      RequestContainerProcessor::ActionType action_type,
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id)
      /*throw(Exception)*/ = 0;

    virtual AdServer::Commons::Awaitable<void>
    co_process_action(
      const AdServer::Commons::UserId& new_user_id,
      RequestContainerProcessor::ActionType action_type,
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id);

    virtual void
    process_impression_post_action(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const RequestPostActionInfo& request_post_action_info)
      /*throw(Exception)*/ = 0;

    virtual AdServer::Commons::Awaitable<void>
    co_process_impression_post_action(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const RequestPostActionInfo& request_post_action_info);

    virtual void
    change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile)
      /*throw(Exception)*/ = 0;

    virtual AdServer::Commons::Awaitable<void>
    co_change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile);

  protected:
    virtual ~RequestOperationProcessor() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<RequestOperationProcessor>
    RequestOperationProcessor_var;
}

namespace AdServer::RequestInfoSvcs
{
  inline AdServer::Commons::Awaitable<void>
  RequestOperationProcessor::co_process_impression(const ImpressionInfo& impression_info)
  {
    process_impression(impression_info);
    co_return;
  }

  inline AdServer::Commons::Awaitable<void>
  RequestOperationProcessor::co_process_action(
    const AdServer::Commons::UserId& new_user_id,
    RequestContainerProcessor::ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
  {
    process_action(new_user_id, action_type, time, request_id);
    co_return;
  }

  inline AdServer::Commons::Awaitable<void>
  RequestOperationProcessor::co_process_impression_post_action(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const RequestPostActionInfo& request_post_action_info)
  {
    process_impression_post_action(new_user_id, request_id, request_post_action_info);
    co_return;
  }

  inline AdServer::Commons::Awaitable<void>
  RequestOperationProcessor::co_change_request_user_id(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const Generics::ConstSmartMemBuf* request_profile)
  {
    change_request_user_id(new_user_id, request_id, request_profile);
    co_return;
  }
}
