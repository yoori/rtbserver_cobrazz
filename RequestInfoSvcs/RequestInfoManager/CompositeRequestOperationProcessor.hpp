#pragma once

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "RequestOperationProcessor.hpp"

namespace AdServer::RequestInfoSvcs
{
  /**
   * CompositeRequestOperationProcessor
   * delegate request processing to child processors
   */
  class CompositeRequestOperationProcessor:
    public virtual RequestOperationProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, RequestOperationProcessor::Exception);

    void add_child_processor(RequestOperationProcessor* child_processor)
      /*throw(Exception)*/;

    virtual void
    process_impression(const ImpressionInfo& impression_info)
      /*throw(Exception)*/;

    virtual AdServer::Commons::Awaitable<void>
    co_process_impression(const ImpressionInfo& impression_info);

    virtual void
    process_action(
      const AdServer::Commons::UserId& new_user_id,
      RequestContainerProcessor::ActionType action_type,
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id)
      /*throw(Exception)*/;

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
      const AdServer::RequestInfoSvcs::RequestPostActionInfo& request_post_action_info)
      /*throw(Exception)*/;

    virtual AdServer::Commons::Awaitable<void>
    co_process_impression_post_action(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const AdServer::RequestInfoSvcs::RequestPostActionInfo&
        request_post_action_info);

    virtual void
    change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile)
      /*throw(Exception)*/;

    virtual AdServer::Commons::Awaitable<void>
    co_change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile);

  protected:
    virtual ~CompositeRequestOperationProcessor() noexcept {}

  private:
    typedef std::list<RequestOperationProcessor_var> RequestOperationProcessorList;
    RequestOperationProcessorList child_processors_;
  };

  typedef
    ReferenceCounting::SmartPtr<CompositeRequestOperationProcessor>
    CompositeRequestOperationProcessor_var;
}

namespace AdServer::RequestInfoSvcs
{
  // CompositeRequestOperationProcessor
  inline
  void
  CompositeRequestOperationProcessor::add_child_processor(
    RequestOperationProcessor* child_processor) /*throw(Exception)*/
  {
    RequestOperationProcessor_var add_processor(ReferenceCounting::add_ref(child_processor));
    child_processors_.push_back(add_processor);
  }

  void
  CompositeRequestOperationProcessor::process_impression(const ImpressionInfo& impression_info)
    /*throw(Exception)*/
  {
    for (RequestOperationProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_impression(impression_info);
    }
  }

  AdServer::Commons::Awaitable<void>
  CompositeRequestOperationProcessor::co_process_impression(const ImpressionInfo& impression_info)
  {
    for (RequestOperationProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      co_await (*it)->co_process_impression(impression_info);
    }
  }

  void
  CompositeRequestOperationProcessor::process_impression_post_action(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const AdServer::RequestInfoSvcs::RequestPostActionInfo& request_post_action_info)
    /*throw(Exception)*/
  {
    for (RequestOperationProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_impression_post_action(new_user_id, request_id, request_post_action_info);
    }
  }

  AdServer::Commons::Awaitable<void>
  CompositeRequestOperationProcessor::co_process_impression_post_action(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const AdServer::RequestInfoSvcs::RequestPostActionInfo&
      request_post_action_info)
  {
    for (RequestOperationProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      co_await (*it)->co_process_impression_post_action(
        new_user_id,
        request_id,
        request_post_action_info);
    }
  }

  void
  CompositeRequestOperationProcessor::process_action(
    const AdServer::Commons::UserId& new_user_id,
    RequestContainerProcessor::ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
    /*throw(Exception)*/
  {
    for (RequestOperationProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->process_action(new_user_id, action_type, time, request_id);
    }
  }

  AdServer::Commons::Awaitable<void>
  CompositeRequestOperationProcessor::co_process_action(
    const AdServer::Commons::UserId& new_user_id,
    RequestContainerProcessor::ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
  {
    for (RequestOperationProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      co_await (*it)->co_process_action(new_user_id, action_type, time, request_id);
    }
  }

  void
  CompositeRequestOperationProcessor::change_request_user_id(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const Generics::ConstSmartMemBuf* request_profile)
    /*throw(Exception)*/
  {
    for (RequestOperationProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      (*it)->change_request_user_id(new_user_id, request_id, request_profile);
    }
  }

  AdServer::Commons::Awaitable<void>
  CompositeRequestOperationProcessor::co_change_request_user_id(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const Generics::ConstSmartMemBuf* request_profile)
  {
    for (RequestOperationProcessorList::iterator it = child_processors_.begin();
        it != child_processors_.end();
        ++it)
    {
      co_await (*it)->co_change_request_user_id(new_user_id, request_id, request_profile);
    }
  }
}
