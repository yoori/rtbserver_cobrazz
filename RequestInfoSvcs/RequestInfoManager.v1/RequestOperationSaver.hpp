#ifndef REQUESTOPERATIONSAVER_HPP
#define REQUESTOPERATIONSAVER_HPP

#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/MessageSaver.hpp>
#include "RequestOperationProcessor.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
  class RequestOperationSaver:
    public virtual RequestOperationProcessor,
    public virtual Generics::ActiveObject,
    public virtual ReferenceCounting::AtomicImpl,
    protected ProfilingCommons::MessageSaver
  {
  public:
    using MessageSaver::FileNameList;

  public:
    RequestOperationSaver(
      Logging::Logger* logger,
      const char* output_dir,
      const char* file_prefix,
      unsigned long chunks_count,
      const Generics::Time& flush_period)
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
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const RequestPostActionInfo& request_post_action_info)
      /*throw(RequestOperationProcessor::Exception)*/;

    virtual void
    change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile)
      /*throw(RequestOperationProcessor::Exception)*/;

    using MessageSaver::flush;

  protected:
    virtual
    ~RequestOperationSaver()
      noexcept
    {}

    void
    write_operation_(
      const AdServer::Commons::UserId& user_id,
      unsigned long op,
      const Generics::MemBuf& mem_buf)
      /*throw(eh::Exception)*/;

    FileHolderGuard_var
    get_file_holder_(const AdServer::Commons::UserId& user_id)
      /*throw(eh::Exception)*/;
  };

  typedef ReferenceCounting::SmartPtr<RequestOperationSaver>
    RequestOperationSaver_var;
}
}

#endif /*REQUESTOPERATIONSAVER_HPP*/
